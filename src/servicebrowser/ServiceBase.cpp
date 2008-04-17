/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "ServiceBase.h"

#include "Amarok.h"

#include "debug.h"

#include "collection/Collection.h"
#include "TheInstances.h"
#include "SearchWidget.h"
#include "ServiceInfoProxy.h"

#include <khbox.h>
#include <KMenuBar>

#include <QFrame>
#include <QLabel>
#include <QPainter>

#include <QDirModel>

ServiceFactory::ServiceFactory()
{
    CollectionManager::instance()->addTrackProvider( this );
}

ServiceFactory::~ ServiceFactory()
{
    CollectionManager::instance()->removeTrackProvider( this );
}


Meta::TrackPtr ServiceFactory::trackForUrl(const KUrl & url)
{
    if ( m_activeServices.size() == 0 ) {
        debug() << "our service is needed for a url, so init it!";
        init();
    }

    Meta::TrackPtr track;
    
    foreach( ServiceBase * service, m_activeServices ) {

        if (  service->collection() )
            track = service->collection()->trackForUrl( url );

        if ( track )
            return track;
    }

    return track;
}

void ServiceFactory::clearActiveServices()
{
    m_activeServices.clear();
}



ServiceBase *ServiceBase::s_instance = 0;

ServiceBase::ServiceBase( const QString &name )
        : KVBox( 0)
        , m_polished( false )
        , m_infoParser( 0 )
{
    DEBUG_BLOCK

    m_name = name;

    setContentsMargins( 1, 1, 1, 1 );
    setSpacing( 1 );

    m_topPanel = new KVBox( this );

    m_topPanel->setFrameStyle( QFrame::StyledPanel | QFrame::Plain );
    m_topPanel->setLineWidth( 2 );
    m_topPanel->setSpacing( 2 );
    m_topPanel->setMargin( 2 );
    //m_topPanel->setFixedHeight( 50 );

    KHBox * commonPanel = new KHBox ( m_topPanel );

    m_homeButton = new QPushButton( commonPanel );
    m_homeButton->setIcon( KIcon("go-previous-amarok") );
    m_homeButton->setIconSize( QSize( 16, 16 ) );
    m_homeButton->setFixedSize( 28, 28 );
    connect( m_homeButton, SIGNAL( clicked( bool ) ), this, SLOT( homeButtonClicked( ) ) );

    QLabel * nameLabel = new QLabel( commonPanel );
    nameLabel->setMinimumSize( 230 , 28 );
    nameLabel->setText( m_name );
    nameLabel->setFont(QFont("Arial", 12, QFont::Bold));
    nameLabel->setAlignment(Qt::AlignCenter | Qt::AlignHCenter);
   
    m_contentView = new ServiceCollectionTreeView( this );

    m_contentView->setAlternatingRowColors ( true );
    //m_contentView->setAnimated( true );
    m_contentView->setSortingEnabled( true );
    m_contentView->sortByColumn ( 0, Qt::AscendingOrder );

    m_contentView->setDragEnabled ( true );
    m_contentView->setDragDropMode ( QAbstractItemView::DragOnly );

    
    //connect( m_contentView, SIGNAL( pressed ( const QModelIndex & ) ), this, SLOT( treeItemSelected( const QModelIndex & ) ) );
    //connect( m_contentView, SIGNAL( doubleClicked ( const QModelIndex & ) ), this, SLOT( itemActivated ( const QModelIndex & ) ) );

    connect( m_contentView, SIGNAL( itemSelected ( CollectionTreeItem * )  ), this, SLOT( itemSelected( CollectionTreeItem * ) ) );


    m_bottomPanel = new KVBox( this );
    m_bottomPanel->setFixedHeight( 50 );
    m_bottomPanel->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
    m_bottomPanel->setLineWidth(2);
    m_bottomPanel->setSpacing( 2 );
    m_bottomPanel->setMargin( 2 );


    m_filterModel = new QSortFilterProxyModel( this );
    m_filterModel->setSortCaseSensitivity( Qt::CaseInsensitive );
    m_filterModel->setFilterCaseSensitivity( Qt::CaseInsensitive );

    m_menubar = new KMenuBar( m_topPanel );
    m_filterMenu = m_menubar->addMenu( i18n( "Group By" ) );

    m_menubar->hide();

    m_searchWidget = new SearchWidget( m_topPanel );
    m_searchWidget->setup( m_contentView );
}

ServiceBase::~ServiceBase()
{
    delete m_infoParser;
}

QString ServiceBase::getName( )
{
    return m_name;
}

void ServiceBase::setShortDescription( const QString &shortDescription )
{
    m_shortDescription = shortDescription;
}

QString ServiceBase::getShortDescription( )
{
    return m_shortDescription;
}

void ServiceBase::setLongDescription( const QString &longDescription )
{
    m_longDescription = longDescription;
}

QString ServiceBase::getLongDescription( )
{
    return m_longDescription;
}

void ServiceBase::setIcon( const QIcon &icon )
{
    m_icon = icon;
}

QIcon ServiceBase::getIcon( )
{
    return m_icon;
}

void ServiceBase::homeButtonClicked( ) 
{
    emit( home() );
}

void ServiceBase::itemActivated ( const QModelIndex & index )
{
    Q_UNUSED( index );
}


void ServiceBase::setModel( SingleCollectionTreeItemModel * model )
{
    //m_filterModel->setSourceModel( model );
    //m_contentView->setModel( m_filterModel );
    m_contentView->setModel( model );
    m_model  = model;
}

SingleCollectionTreeItemModel * ServiceBase::getModel() {
    return m_model;
}

void ServiceBase::infoChanged ( const QString &infoHtml ) {


    DEBUG_BLOCK

    QVariantMap map;
    map["service_name"] = m_name;
    map["main_info"] = infoHtml;
    The::serviceInfoProxy()->setInfo( map );

}

void ServiceBase::itemSelected( CollectionTreeItem * item )
{
    DEBUG_BLOCK

    Meta::DataPtr ptr = item->data();
    if ( ( ptr.data() == 0 ) || ( m_infoParser == 0 )) return; 

    debug() << "selected item: " << ptr.data()->name();

    ServiceDisplayInfoProvider * infoProvider = dynamic_cast<ServiceDisplayInfoProvider *>( ptr.data() );
    if (infoProvider == 0 ) return; 

    infoProvider->processInfoOf( m_infoParser );
}

void ServiceBase::generateWidgetInfo( const QString &html ) const
{
    QVariantMap map;
    map["service_name"] = m_name;
    map["main_info"] = html;
    The::serviceInfoProxy()->setInfo( map );
}

void ServiceBase::setPlayableTracks(bool playable)
{
    m_contentView->setPlayableTracks( playable );
}

void ServiceBase::sortByArtist()
{
    m_contentView->setLevels( QList<int>() << CategoryId::Artist );
}

void ServiceBase::sortByArtistAlbum()
{
    m_contentView->setLevels( QList<int>() << CategoryId::Artist << CategoryId::Album );
}

void ServiceBase::sortByAlbum()
{
    m_contentView->setLevels( QList<int>() << CategoryId::Album );
}

void ServiceBase::sortByGenreArtist()
{
    m_contentView->setLevels( QList<int>() << CategoryId::Genre << CategoryId::Artist );
}

void ServiceBase::sortByGenreArtistAlbum()
{
    m_contentView->setLevels( QList<int>() << CategoryId::Genre << CategoryId::Artist << CategoryId::Album );
}

void ServiceBase::setFilter(const QString & filter)
{
    polish();
    m_searchWidget->setSearchString( filter );
}

void ServiceBase::setInfoParser(InfoParserBase * infoParser)
{
    m_infoParser = infoParser;
    
    connect ( m_infoParser, SIGNAL( info( QString) ), this, SLOT( infoChanged( QString ) ) );
}

InfoParserBase * ServiceBase::infoParser()
{
    return m_infoParser;
}

QString ServiceBase::messages()
{
    return i18n( "This service does not accept any messages" );
}

QString ServiceBase::sendMessage(const QString & message)
{
    return i18n( "ERROR: unknown message" );
}


#include "ServiceBase.moc"

