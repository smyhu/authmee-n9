/*
 * AuthMee for Nokia N9 - round countdown ring.
 * Copyright (c) 2026 smyhu. MIT License (see LICENSE).
 * Designed and written by Claude (Anthropic) together with smyhu.
 */
#ifndef AUTHMEE_RING_H
#define AUTHMEE_RING_H

#include <QtDeclarative/QDeclarativeItem>
#include <QtGui/QPainter>

class Ring : public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(qreal value READ value WRITE setValue)
    Q_PROPERTY(QColor color READ color WRITE setColor)
    Q_PROPERTY(QColor trackColor READ trackColor WRITE setTrackColor)
    Q_PROPERTY(qreal lineWidth READ lineWidth WRITE setLineWidth)
public:
    explicit Ring(QDeclarativeItem *parent = 0)
        : QDeclarativeItem(parent), m_value(1), m_color(Qt::green), m_track(QColor(255, 255, 255, 40)), m_line(5)
    {
        setFlag(QGraphicsItem::ItemHasNoContents, false);
    }
    qreal value() const { return m_value; }
    void setValue(qreal v) { m_value = qBound(qreal(0), v, qreal(1)); update(); }
    QColor color() const { return m_color; }
    void setColor(const QColor &c) { m_color = c; update(); }
    QColor trackColor() const { return m_track; }
    void setTrackColor(const QColor &c) { m_track = c; update(); }
    qreal lineWidth() const { return m_line; }
    void setLineWidth(qreal w) { m_line = w; update(); }

    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *)
    {
        p->setRenderHint(QPainter::Antialiasing, true);
        qreal half = m_line / 2 + 1;
        QRectF r = boundingRect().adjusted(half, half, -half, -half);
        p->setBrush(Qt::NoBrush);
        p->setPen(QPen(m_track, m_line, Qt::SolidLine, Qt::FlatCap));
        p->drawEllipse(r);
        if (m_value > 0) {
            p->setPen(QPen(m_color, m_line, Qt::SolidLine, Qt::RoundCap));
            p->drawArc(r, 90 * 16, -int(m_value * 360 * 16));
        }
    }

private:
    qreal m_value;
    QColor m_color;
    QColor m_track;
    qreal m_line;
};

#endif
