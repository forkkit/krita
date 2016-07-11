/*
 *  Copyright (c) 2016 Jouni Pentikäinen <joupent@gmail.com>
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

#include "kis_animation_curves_model.h"

#include <QAbstractItemModel>

#include "kis_node.h"
#include "kis_keyframe_channel.h"
#include "kis_scalar_keyframe_channel.h"

struct KisAnimationCurvesModel::Private
{
    QList<KisKeyframeChannel*> channels;

    KisKeyframeChannel * getChannelAt(const QModelIndex& index) {
        int row = index.row();

        if (row < 0 || row >= channels.size()) {
            return 0;
        }

        return channels.at(row);
    }
};

KisAnimationCurvesModel::KisAnimationCurvesModel(QObject *parent)
    : KisTimeBasedItemModel(parent)
    , m_d(new Private())
{}

KisAnimationCurvesModel::~KisAnimationCurvesModel()
{}

int KisAnimationCurvesModel::rowCount(const QModelIndex &parent) const
{
    return m_d->channels.size();
}

QVariant KisAnimationCurvesModel::data(const QModelIndex &index, int role) const
{
    KisKeyframeChannel *channel = m_d->getChannelAt(index);
    KisScalarKeyframeChannel *scalarChannel = dynamic_cast<KisScalarKeyframeChannel*>(channel);

    if (scalarChannel) {
        int time = index.column();
        KisKeyframeSP keyframe = channel->keyframeAt(time);

        switch (role) {
        case SpecialKeyframeExists:
            return !keyframe.isNull();
        case ScalarValueRole:
            return (!scalarChannel) ? QVariant() : scalarChannel->interpolatedValue(time);
        case LeftTangentRole:
            return (keyframe.isNull()) ? QVariant() : keyframe->leftTangent();
        case RightTangentRole:
            return (keyframe.isNull()) ? QVariant() : keyframe->rightTangent();
        case InterpolationModeRole:
            return (keyframe.isNull()) ? QVariant() : keyframe->interpolationMode();
        case CurveColorRole:
            return QColor(Qt::red);
        case PreviousKeyframeTime:
        {
            KisKeyframeSP active = channel->activeKeyframeAt(time);
            if (active.isNull()) return QVariant();
            if (active->time() < time) {
                return active->time();
            }
            KisKeyframeSP previous = channel->previousKeyframe(active);
            if (previous.isNull()) return QVariant();
            return previous->time();
        }
        case NextKeyframeTime:
        {
            KisKeyframeSP active = channel->activeKeyframeAt(time);
            if (active.isNull()) {
                KisKeyframeSP first = channel->firstKeyframe();
                if (!first.isNull() && first->time() > time) {
                    return first->time();
                }
                return QVariant();
            }
            KisKeyframeSP next = channel->nextKeyframe(active);
            if (next.isNull()) return QVariant();
            return next->time();
        }
        default:
            break;
        }
    }

    return KisTimeBasedItemModel::data(index, role);
}

bool KisAnimationCurvesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    // TODO
    return KisTimeBasedItemModel::setData(index, value, role);
}

QVariant KisAnimationCurvesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // TODO
    return KisTimeBasedItemModel::headerData(section, orientation, role);
}

void KisAnimationCurvesModel::slotCurrentNodeChanged(KisNodeSP node)
{
    beginResetModel();
    m_d->channels = node->keyframeChannels();
    endResetModel();
}

bool KisAnimationCurvesModel::removeFrames(const QModelIndexList &indexes)
{
    // TODO
    return false;
}

bool KisAnimationCurvesModel::offsetFrames(QVector<QPoint> srcIndexes, const QPoint &offset, bool copyFrames)
{
    // TODO
    return false;
}
