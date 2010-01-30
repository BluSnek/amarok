/****************************************************************************************
 * Copyright (c) 2010 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "FileView.h"

#include "Debug.h"
#include "EngineController.h"
#include "playlist/PlaylistController.h"
#include "PopupDropperFactory.h"
#include "context/ContextView.h"
#include "context/popupdropper/libpud/PopupDropper.h"
#include "context/popupdropper/libpud/PopupDropperItem.h"
#include "PaletteHandler.h"
#include "SvgHandler.h"

#include <KIcon>
#include <KLocale>
#include <KMenu>
#include <KUrl>

#include <QContextMenuEvent>
#include <QFileSystemModel>
#include <QItemDelegate>
#include <QPainter>



class FileViewDelegate : public QItemDelegate
{

public:

    FileViewDelegate( QObject *parent = 0 )
        : QItemDelegate( parent )
    {
    }

    
    virtual void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
    {
        const int width = option.rect.width();
        const int height = option.rect.height();

        if( height > 0 )
        {
            painter->save();
            QPixmap background;

            background = The::svgHandler()->renderSvgWithDividers( "service_list_item", width, height, "service_list_item" );

            painter->drawPixmap( option.rect.topLeft().x(), option.rect.topLeft().y(), background );

            painter->restore();
        }

        QItemDelegate::paint( painter, option, index );
    }

};




FileView::FileView( QWidget * parent )
    : QListView( parent )
    , m_appendAction( 0 )
    , m_loadAction( 0 )
    , m_pd( 0 )
    , m_ongoingDrag( false )
{
    setFrameStyle( QFrame::NoFrame );

     setAlternatingRowColors( true );

    The::paletteHandler()->updateItemView( this );
    connect( The::paletteHandler(), SIGNAL( newPalette( const QPalette & ) ), SLOT( newPalette( const QPalette & ) ) );

    setItemDelegate( new FileViewDelegate( this ) );
    
}

void FileView::contextMenuEvent ( QContextMenuEvent * e )
{

    if( !model() )
        return;

    QModelIndexList indices = selectedIndexes();

    // Abort if nothing is selected
    if( indices.isEmpty() )
        return;

    

    KMenu* menu = new KMenu( this );

    QList<QAction *> actions = actionsForIndices( indices );

    foreach( QAction * action, actions )
        menu->addAction( action );
    
    menu->exec( e->globalPos() );
 
}

void FileView::slotAppendToPlaylist()
{
    addSelectionToPlaylist( false );
}


void FileView::slotReplacePlaylist()
{
    addSelectionToPlaylist( true );
}

QList<QAction *> FileView::actionsForIndices( const QModelIndexList &indices )
{

    QList<QAction *> actions;
    
    if( !indices.isEmpty() )
    {
        if( m_appendAction == 0 )
        {
            m_appendAction = new QAction( KIcon( "media-track-add-amarok" ), i18n( "&Add to Playlist" ), this );
            m_appendAction->setProperty( "popupdropper_svg_id", "append" );
            connect( m_appendAction, SIGNAL( triggered() ), this, SLOT( slotAppendToPlaylist() ) );
        }

        actions.append( m_appendAction );

        if( m_loadAction == 0 )
        {
            m_loadAction = new QAction( i18nc( "Replace the currently loaded tracks with these", "&Replace Playlist" ), this );
            m_loadAction->setProperty( "popupdropper_svg_id", "load" );
            connect( m_loadAction, SIGNAL( triggered() ), this, SLOT( slotReplacePlaylist() ) );
        }

        actions.append( m_loadAction );
    }

    return actions;
}

void FileView::addSelectionToPlaylist( bool replace )
{
    DEBUG_BLOCK
    QModelIndexList indices = selectedIndexes();

    if( indices.count() == 0 )
        return;
    
    QFileSystemModel * fsModel = qobject_cast<QFileSystemModel *>( model() );
    
    if( fsModel )
    {
        QList<KUrl> urls;
        
        foreach( QModelIndex index, indices )
        {
            QString path = fsModel->filePath( index );
            debug() << "file path: " << path;
            if( EngineController::canDecode( path ) || fsModel->isDir( index ) )
            {
                urls << KUrl( path );
            }
        }

        The::playlistController()->insertOptioned( urls, replace ? Playlist::Replace : Playlist::AppendAndPlay );
    }
}


void
FileView::startDrag( Qt::DropActions supportedActions )
{
    DEBUG_BLOCK

    //setSelectionMode( QAbstractItemView::NoSelection );
    // When a parent item is dragged, startDrag() is called a bunch of times. Here we prevent that:
    m_dragMutex.lock();
    if( m_ongoingDrag )
    {
        m_dragMutex.unlock();
        return;
    }
    m_ongoingDrag = true;
    m_dragMutex.unlock();

    if( !m_pd )
        m_pd = The::popupDropperFactory()->createPopupDropper( Context::ContextView::self() );

    if( m_pd && m_pd->isHidden() )
    {
        QModelIndexList indices = selectedIndexes();

        QList<QAction *> actions = actionsForIndices( indices );

        QFont font;
        font.setPointSize( 16 );
        font.setBold( true );

        foreach( QAction * action, actions )
            m_pd->addItem( The::popupDropperFactory()->createItem( action ) );

        m_pd->show();
    }

    QListView::startDrag( supportedActions );
    debug() << "After the drag!";

    if( m_pd )
    {
        debug() << "clearing PUD";
        connect( m_pd, SIGNAL( fadeHideFinished() ), m_pd, SLOT( clear() ) );
        m_pd->hide();
    }

    m_dragMutex.lock();
    m_ongoingDrag = false;
    m_dragMutex.unlock();
}

void FileView::newPalette( const QPalette & palette )
{
    Q_UNUSED( palette )
    The::paletteHandler()->updateItemView( this );
    reset(); // redraw all potential delegates
}

#include "FileView.moc"