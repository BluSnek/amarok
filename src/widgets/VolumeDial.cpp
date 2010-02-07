/****************************************************************************************
* Copyright (c) 2009 Thomas Luebking <thomas.luebking@web.de>                          *
*                                                                                      *
* This program is free software; you can redistribute it and/or modify it under        *
* the terms of the GNU General Public License as published by the Free Software        *
* Foundation; either version 2 of the License, or (at your option) any later           *
* version.                                                                             *
*                                                                                      *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
* PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
*                                                                                      *
* You should have received a copy of the GNU General Public License along with         *
* this program.  If not, see <http://www.gnu.org/licenses/>.                           *
****************************************************************************************/

#include "VolumeDial.h"

#include "MainWindow.h"
#include "SvgHandler.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QToolBar>
#include <QToolTip>

#include <KLocale>


VolumeDial::VolumeDial( QWidget *parent ) : QDial( parent )
    , m_isClick( false )
    , m_isDown( false )
    , m_muted( false )
{
    m_anim.step = 0;
    m_anim.timer = 0;
    connect ( this, SIGNAL( valueChanged(int) ), SLOT( valueChangedSlot(int) ) );
}


void VolumeDial::enterEvent( QEvent * )
{
    startFade();
}

// NOTICE: we intercept wheelEvents for ourself to prevent the tooltip hiding on them,
// see ::wheelEvent()
// this is _NOT_ redundant to the code in MainToolbar.cpp
bool VolumeDial::eventFilter( QObject *o, QEvent *e )
{
    if ( e->type() == QEvent::Wheel )
    {
        if ( o == this || QToolTip::text() == m_toolTip )
        {
            QWheelEvent *wev = static_cast<QWheelEvent*>(e);
            if ( o != this )
            {
                QPoint pos( 0, 0 ); // the event needs to be on us or nothing will happen
                QWheelEvent nwev( pos, mapToGlobal( pos ), wev->delta(), wev->buttons(), wev->modifiers() );
                wheelEvent( &nwev );
            }
            else
                wheelEvent( wev );
            return true;
        }
        else // we're not needed anymore
            qApp->removeEventFilter( this );
    }
    return false;
}

void VolumeDial::leaveEvent( QEvent * )
{
    startFade();
}

void VolumeDial::mousePressEvent( QMouseEvent *me )
{
    if ( me->button() == Qt::LeftButton )
    {
        setCursor( Qt::PointingHandCursor );
        const int dx = width()/4;
        const int dy = height()/4;
        m_isClick = rect().adjusted(dx, dy, -dx, -dy).contains( me->pos() );
    }
//     if ( !m_isClick )
    QDial::mousePressEvent( me );
}

void VolumeDial::mouseReleaseEvent( QMouseEvent *me )
{
    if ( me->button() != Qt::LeftButton )
        return;
    
    setCursor( Qt::ArrowCursor );
    if ( !m_isClick )
    {
        QDial::mouseReleaseEvent( me );
        return;
    }
    const int dx = width()/4;
    const int dy = height()/4;
    m_isClick = rect().adjusted(dx, dy, -dx, -dy).contains( me->pos() );
    if ( !m_isClick )
    {
        QDial::mouseReleaseEvent( me );
        return;
    }

    m_isClick = false;

    emit muteToggled( !m_muted );
}

static QColor mix( const QColor &c1, const QColor &c2 )
{
    QColor c;
    c.setRgb( ( c1.red() + c2.red() ) / 2, ( c1.green() + c2.green() ) / 2,
              ( c1.blue() + c2.blue() ) / 2, ( c1.alpha() + c2.alpha() ) / 2 );
    return c;
}


void VolumeDial::paintEvent( QPaintEvent * )
{
    QPainter p( this );
    int icon = m_muted ? 0 : 3;
    if (icon && value() < 66)
        icon = value() < 33 ? 1 : 2;
    p.drawPixmap(0,0, m_icon[ icon ]);
    QColor c = mix( palette().color( foregroundRole() ), palette().color( QPalette::Highlight ) );
    c.setAlpha( 82 + m_anim.step*96/6 );
    p.setPen( QPen( c, 3, Qt::SolidLine, Qt::RoundCap ) );
    p.setRenderHint(QPainter::Antialiasing);
    p.drawArc( rect().adjusted(4,4,-4,-4), -110*16, - value()*320*16 / (maximum() - minimum()) );
    p.end();
}

void VolumeDial::resizeEvent( QResizeEvent *re )
{
    if ( width() != height() )
        resize( height(), height() );
    else
        QDial::resizeEvent( re );

    m_icon[0] = The::svgHandler()->renderSvg( "Muted",      width(), height(), "Muted" );
    m_icon[1] = The::svgHandler()->renderSvg( "Volume_low", width(), height(), "Volume_low" );
    m_icon[2] = The::svgHandler()->renderSvg( "Volume_mid", width(), height(), "Volume_mid" );
    m_icon[3] = The::svgHandler()->renderSvg( "Volume",     width(), height(), "Volume" );
    
    update();
}

void VolumeDial::startFade()
{
    if ( m_anim.timer )
        killTimer( m_anim.timer );
    m_anim.timer = startTimer( 40 );
}

void VolumeDial::stopFade()
{
    killTimer( m_anim.timer );
    m_anim.timer = 0;
    if ( m_anim.step < 0 )
        m_anim.step = 0;
    else if ( m_anim.step > 6 )
        m_anim.step = 6;
}

void VolumeDial::timerEvent( QTimerEvent *te )
{
    if ( te->timerId() != m_anim.timer )
        return;
    if ( underMouse() ) // fade in
    {
        m_anim.step += 2;
        if ( m_anim.step > 5 )
            stopFade();
    }
    else // fade out
    {
        --m_anim.step;
        if ( m_anim.step < 1 )
            stopFade();
    }
    repaint();
}

void VolumeDial::wheelEvent( QWheelEvent *wev )
{
    QDial::wheelEvent( wev );
    wev->accept();

    // NOTICE: this is a bit tricky.
    // the ToolTip "QTipLabel" just installed a global eventfilter that intercepts various
    // events and hides itself on them. Therefore every odd wheelevent will close the tip
    // ("works - works not - works - works not - ...")
    // so we post-install our own global eventfilter to handle wheel events meant for us bypassing
    // the ToolTip eventfilter

    // first remove to prevent multiple installations but ensure we're on top of the ToolTip filter
    qApp->removeEventFilter( this );
    // it's ultimately removed in the timer triggered ::hideToolTip() slot
    qApp->installEventFilter( this );
}

void VolumeDial::setMuted( bool mute )
{
    if ( mute == m_muted )
        return;

    if ( mute )
    {
        m_unmutedValue = value();
        setValue( minimum() );
        m_toolTip = i18n( "Muted" );
    }
    else
    {
        setValue( m_unmutedValue );
        m_toolTip = QString( "Volume: %1 %" ).arg( value() );
    }
    setToolTip( m_toolTip );
}

QSize VolumeDial::sizeHint() const
{
    if ( QToolBar *toolBar = qobject_cast<QToolBar*>( parentWidget() ) )
        return toolBar->iconSize();

    return QDial::sizeHint();
}

void VolumeDial::valueChangedSlot( int v )
{
    m_toolTip = QString( "Volume: %1 %" ).arg( value() );
    setToolTip( m_toolTip );

    if( The::mainWindow()->isReallyShown() )
        QToolTip::showText( mapToGlobal( rect().bottomLeft() ), m_toolTip );
    
    m_isClick = false;

    if ( m_muted == ( v == minimum() ) )
        return;
    m_muted = ( v == minimum() );

    update();
}

#include "VolumeDial.moc"
