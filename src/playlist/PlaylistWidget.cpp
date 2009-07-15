/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "PlaylistWidget.h"

#include "ActionClasses.h"
#include "App.h"
#include "Debug.h"
#include "DynamicModel.h"
#include "MainWindow.h"
#include "ToolBar.h"
#include "PaletteHandler.h"
#include "PlaylistController.h"
#include "PlaylistDefines.h"
#include "PlaylistHeader.h"
#include "PlaylistModel.h"
#include "layouts/LayoutManager.h"
#include "widgets/ProgressiveSearchWidget.h"
#include "widgets/HorizontalDivider.h"
#include "layouts/LayoutConfigAction.h"


#include <KToolBarSpacerAction>

#include <QHBoxLayout>
#include <amarokconfig.h>

Playlist::Widget::Widget( QWidget* parent )
        : KVBox( parent )
{
    DEBUG_BLOCK
    setContentsMargins( 0, 0, 0, 0 );

    m_sortWidget = new Playlist::SortWidget( this );
    new HorizontalDivider( this );

    m_searchWidget = new ProgressiveSearchWidget( this );

    //this is really only useful for debugging at the moment, so dont show it to users and testers
    /*m_sortBox = new QComboBox( this );
    m_sortBox->insertItem( 0, "Album", Album);
    m_sortBox->insertItem( 1, "AlbumArtist", Album);
    m_sortBox->insertItem( 2, "Artist", Artist );
    m_sortBox->insertItem( 3, "Bitrate", Bitrate );
    m_sortBox->insertItem( 4, "Bpm", Bpm );
    m_sortBox->insertItem( 5, "Comment", Comment );
    m_sortBox->insertItem( 6, "Composer", Composer );
    m_sortBox->insertItem( 7, "Directory", Directory );
    m_sortBox->insertItem( 8, "DiscNumber", DiscNumber );
    m_sortBox->insertItem( 9, "Filename", Filename );
    m_sortBox->insertItem( 10, "Filesize", Filesize );
    m_sortBox->insertItem( 11, "Genre", Genre );
    m_sortBox->insertItem( 12, "LastPlayed", LastPlayed );
    m_sortBox->insertItem( 13, "Length", Length );
    m_sortBox->insertItem( 14, "Mood", Mood );
    m_sortBox->insertItem( 15, "PlayCount", PlayCount );
    m_sortBox->insertItem( 16, "Rating", Rating );
    m_sortBox->insertItem( 17, "SampleRate", SampleRate );
    m_sortBox->insertItem( 18, "Score", Score );
    m_sortBox->insertItem( 29, "Source", Source );
    m_sortBox->insertItem( 30, "Title", Title );
    m_sortBox->insertItem( 31, "TrackNumber", TrackNumber );
    m_sortBox->insertItem( 32, "Type", Type );
    m_sortBox->insertItem( 33, "Year", Year );

    connect( m_sortBox, SIGNAL( activated( int ) ), this, SLOT( sort( int ) ) );*/

    // show visual indication of dynamic playlists  being enabled
    connect( PlaylistBrowserNS::DynamicModel::instance(), SIGNAL( enableDynamicMode( bool ) ), SLOT( showDynamicHint( bool ) ) );
    m_dynamicHintWidget = new QLabel( i18n( "Dynamic Mode Enabled" ), this );
    m_dynamicHintWidget->setAlignment( Qt::AlignCenter );
    m_dynamicHintWidget->setStyleSheet( QString( "QLabel { background-color: %1; } " ).arg( PaletteHandler::highlightColor().name() ) );
    showDynamicHint( AmarokConfig::dynamicMode() );

    QWidget * layoutHolder = new QWidget( this );

    QVBoxLayout* mainPlaylistlayout = new QVBoxLayout( layoutHolder );
    mainPlaylistlayout->setContentsMargins( 0, 0, 0, 0 );

    m_playlistView = new PrettyListView( this );
    m_playlistView->show();

    connect( m_searchWidget, SIGNAL( filterChanged( const QString &, int, bool ) ), m_playlistView, SLOT( find( const QString &, int, bool ) ) );
    connect( m_searchWidget, SIGNAL( next( const QString &, int ) ), m_playlistView, SLOT( findNext( const QString &, int ) ) );
    connect( m_searchWidget, SIGNAL( previous( const QString &, int ) ), m_playlistView, SLOT( findPrevious( const QString &, int ) ) );
    connect( m_searchWidget, SIGNAL( filterCleared() ), m_playlistView, SLOT( clearSearchTerm() ) );
    connect( m_searchWidget, SIGNAL( showOnlyMatches( bool ) ), m_playlistView, SLOT( showOnlyMatches( bool ) ) );
    connect( m_searchWidget, SIGNAL( activateFilterResult() ), m_playlistView, SLOT( playFirstSelected() ) );
    connect( m_searchWidget, SIGNAL( downPressed() ), m_playlistView, SLOT( setFocus() ) );

    KConfigGroup searchConfig = Amarok::config("Playlist Search");
    m_playlistView->showOnlyMatches( searchConfig.readEntry( "ShowOnlyMatches", false ) );

    connect( m_playlistView, SIGNAL( found() ), m_searchWidget, SLOT( match() ) );
    connect( m_playlistView, SIGNAL( notFound() ), m_searchWidget, SLOT( noMatch() ) );

    connect( LayoutManager::instance(), SIGNAL( activeLayoutChanged() ), m_playlistView, SLOT( reset() ) );

    mainPlaylistlayout->setSpacing( 0 );
    mainPlaylistlayout->addWidget( m_playlistView );

    KHBox *barBox = new KHBox( this );
    barBox->setMargin( 0 );
    barBox->setContentsMargins( 0, 0, 0, 0 );

    
    Amarok::ColoredToolBar *plBar = new Amarok::ColoredToolBar( barBox );
    plBar->setFixedHeight( 30 );
    plBar->setObjectName( "PlaylistToolBar" );

    Model::instance();

    // the Controller ctor creates the undo/redo actions that we use below, so we want
    // to make sure that it's been constructed and the the actions registered
    Controller::instance();

    {
        //START Playlist toolbar
        plBar->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred );
        plBar->setIconDimensions( 22 );
        plBar->setMovable( false );
        plBar->addAction( new KToolBarSpacerAction( this ) );

        plBar->addAction( Amarok::actionCollection()->action( "playlist_clear" ) );

        //FIXME this action should go in ActionController, but we don't have any visibility to the view
        KAction *action = new KAction( KIcon( "music-amarok" ), i18n("Show active track"), this );
        connect( action, SIGNAL( triggered( bool ) ), m_playlistView, SLOT( scrollToActiveTrack() ) );
        plBar->addAction( action );

        plBar->addSeparator();
        plBar->addAction( Amarok::actionCollection()->action( "playlist_undo" ) );
        plBar->addAction( Amarok::actionCollection()->action( "playlist_redo" ) );
        plBar->addSeparator();
        plBar->addAction( Amarok::actionCollection()->action( "playlist_save" ) );

        plBar->addAction( new KToolBarSpacerAction( this ) );
    } //END Playlist Toolbar

    setFrameShape( QFrame::NoFrame );
}

QSize
Playlist::Widget::sizeHint() const
{
    return QSize( static_cast<QWidget*>( parent() )->size().width() / 4 , 300 );
}

/*void
Playlist::Widget::sort( int index )
{
    DEBUG_BLOCK
    int field = m_sortBox->itemData( index ).toInt();
    debug() << "Field: " << field;
    //The::playlistModel()->sort( field );
    FilterProxy::instance()->sort( field );
}*/

void
Playlist::Widget::showDynamicHint( bool enabled )
{
    DEBUG_BLOCK
    if( enabled )
        m_dynamicHintWidget->show();
    else
        m_dynamicHintWidget->hide();

}

