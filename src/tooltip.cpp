/*
  Copyright (c) 2006 Gábor Lehel <illissius@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "tooltip.h"

#include "debug.h"

#include <KApplication>
#include <KGlobal>

#include <q3simplerichtext.h>
#include <QApplication>
#include <QCursor>
#include <QEvent>
#include <QFrame>
#include <QPainter>
#include <QPixmap>


class Amarok::ToolTip::Manager: public QObject
{
public:
    Manager( QObject *parent ): QObject( parent ) { kapp->installEventFilter( this ); }
    virtual ~Manager()
    {
        for( int n = Amarok::ToolTip::s_tooltips.count() - 1; n >= 0; --n )
            delete Amarok::ToolTip::s_tooltips[n];
    }

    bool eventFilter( QObject*, QEvent *e )
    {
        switch ( e->type() )
        {
            case QEvent::KeyPress:
            case QEvent::KeyRelease:
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonRelease:
            //case QEvent::MouseMove:
            case QEvent::Wheel:
                ToolTip::hideTips();
                break;
            case QEvent::FocusIn:
            case QEvent::Enter:
            case QEvent::Leave:
            case QEvent::FocusOut:
                if( !dynamic_cast<Amarok::ToolTip*>( kapp->widgetAt( QCursor::pos() ) ) )
                    Amarok::ToolTip::hideTips();
            default: break;
        }

        return false;
    }
};

Amarok::ToolTip::Manager* Amarok::ToolTip::s_manager = 0;
QPoint Amarok::ToolTip::s_pos;
QRect Amarok::ToolTip::s_rect;
QString Amarok::ToolTip::s_text;
QList<Amarok::ToolTip*> Amarok::ToolTip::s_tooltips;
int Amarok::ToolTip::s_hack = 0;

void Amarok::ToolTip::add( ToolTipClient *client, QWidget *parent ) //static
{
    Q_UNUSED( client ); Q_UNUSED( parent );
    //if( !s_manager )
    //    s_manager = new Amarok::ToolTip::Manager( kapp );
    //new ToolTip( client, parent );
}

void Amarok::ToolTip::remove( QWidget *widget ) //static
{
    Q_UNUSED( widget );
    //for( int i = s_tooltips.count() - 1; i >= 0; --i )
    //    if( s_tooltips[i]->QToolTip::parentWidget() == widget )
    //        delete s_tooltips[i];
}

void Amarok::ToolTip::hideTips() //static
{
    //for( int i = 0, n = s_tooltips.count(); i < n; ++i )
    //    s_tooltips[i]->hideTip();
    //QToolTip::hide();
}

QString Amarok::ToolTip::textFor( QWidget *widget, const QPoint &pos ) //static
{
    Q_UNUSED( widget ); Q_UNUSED( pos );
    /*for( int i = 0, n = s_tooltips.count(); i < n; ++i )
        if( s_tooltips[i]->QToolTip::parentWidget() == widget )
            return s_tooltips[i]->m_client->toolTipText( widget, pos ).first;
    return QToolTip::textFor( widget, pos );*/
    return QString();
}

void Amarok::ToolTip::updateTip() //static
{
    for( int i = 0, n = s_tooltips.count(); i < n; ++i )
        if( s_tooltips[i]->isVisible() )
        {
            /*QWidget* const w = s_tooltips[i]->QToolTip::parentWidget();
            QPair<QString, QRect> p = s_tooltips[i]->m_client->toolTipText( w, w->mapFromGlobal( s_pos ) );
            QString prev = s_text;
            if( prev != p.first )
            {
                s_text = p.first;
                s_rect = p.second;
                s_tooltips[i]->showTip();
            }*/
            break;
        }
}

Amarok::ToolTip::ToolTip( ToolTipClient *client, QWidget *parent )
    : QFrame(0, Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint
                    | Qt::X11BypassWindowManagerHint ),
//      QToolTip( ),
      m_client( client )
{
    s_tooltips.append( this );
    QFrame::setPalette( QToolTip::palette() );
    connect( &m_timer, SIGNAL( timeout() ), this, SLOT( hideTip() ) );
}

Amarok::ToolTip::~ToolTip()
{
    s_tooltips.removeAll( this );
}

void Amarok::ToolTip::maybeTip( const QPoint &pos )
{
    /*s_pos = QToolTip::parentWidget()->mapToGlobal( pos );
    QString prev = s_text;
    QPair<QString, QRect> p = m_client->toolTipText( QToolTip::parentWidget(), pos );
    s_text = p.first;
    s_rect = p.second;
    if( QToolTip::parentWidget() && !s_text.isEmpty() )
    {
        if( s_text != prev )
            hideTips();
        showTip();
    }
    else
        hideTips();*/
}

void Amarok::ToolTip::showTip()
{
    m_timer.setSingleShot( true );
    m_timer.start( 15000 );
    if( !isVisible() || sizeHint() != size() )
    {
        resize( sizeHint() );
        position();
    }
    if( !isVisible() )
        show();
    else
        update();
}

void Amarok::ToolTip::hideTip()
{
    if( !isVisible() )
        return;
    QFrame::hide();
    //QToolTip::parentWidget()->update();
    m_timer.stop();
    s_hack = 0;
}

void Amarok::ToolTip::drawContents( QPainter *painter )
{
    /*QPixmap buf( width(), height() );
    QPainter p( &buf );
    buf.fill( colorGroup().background() );

    p.setPen( colorGroup().foreground() );
    p.drawRect( buf.rect() );

    Q3SimpleRichText text( s_text, QToolTip::parentWidget()->font() );
    text.setWidth( width() );
    p.translate( 0, height() / 2 - text.height() / 2);
    QPoint pos = s_rect.isNull() ? QPoint(2, -1)
               : s_hack == 1     ? QPoint(4, -2)
               : QPoint(2, -2); //HACK positioning
    p.setFont( QToolTip::parentWidget()->font() );
    text.draw( &p, pos.x(), pos.y(), rect(), colorGroup() );

    painter->drawPixmap( 0, 0, buf );*/
}

QSize Amarok::ToolTip::sizeHint() const
{
    if( !s_rect.isNull() )
        return s_rect.size();
    else
    {
        //Q3SimpleRichText text( s_text, QToolTip::parentWidget()->font() );
        //text.setWidth( 500 ); //is this reasonable?
        //return QSize( text.widthUsed() - 2, text.height() );
        return QSize(0,0);
    }
}

void Amarok::ToolTip::position()
{
    /*const QRect drect = QApplication::desktop()->availableGeometry( QToolTip::parentWidget() );
    const QSize size = sizeHint();
    const int width = size.width(), height = size.height();
    QPoint pos;
    if( !s_rect.isNull() )
    {
        pos = s_rect.topLeft();
        if( pos.y() + height > drect.bottom() )
            pos.setY( qMax( drect.top(), drect.bottom() - height ) );
        if( pos.x() + width > drect.right() )
            pos.setX( qMax( drect.left(), drect.right() - width ) );
    }
    else
    {
        const QRect r = QRect( QToolTip::parentWidget()->mapToGlobal( QToolTip::parentWidget()->pos() ), QToolTip::parentWidget()->size() );
        pos = r.bottomRight();
        if( pos.y() + height > drect.bottom() )
            pos.setY( qMax( drect.top(), r.top() - height ) );
        if( pos.x() + width > drect.right() )
            pos.setX( qMax( drect.left(), r.left() - width ) );
    }

    move( pos );*/
}

#include "tooltip.moc"
