/*
 *  Copyright (c) 2015 Jouni Pentikäinen <joupent@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_keyframe_channel.h"
#include "KoID.h"
#include "kis_global.h"
#include "kis_node.h"
#include "kis_time_range.h"
#include "kundo2command.h"
#include "kis_keyframe_commands.h"

#include <QMap>

const KoID KisKeyframeChannel::Content = KoID("content", ki18n("Content"));
const KoID KisKeyframeChannel::Opacity = KoID("opacity", ki18n("Opacity"));
const KoID KisKeyframeChannel::TransformArguments = KoID("transform_arguments", ki18n("Transform"));
const KoID KisKeyframeChannel::TransformPositionX = KoID("transform_pos_x", ki18n("X position"));
const KoID KisKeyframeChannel::TransformPositionY = KoID("transform_pos_y", ki18n("Y position"));

struct KisKeyframeChannel::Private
{
    Private() {}
    Private(const Private &rhs, KisNodeWSP newParentNode) {
        keys = rhs.keys;
        node = newParentNode;
        id = rhs.id;
        defaultBounds = rhs.defaultBounds;
    }

    KeyframesMap keys;
    KisNodeWSP node;
    KoID id;
    KisDefaultBoundsBaseSP defaultBounds;
};

KisKeyframeChannel::KisKeyframeChannel(const KoID &id, KisDefaultBoundsBaseSP defaultBounds)
    : m_d(new Private)
{
    m_d->id = id;
    m_d->node = 0;
    m_d->defaultBounds = defaultBounds;
}

KisKeyframeChannel::KisKeyframeChannel(const KisKeyframeChannel &rhs, KisNodeWSP newParentNode)
    : m_d(new Private(*rhs.m_d, newParentNode))
{
    KIS_ASSERT_RECOVER_NOOP(&rhs != this);
}

KisKeyframeChannel::~KisKeyframeChannel()
{}

QString KisKeyframeChannel::id() const
{
    return m_d->id.id();
}

QString KisKeyframeChannel::name() const
{
    return m_d->id.name();
}

void KisKeyframeChannel::setNode(KisNodeWSP node)
{
    m_d->node = node;
}

KisNodeWSP KisKeyframeChannel::node() const
{
    return m_d->node;
}

int KisKeyframeChannel::keyframeCount() const
{
    return m_d->keys.count();
}

KisKeyframeChannel::KeyframesMap& KisKeyframeChannel::keys()
{
    return m_d->keys;
}

const KisKeyframeChannel::KeyframesMap& KisKeyframeChannel::constKeys() const
{
    return m_d->keys;
}

#define LAZY_INITIALIZE_PARENT_COMMAND(cmd)       \
    QScopedPointer<KUndo2Command> __tempCommand;  \
    if (!parentCommand) {                         \
        __tempCommand.reset(new KUndo2Command()); \
        cmd = __tempCommand.data();               \
    }

KisKeyframeSP KisKeyframeChannel::addKeyframe(int time, KUndo2Command *parentCommand)
{
    LAZY_INITIALIZE_PARENT_COMMAND(parentCommand);
    return insertKeyframe(time, KisKeyframeSP(), parentCommand);
}

KisKeyframeSP KisKeyframeChannel::copyKeyframe(const KisKeyframeSP keyframe, int newTime, KUndo2Command *parentCommand)
{
    LAZY_INITIALIZE_PARENT_COMMAND(parentCommand);
    return insertKeyframe(newTime, keyframe, parentCommand);
}

KisKeyframeSP KisKeyframeChannel::insertKeyframe(int time, const KisKeyframeSP copySrc, KUndo2Command *parentCommand)
{
    KisKeyframeSP keyframe = keyframeAt(time);
    if (keyframe) {
        deleteKeyframeImpl(keyframe, parentCommand, false);
    }

    Q_ASSERT(parentCommand);
    keyframe = createKeyframe(time, copySrc, parentCommand);

    KUndo2Command *cmd = new KisReplaceKeyframeCommand(this, keyframe->time(), keyframe, parentCommand);
    cmd->redo();

    return keyframe;
}

bool KisKeyframeChannel::deleteKeyframe(KisKeyframeSP keyframe, KUndo2Command *parentCommand)
{
    return deleteKeyframeImpl(keyframe, parentCommand, true);
}

bool KisKeyframeChannel::moveKeyframe(KisKeyframeSP keyframe, int newTime, KUndo2Command *parentCommand)
{
    LAZY_INITIALIZE_PARENT_COMMAND(parentCommand);

    if (newTime == keyframe->time()) return false;

    KisKeyframeSP other = keyframeAt(newTime);
    if (other) {
        deleteKeyframeImpl(other, parentCommand, false);
    }

    const int srcTime = keyframe->time();

    KUndo2Command *cmd = new KisMoveFrameCommand(this, keyframe, srcTime, newTime, parentCommand);
    cmd->redo();

    if (srcTime == 0) {
        addKeyframe(srcTime, parentCommand);
    }

    return true;
}

bool KisKeyframeChannel::deleteKeyframeImpl(KisKeyframeSP keyframe, KUndo2Command *parentCommand, bool recreate)
{
    LAZY_INITIALIZE_PARENT_COMMAND(parentCommand);

    Q_ASSERT(parentCommand);

    KUndo2Command *cmd = new KisReplaceKeyframeCommand(this, keyframe->time(), KisKeyframeSP(), parentCommand);
    cmd->redo();
    destroyKeyframe(keyframe, parentCommand);

    if (recreate && keyframe->time() == 0) {
        addKeyframe(0, parentCommand);
    }

    return true;
}

void KisKeyframeChannel::moveKeyframeImpl(KisKeyframeSP keyframe, int newTime)
{
    KIS_ASSERT_RECOVER_RETURN(keyframe);
    KIS_ASSERT_RECOVER_RETURN(!keyframeAt(newTime));

    KisTimeRange rangeSrc = affectedFrames(keyframe->time());
    QRect rectSrc = affectedRect(keyframe);

    emit sigKeyframeAboutToBeMoved(keyframe.data(), newTime);

    m_d->keys.remove(keyframe->time());
    int oldTime = keyframe->time();
    keyframe->setTime(newTime);
    m_d->keys.insert(newTime, keyframe);

    emit sigKeyframeMoved(keyframe.data(), oldTime);

    KisTimeRange rangeDst = affectedFrames(keyframe->time());
    QRect rectDst = affectedRect(keyframe);

    requestUpdate(rangeSrc, rectSrc);
    requestUpdate(rangeDst, rectDst);
}

KisKeyframeSP KisKeyframeChannel::replaceKeyframeAt(int time, KisKeyframeSP newKeyframe)
{
    Q_ASSERT(newKeyframe.isNull() || time == newKeyframe->time());

    KisKeyframeSP existingKeyframe = keyframeAt(time);
    if (!existingKeyframe.isNull()) {
        removeKeyframeLogical(existingKeyframe);
    }

    if (!newKeyframe.isNull()) {
        insertKeyframeLogical(newKeyframe);
    }

    return existingKeyframe;
}

void KisKeyframeChannel::insertKeyframeLogical(KisKeyframeSP keyframe)
{
    const int time = keyframe->time();

    emit sigKeyframeAboutToBeAdded(keyframe.data());
    m_d->keys.insert(time, keyframe);
    emit sigKeyframeAdded(keyframe.data());

    QRect rect = affectedRect(keyframe);
    KisTimeRange range = affectedFrames(time);
    requestUpdate(range, rect);
}

void KisKeyframeChannel::removeKeyframeLogical(KisKeyframeSP keyframe)
{
    QRect rect = affectedRect(keyframe);
    KisTimeRange range = affectedFrames(keyframe->time());

    emit sigKeyframeAboutToBeRemoved(keyframe.data());
    m_d->keys.remove(keyframe->time());
    emit sigKeyframeRemoved(keyframe.data());

    requestUpdate(range, rect);
}

KisKeyframeSP KisKeyframeChannel::keyframeAt(int time) const
{
    KeyframesMap::const_iterator i = m_d->keys.constFind(time);
    if (i != m_d->keys.constEnd()) {
        return i.value();
    }

    return KisKeyframeSP();
}

KisKeyframeSP KisKeyframeChannel::activeKeyframeAt(int time) const
{
    KeyframesMap::const_iterator i = activeKeyIterator(time);
    if (i != m_d->keys.constEnd()) {
        return i.value();
    }

    return KisKeyframeSP();
}

KisKeyframeSP KisKeyframeChannel::currentlyActiveKeyframe() const
{
    return activeKeyframeAt(currentTime());
}

KisKeyframeSP KisKeyframeChannel::firstKeyframe() const
{
    return m_d->keys.first();
}

KisKeyframeSP KisKeyframeChannel::nextKeyframe(KisKeyframeSP keyframe) const
{
    KeyframesMap::const_iterator i = m_d->keys.constFind(keyframe->time());
    if (i == m_d->keys.constEnd()) return KisKeyframeSP(0);

    i++;

    if (i == m_d->keys.constEnd()) return KisKeyframeSP(0);
    return i.value();
}

KisKeyframeSP KisKeyframeChannel::previousKeyframe(KisKeyframeSP keyframe) const
{
    KeyframesMap::const_iterator i = m_d->keys.constFind(keyframe->time());
    if (i == m_d->keys.constBegin() || i == m_d->keys.constEnd()) return KisKeyframeSP(0);
    i--;

    return i.value();
}

KisKeyframeSP KisKeyframeChannel::lastKeyframe() const
{
    if (m_d->keys.isEmpty()) return KisKeyframeSP(0);

    return (m_d->keys.end()-1).value();
}

int KisKeyframeChannel::framesHash() const
{
    KeyframesMap::const_iterator it = m_d->keys.constBegin();
    KeyframesMap::const_iterator end = m_d->keys.constEnd();

    int hash = 0;

    while (it != end) {
        hash += it.key();
        ++it;
    }

    return hash;
}

QSet<int> KisKeyframeChannel::allKeyframeIds() const
{
    QSet<int> frames;

    KeyframesMap::const_iterator it = m_d->keys.constBegin();
    KeyframesMap::const_iterator end = m_d->keys.constEnd();

    while (it != end) {
        frames.insert(it.key());
        ++it;
    }

    return frames;
}

KisTimeRange KisKeyframeChannel::affectedFrames(int time) const
{
    if (m_d->keys.isEmpty()) return KisTimeRange::infinite(0);

    KeyframesMap::const_iterator active = activeKeyIterator(time);
    KeyframesMap::const_iterator next;

    int from;

    if (active == m_d->keys.constEnd()) {
        // No active keyframe, ie. time is before the first keyframe
        from = 0;
        next = m_d->keys.constBegin();
    } else {
        from = active.key();
        next = active + 1;
    }

    if (next == m_d->keys.constEnd()) {
        return KisTimeRange::infinite(from);
    } else {
        return KisTimeRange::fromTime(from, next.key() - 1);
    }
}

KisTimeRange KisKeyframeChannel::identicalFrames(int time) const
{
    KeyframesMap::const_iterator active = activeKeyIterator(time);

    if (active != m_d->keys.constEnd() && (active+1) != m_d->keys.constEnd()) {
        if (active->data()->interpolationMode() != KisKeyframe::Constant) {
            return KisTimeRange::fromTime(time, time);
        }
    }

    return affectedFrames(time);
}

int KisKeyframeChannel::keyframeRowIndexOf(KisKeyframeSP keyframe) const
{
    KeyframesMap::const_iterator it = m_d->keys.constBegin();
    KeyframesMap::const_iterator end = m_d->keys.constEnd();

    int row = 0;

    for (; it != end; ++it) {
        if (it.value().data() == keyframe) {
            return row;
        }

        row++;
    }

    return -1;
}

KisKeyframeSP KisKeyframeChannel::keyframeAtRow(int row) const
{
    KeyframesMap::const_iterator it = m_d->keys.constBegin();
    KeyframesMap::const_iterator end = m_d->keys.constEnd();

    for (; it != end; ++it) {
        if (row <= 0) {
            return it.value();
        }

        row--;
    }

    return KisKeyframeSP();
}

int KisKeyframeChannel::keyframeInsertionRow(int time) const
{
    KeyframesMap::const_iterator it = m_d->keys.constBegin();
    KeyframesMap::const_iterator end = m_d->keys.constEnd();

    int row = 0;

    for (; it != end; ++it) {
        if (it.value()->time() > time) {
            break;
        }
        row++;
    }

    return row;
}

QDomElement KisKeyframeChannel::toXML(QDomDocument doc, const QString &layerFilename)
{
    QDomElement channelElement = doc.createElement("channel");

    channelElement.setAttribute("name", id());

    Q_FOREACH (KisKeyframeSP keyframe, m_d->keys.values()) {
        QDomElement keyframeElement = doc.createElement("keyframe");
        keyframeElement.setAttribute("time", keyframe->time());

        saveKeyframe(keyframe, keyframeElement, layerFilename);

        channelElement.appendChild(keyframeElement);
    }

    return channelElement;
}

void KisKeyframeChannel::loadXML(const QDomElement &channelNode)
{
    for (QDomElement keyframeNode = channelNode.firstChildElement(); !keyframeNode.isNull(); keyframeNode = keyframeNode.nextSiblingElement()) {
        if (keyframeNode.nodeName().toUpper() != "KEYFRAME") continue;

        KisKeyframeSP keyframe = loadKeyframe(keyframeNode);

        m_d->keys.insert(keyframe->time(), keyframe);
    }
}





KisKeyframeSP KisKeyframeChannel::copyExternalKeyframe(KisKeyframeChannel *srcChannel, int srcTime, int dstTime, KUndo2Command *parentCommand)
{
    if (srcChannel->id() != id()) {
        warnKrita << "Cannot copy frames from different ids:" << ppVar(srcChannel->id()) << ppVar(id());
        return KisKeyframeSP();
    }

    LAZY_INITIALIZE_PARENT_COMMAND(parentCommand);

    KisKeyframeSP dstFrame = keyframeAt(dstTime);
    if (dstFrame) {
        deleteKeyframeImpl(dstFrame, parentCommand, false);
    }

    KisKeyframeSP newKeyframe = createKeyframe(dstTime, KisKeyframeSP(), parentCommand);
    uploadExternalKeyframe(srcChannel, srcTime, newKeyframe);

    KUndo2Command *cmd = new KisReplaceKeyframeCommand(this, newKeyframe->time(), newKeyframe, parentCommand);
    cmd->redo();

    return newKeyframe;
}

KisKeyframeChannel::KeyframesMap::const_iterator
KisKeyframeChannel::activeKeyIterator(int time) const
{
    KeyframesMap::const_iterator i = const_cast<const KeyframesMap*>(&m_d->keys)->upperBound(time);

    if (i == m_d->keys.constBegin()) return m_d->keys.constEnd();
    return --i;
}

void KisKeyframeChannel::requestUpdate(const KisTimeRange &range, const QRect &rect)
{
    if (m_d->node) {
        m_d->node->invalidateFrames(range, rect);

        int currentTime = m_d->defaultBounds->currentTime();
        if (range.contains(currentTime)) {
            m_d->node->setDirty(rect);
        }
    }
}

int KisKeyframeChannel::currentTime() const
{
    return m_d->defaultBounds->currentTime();
}

qreal KisKeyframeChannel::minScalarValue() const
{
    return 0;
}

qreal KisKeyframeChannel::maxScalarValue() const
{
    return 0;
}

qreal KisKeyframeChannel::scalarValue(const KisKeyframeSP keyframe) const
{
    Q_UNUSED(keyframe);

    return 0;
}

void KisKeyframeChannel::setScalarValue(KisKeyframeSP keyframe, qreal value, KUndo2Command *parentCommand)
{
    Q_UNUSED(keyframe);
    Q_UNUSED(value);
    Q_UNUSED(parentCommand);
}
