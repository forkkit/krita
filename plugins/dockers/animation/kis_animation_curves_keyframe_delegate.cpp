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

#include "kis_animation_curves_keyframe_delegate.h"

#include <QPainter>
#include <QApplication>

#include "kis_animation_curves_model.h"

const int NODE_RENDER_RADIUS = 2;
const int NODE_UI_RADIUS = 8;

struct KisAnimationCurvesKeyframeDelegate::Private
{
    Private(const TimelineRulerHeader *horizontalRuler, const KisAnimationCurvesValueRuler *verticalRuler)
        : horizontalRuler(horizontalRuler)
        , verticalRuler(verticalRuler)
    {}
    const TimelineRulerHeader *horizontalRuler;
    const KisAnimationCurvesValueRuler *verticalRuler;
    QPointF selectionOffset;
};

KisAnimationCurvesKeyframeDelegate::KisAnimationCurvesKeyframeDelegate(const TimelineRulerHeader *horizontalRuler, const KisAnimationCurvesValueRuler *verticalRuler, QObject *parent)
    : QAbstractItemDelegate(parent)
    , m_d(new Private(horizontalRuler, verticalRuler))
{}

KisAnimationCurvesKeyframeDelegate::~KisAnimationCurvesKeyframeDelegate()
{}

void KisAnimationCurvesKeyframeDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    bool selected = option.state & QStyle::State_Selected;
    QPointF center = nodeCenter(index, selected);

    QColor color;
    QColor bgColor = qApp->palette().color(QPalette::Window);
    if (selected) {
        color = (bgColor.value() > 128) ? Qt::black : Qt::white;
    } else {
        color = index.data(KisAnimationCurvesModel::CurveColorRole).value<QColor>();
    }

    painter->setPen(QPen(color, 0));
    painter->setBrush(color);
    painter->drawEllipse(center, NODE_RENDER_RADIUS, NODE_RENDER_RADIUS);

    if (selected) {
        QPointF leftTangent = leftHandle(index);
        QPointF rightTangent = rightHandle(index);

        painter->setPen(QPen(color, 1));
        painter->setBrush(bgColor);

        paintHandle(painter, center, leftTangent);
        paintHandle(painter, center, rightTangent);
    }
}

QSize KisAnimationCurvesKeyframeDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QSize(2*NODE_UI_RADIUS, 2*NODE_UI_RADIUS);
}

QPointF KisAnimationCurvesKeyframeDelegate::nodeCenter(const QModelIndex index, bool selected) const
{
    int section = m_d->horizontalRuler->logicalIndex(index.column());
    int x = m_d->horizontalRuler->sectionViewportPosition(section)
            + (m_d->horizontalRuler->sectionSize(section) / 2);

    float value = index.data(KisAnimationCurvesModel::ScalarValueRole).toReal();
    float y = m_d->verticalRuler->mapValueToView(value);

    QPointF center = QPointF(x, y);
    if (selected) center += m_d->selectionOffset;
    return center;
}

QPointF KisAnimationCurvesKeyframeDelegate::leftHandle(const QModelIndex index) const
{
    QPointF tangent = index.data(KisAnimationCurvesModel::LeftTangentRole).toPointF();

    float x = tangent.x() * m_d->horizontalRuler->defaultSectionSize();
    float y = tangent.y() * m_d->verticalRuler->scaleFactor();

    return QPointF(x, y);
}

QPointF KisAnimationCurvesKeyframeDelegate::rightHandle(const QModelIndex index) const
{
    QPointF tangent = index.data(KisAnimationCurvesModel::RightTangentRole).toPointF();

    float x = tangent.x() * m_d->horizontalRuler->defaultSectionSize();
    float y = tangent.y() * m_d->verticalRuler->scaleFactor();

    return QPointF(x, y);
}

void KisAnimationCurvesKeyframeDelegate::setSelectedItemVisualOffset(QPointF offset)
{
    m_d->selectionOffset = offset;
}

void KisAnimationCurvesKeyframeDelegate::paintHandle(QPainter *painter, QPointF nodePos, QPointF tangent) const
{
    QPointF handlePos = nodePos + tangent;

    painter->drawLine(nodePos, handlePos);
    painter->drawEllipse(handlePos, NODE_RENDER_RADIUS, NODE_RENDER_RADIUS);
}

QRect KisAnimationCurvesKeyframeDelegate::itemRect(const QModelIndex index) const
{
    QPointF center = nodeCenter(index, false);

    return QRect(center.x() - NODE_UI_RADIUS, center.y() - NODE_UI_RADIUS, 2*NODE_UI_RADIUS, 2*NODE_UI_RADIUS);
}

QRect KisAnimationCurvesKeyframeDelegate::visualRect(const QModelIndex index) const
{
    QPointF center = nodeCenter(index, false);
    QPointF leftHandlePos = center + leftHandle(index);
    QPointF rightHandlePos = center + rightHandle(index);

    int minX = qMin(center.x(), leftHandlePos.x()) - NODE_RENDER_RADIUS;
    int maxX = qMax(center.x(), rightHandlePos.x()) + NODE_RENDER_RADIUS;

    int minY = qMin(center.y(), qMin(leftHandlePos.y(), rightHandlePos.y())) - NODE_RENDER_RADIUS;
    int maxY = qMax(center.y(), qMax(leftHandlePos.y(), rightHandlePos.y())) + NODE_RENDER_RADIUS;

    return QRect(QPoint(minX, minY), QPoint(maxX, maxY));
}
