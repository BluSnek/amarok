/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "PrettyTreeView.h"

#include "PaletteHandler.h"
#include "SvgHandler.h"
#include "widgets/PrettyTreeRoles.h"
#include "widgets/PrettyTreeDelegate.h"

#include <KGlobalSettings>

#include <QAction>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

Q_DECLARE_METATYPE( QAction* )
Q_DECLARE_METATYPE( QList<QAction*> )

using namespace Amarok;

PrettyTreeView::PrettyTreeView( QWidget *parent )
    : QTreeView( parent )
    , m_expandToggledWhenPressed( false )
{
    setAlternatingRowColors( true );
    setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );

    The::paletteHandler()->updateItemView( this );
    connect( The::paletteHandler(), SIGNAL( newPalette( const QPalette & ) ), SLOT( newPalette( const QPalette & ) ) );

#ifdef Q_WS_MAC
    // for some bizarre reason w/ some styles on mac per-pixel scrolling is slower than
    // per-item
    setVerticalScrollMode( QAbstractItemView::ScrollPerItem );
    setHorizontalScrollMode( QAbstractItemView::ScrollPerItem );
#else
    // Scrolling per item is really not smooth and looks terrible
    setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    setHorizontalScrollMode( QAbstractItemView::ScrollPerPixel );
#endif

    setAnimated( KGlobalSettings::graphicEffectsLevel() != KGlobalSettings::NoEffects );
}

PrettyTreeView::~PrettyTreeView()
{
}

void
PrettyTreeView::edit( const QModelIndex &index )
{
    QTreeView::edit( index );
}

bool
PrettyTreeView::edit( const QModelIndex &index, QAbstractItemView::EditTrigger trigger, QEvent *event )
{
    QModelIndex parent = index.parent();
    while( parent.isValid() )
    {
        expand( parent );
        parent = parent.parent();
    }
    return QAbstractItemView::edit( index, trigger, event );
}

void
PrettyTreeView::drawRow( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    QTreeView::drawRow( painter, option, index );

    const int width = option.rect.width();
    const int height = option.rect.height();

    if( height > 0 )
    {
        QPixmap background = The::svgHandler()->renderSvgWithDividers(
                "service_list_item", width, height, "service_list_item" );

        painter->save();
        painter->drawPixmap( option.rect.topLeft().x(), option.rect.topLeft().y(), background );
        painter->restore();
    }
}

void
PrettyTreeView::mouseMoveEvent( QMouseEvent *event )
{
    QTreeView::mouseMoveEvent( event );

    // Make sure we repaint the item for the collection action buttons
    const QModelIndex index = indexAt( event->pos() );
    const int actionsCount = index.data( PrettyTreeRoles::DecoratorRoleCount ).toInt();
    if( actionsCount )
        update( index );
}

void
PrettyTreeView::mousePressEvent( QMouseEvent *event )
{
    const QModelIndex index = indexAt( event->pos() );

    // Only forward the press event if we aren't on an action (which gets triggered on a release)
    if( event->button() == Qt::LeftButton &&
        event->modifiers() == Qt::NoModifier &&
        decoratorActionAt( index, event->pos() ) )
    {
        event->accept();
        return;
    }

    bool prevExpandState = isExpanded( index );

    // This will toggle the expansion of the current item when clicking
    // on the fold marker but not on the item itself. Required here to
    // enable dragging.
    QTreeView::mousePressEvent( event );

    if( index.isValid() )
        m_expandToggledWhenPressed = ( prevExpandState != isExpanded( index ) );
}

void
PrettyTreeView::mouseReleaseEvent( QMouseEvent *event )
{
    const QModelIndex index = indexAt( event->pos() );

    // if root is decorated, it doesn't show any actions
    QAction *action = rootIsDecorated() ? 0 : decoratorActionAt( index, event->pos() );
    if( action &&
        event->button() == Qt::LeftButton &&
        event->modifiers() == Qt::NoModifier )
    {
        action->trigger();
        event->accept();
        return;
    }

    if( !m_expandToggledWhenPressed &&
        event->button() == Qt::LeftButton &&
        event->modifiers() == Qt::NoModifier &&
        KGlobalSettings::singleClick() &&
        model()->hasChildren( index ) )
    {
        m_expandToggledWhenPressed = !m_expandToggledWhenPressed;
        setExpanded( index, !isExpanded( index ) );
        event->accept();
        return;
    }

    QTreeView::mouseReleaseEvent( event );
}

bool
PrettyTreeView::viewportEvent( QEvent *event )
{
    if( event->type() == QEvent::ToolTip )
    {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>( event );
        const QModelIndex index = indexAt( helpEvent->pos() );
        // if root is decorated, it doesn't show any actions
        QAction *action = rootIsDecorated() ? 0 : decoratorActionAt( index, helpEvent->pos() );
        if( action )
        {
            QToolTip::showText( helpEvent->globalPos(), action->toolTip() );
            event->accept();
            return true;
        }
    }

    return QAbstractItemView::viewportEvent( event );
}

QAction *
PrettyTreeView::decoratorActionAt( const QModelIndex &index, const QPoint &pos )
{
    const int actionsCount = index.data( PrettyTreeRoles::DecoratorRoleCount ).toInt();
    if( actionsCount <= 0 )
        return 0;

    PrettyTreeDelegate* ptd = qobject_cast<PrettyTreeDelegate*>( itemDelegate( index ) );
    if( !ptd )
        return 0;

    QList<QAction *> actions = index.data( PrettyTreeRoles::DecoratorRole ).value<QList<QAction *> >();
    QRect rect = visualRect( index );

    for( int i = 0; i < actions.count(); i++ )
        if( ptd->decoratorRect( rect, i ).contains( pos ) )
            return actions.at( i );

    return 0;
}

void
PrettyTreeView::newPalette( const QPalette & palette )
{
    Q_UNUSED( palette )
    The::paletteHandler()->updateItemView( this );
    reset(); // redraw all potential delegates
}
