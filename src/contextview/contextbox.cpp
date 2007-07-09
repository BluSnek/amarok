/***************************************************************************
 * copyright     : (C) 2007 Seb Ruiz <ruiz@kde.org>                        *
 *                 (C) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com> *
 *                 (C) 2007 Jeff Mitchell <kde-dev@emailgoeshere.com>      *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "contextbox.h"
#include "contextview.h"
#include "debug.h"

#include <QGraphicsItemAnimation>
#include <QGraphicsTextItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QRectF>

using namespace Context;

ContextBox::ContextBox( QGraphicsItem *parent, QGraphicsScene *scene )
    : QObject()
    , QGraphicsRectItem( parent, scene )
    , m_titleItem( 0 )
    , m_goingUp( false )
    , m_optimumHeight( 0 )
    , m_animationTimer( 0 )
{
    setFlag( QGraphicsItem::ItemClipsChildrenToShape, true );
    setFlag( QGraphicsItem::ItemIsMovable, true );

    //this prohibits the use of multiple views for the items, but something we can resolve if we ever need this functionality
    ContextView *cv = ContextView::instance();
    const qreal viewWidth = cv->width();

    static const qreal padding = ContextView::BOX_PADDING;
    const qreal boxWidth = viewWidth - padding*2; // twice the padding for left and right sides

    const QRectF boundingRect = QRectF( 0, 0, boxWidth, 200 );
    setRect( boundingRect );

    setPen( QPen( Qt::black, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin ) );

    m_titleBarRect = new QGraphicsRectItem( this, scene );

    m_titleItem = new QGraphicsTextItem( "", m_titleBarRect, scene );
    m_titleItem->setDefaultTextColor( QColor( 255, 255, 255 ) );
    // increase the font size for the title
    QFont font = m_titleItem->font();
    font.setPointSize( 12 );
    font.setBold( true );
    m_titleItem->setFont( font );

    m_titleBarRect->setRect( 0, 0, boundingRect.width(), m_titleItem->boundingRect().height() );
    m_titleBarRect->setPos( 0, 0 );
    m_titleBarRect->setPen( Qt::NoPen );

    QLinearGradient titleGradient(QPointF( 0, 0 ), QPointF( 0, m_titleBarRect->boundingRect().height() ) );
    titleGradient.setColorAt( 0, QColor( 200, 200, 255 ) );
    titleGradient.setColorAt( 1, QColor( 50, 50, 255 ) );

    m_titleBarRect->setBrush( QBrush( titleGradient ) );

    m_contentRect = new QGraphicsRectItem( this, scene );
    m_contentRect->setRect( 0, 0, boundingRect.width(), boundingRect.height() - m_titleBarRect->boundingRect().height() );
    m_contentRect->setPos( 0, m_titleBarRect->boundingRect().height() );
    m_contentRect->setPen( Qt::NoPen );

    //make a nice shadow
    QLinearGradient shadowGradient( QPointF( 0, 0 ), QPointF( 0, 10) );
    shadowGradient.setColorAt( 0, QColor( 150, 150, 150 ) );
    shadowGradient.setColorAt( 1, QColor( 255, 255, 255 ) );
    m_contentRect->setBrush( QBrush( shadowGradient ) );

    m_optimumHeight = m_contentRect->rect().height();
}

void ContextBox::setTitle( const QString &title )
{
    m_titleItem->setPlainText( title );
    ensureTitleCentered();
}

void ContextBox::ensureTitleCentered()
{
    //FIXME: we need to break the title into multiple lines if the width is now too long
    const qreal titleWidth = m_titleItem->boundingRect().width();

    // Center the title
    const qreal xOffset = ( m_titleBarRect->boundingRect().width() - titleWidth ) / 2.0;
    m_titleItem->setPos( xOffset, 0.0 );
}

void ContextBox::setBoundingRectSize( const QSizeF &sz )
{
    prepareGeometryChange();

    QRectF newRect = QRectF( 0, 0, sz.width(), sz.height() );
    setRect( newRect );
}

void ContextBox::setContentRectSize( const QSizeF &sz, const bool synchroniseHeight )
{
    prepareGeometryChange();

    m_contentRect->setRect( QRectF( 0, 0, sz.width(), sz.height() ) );
    //set correct size of this as well
    setRect( QRectF( 0, 0, sz.width(), sz.height() +  m_titleBarRect->boundingRect().height()) );
    m_titleBarRect->setRect( 0, 0, sz.width(), m_titleBarRect->boundingRect().height() );
    ensureTitleCentered();

    if( synchroniseHeight )
        m_optimumHeight = sz.height();
}

void ContextBox::ensureWidthFits( const qreal width )
{
    const qreal padding = ContextView::BOX_PADDING * 2;
    const qreal height  = m_contentRect->boundingRect().height();

    QSizeF newSize = QSizeF( width - padding, height );
    setContentRectSize( newSize, false );
}

void ContextBox::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
    if( event->buttons() & Qt::LeftButton ) // only handle left button clicks for now
    {
        QPointF pressPoint = event->buttonDownScenePos( Qt::LeftButton );
        QPointF pressPointLocal = m_titleBarRect->mapFromScene( pressPoint );
        if( m_titleBarRect->contains( pressPointLocal ) )
        {
           event->accept();
           toggleVisibility();
        }

    } else {
        event->ignore();
    }
    QGraphicsItem::mousePressEvent( event );
}

// With help from QGraphicsView::mouseMoveEvent()
void ContextBox::mouseMoveEvent( QGraphicsSceneMouseEvent *event )
{
    if ((event->buttons() & Qt::LeftButton) && (flags() & ItemIsMovable))
    {
        if( (flags() & ItemIsMovable) && (!parentItem() || !parentItem()->isSelected()) )
        {
            QPointF diff = mapToParent(event->pos()) - mapToParent(event->lastPos());

            moveBy( 0, diff.y() );

            if( flags() & ItemIsSelectable )
                setSelected( true );
        }
    }
    else
    {
        QGraphicsItem::mouseMoveEvent( event );
    }
}

void ContextBox::toggleVisibility()
{
    const qreal desiredHeight = m_goingUp ? m_optimumHeight : 0;
    const qreal change = desiredHeight - m_contentRect->rect().height();

    m_goingUp = !m_goingUp;

    foreach( QGraphicsItem *child, m_contentRect->children() )
    {
        m_goingUp ? child->hide() : child->show();
    }
    setContentRectSize( QSizeF( m_contentRect->rect().width(), desiredHeight ), false );

    emit heightChanged( change );
    /*
    static const int range = 100;

    if( !m_animationTimer )
    {
        m_animationTimer = new QTimeLine( 500 );
        m_animationTimer->setUpdateInterval( 30 ); // ~33 fps
        m_animationTimer->setFrameRange( 0, range );
        m_animationTimer->setLoopCount( 0 ); // loop forever until we explicitly stop it

        connect( m_animationTimer, SIGNAL( frameChanged(int) ), SLOT( visibilityTimerSlot() ) );
        connect( m_animationTimer, SIGNAL( stateChanged( QTimeLine::State ) ), SLOT( animationStateChanged( QTimeLine::State ) ) );

    }

    if( m_animationTimer->state() == QTimeLine::Running )
    {
        m_goingUp = !m_goingUp; // change direction if the is already an animation
        return;
    }

    m_animationTimer->setStartFrame( 0 );

    m_animationIncrement = m_optimumHeight / range;
    m_animationTimer->start();
    */
}

void ContextBox::visibilityTimerSlot()
{
    const qreal desiredHeight = m_goingUp ? m_optimumHeight : 0;

    qreal change = m_goingUp ? m_animationIncrement : -m_animationIncrement;

    qreal newHeight = m_contentRect->rect().height() + change;

    if( ( !m_goingUp && newHeight <= desiredHeight ) ||
        (  m_goingUp && newHeight >= desiredHeight ) )
    {
        newHeight = desiredHeight;
        m_animationTimer->stop(); //stop the timeline _before_ changing the direction
    }

    setContentRectSize( QSizeF( m_contentRect->rect().width(), newHeight ), false );
    emit heightChanged( change );
}

void ContextBox::animationStateChanged( QTimeLine::State newState )
{
    if( newState == QTimeLine::NotRunning )
        m_goingUp = !m_goingUp;
}

#include "contextbox.moc"
