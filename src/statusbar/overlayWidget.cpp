/***************************************************************************
 *   Copyright (C) 2005 by Max Howell <max.howell@methylblue.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "overlayWidget.h"
#include "ContextStatusBar.h"

#include <qpoint.h>
//Added by qt3to4:
#include <QEvent>

#include "debug.h"


namespace KDE {


OverlayWidget::OverlayWidget( QWidget *parent, QWidget *anchor, const char* name )
        : QFrame( parent->parentWidget() )
        , m_anchor( anchor )
        , m_parent( parent )
{
    parent->installEventFilter( this );
    setObjectName( name );

    hide();
}

void
OverlayWidget::reposition()
{
    //setMaximumSize( parentWidget()->size() );
    //setMaximumSize( QSize( 200, 100 ) );
    //setMinimumSize( QSize( 200, 100 ) );
    adjustSize();

    // p is in the alignWidget's coordinates
    QPoint p;

   // p.setX( m_anchor->width() - width() );
    p.setX( m_anchor->x() );
    p.setY( m_anchor->y() - height() );

    debug() << "p before: " << p;

    p = m_anchor->mapToGlobal( p );

    debug() << "p after: " << p;

    //// Position in the toplevelwidget's coordinates
    //QPoint pTopLevel = m_anchor->mapTo( topLevelWidget(), p );

    // Position in the widget's parentWidget coordinates
    //QPoint pParent = parentWidget() ->mapFrom( topLevelWidget(), pTopLevel );
    // keep it on the screen
    //if( pParent.x() < 0 )
    //    pParent.rx() = 0;

    // Move 'this' to that position.
    move( p );
}

// bool
// OverlayWidget::eventFilter( QObject* o, QEvent* e )
// {
//     if ( e->type() == QEvent::Move || e->type() == QEvent::Resize )
//         reposition();
//
//     return QFrame::eventFilter( o, e );
// }

// void
// OverlayWidget::resizeEvent( QResizeEvent* ev )
// {
//     reposition();
//     QFrame::resizeEvent( ev );
// }

bool
OverlayWidget::event( QEvent *e )
{
    if ( e->type() == QEvent::ChildInserted )
        adjustSize();

    return QFrame::event( e );
}

}
