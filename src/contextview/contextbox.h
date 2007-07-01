/***************************************************************************
 * copyright     : (C) 2007 Seb Ruiz <ruiz@kde.org>                        *
 *                 (C) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com> *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_CONTEXTBOX_H
#define AMAROK_CONTEXTBOX_H

#include <QBrush>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsRectItem>
#include <QTimeLine>

class QGraphicsItem;
class QGraphicsRectItem;
class QGraphicsScene;
class QGraphicsSceneMouseEvent;
class QSize;
class QStyleOptionGraphicsItem;

namespace Context
{

class ContextBox : public QObject, public QGraphicsRectItem
{
    Q_OBJECT

    friend class GraphicsItemFader;

    public:
        explicit ContextBox( QGraphicsItem *parent = 0, QGraphicsScene *scene = 0 );
        ~ContextBox() { /* delete, disconnect and disembark */ }

        virtual void setTitle( const QString &title );
        virtual void toggleVisibility();
        virtual void ensureWidthFits( const qreal width );

        virtual const QString title() { return m_titleItem->toPlainText(); }

    protected:
        virtual void mousePressEvent( QGraphicsSceneMouseEvent *event );
        virtual void mouseMoveEvent( QGraphicsSceneMouseEvent *event );
        virtual void setBoundingRectSize( const QSizeF &sz );
        void setContentRectSize( const QSizeF &sz, const bool synchroniseHeight = true );
        void ensureTitleCentered();

        QGraphicsTextItem *m_titleItem;
        QGraphicsRectItem *m_titleBarRect;
        QGraphicsRectItem *m_contentRect;

        bool  m_goingUp;
        qreal m_optimumHeight;
        qreal m_animationIncrement;
        QTimeLine *m_animationTimer;

    protected slots:
        void visibilityTimerSlot();
        void animationStateChanged( QTimeLine::State newState );

    signals:
        void heightChanged(qreal change);

    private:
        QGraphicsRectItem* titleBarRect() { return m_titleBarRect; }
        QGraphicsRectItem* contentRect() { return m_contentRect; }
        QGraphicsTextItem* titleItem() { return m_titleItem; }

};

}

#endif

