/*
    Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "DynamicModel.h"

#include "Bias.h"
#include "BiasedPlaylist.h"
#include "BlockingQuery.h"
#include "Collection.h"
#include "CollectionManager.h"
#include "MetaQueryMaker.h"
#include "Debug.h"
#include "DynamicPlaylist.h"
#include "RandomPlaylist.h"
#include "collection/support/XmlQueryReader.h"
#include "collection/support/XmlQueryWriter.h"

#include <QFile>
#include <QMutexLocker>
#include <QVariant>


void
Amarok::setDynamicPlaylist( const QString& title )
{
    PlaylistBrowserNS::DynamicModel::instance()->changePlaylist(
            PlaylistBrowserNS::DynamicModel::instance()->playlistIndex( title ) );
}

void
Amarok::enableDynamicMode( bool enable )
{
    PlaylistBrowserNS::DynamicModel::instance()->enable( enable );
}


PlaylistBrowserNS::DynamicModel* PlaylistBrowserNS::DynamicModel::s_instance = 0;

PlaylistBrowserNS::DynamicModel*
PlaylistBrowserNS::DynamicModel::instance()
{
    if( s_instance == 0 ) s_instance = new DynamicModel();

    return s_instance;
}



PlaylistBrowserNS::DynamicModel::DynamicModel()
    : QAbstractItemModel()
    , m_activeUnsaved(false)
    , m_universeCurrent(false)
{
    Dynamic::DynamicPlaylistPtr randomPlaylist = createDefaultPlaylist();

    insertPlaylist( randomPlaylist );
    m_playlistElements.append( QDomElement() );
    m_activePlaylist = m_defaultPlaylist = 0;


    connect( CollectionManager::instance(),
            SIGNAL(collectionDataChanged(Collection*)),
            SLOT(universeNeedsUpdate()) );
}

PlaylistBrowserNS::DynamicModel::~DynamicModel()
{
}

void
PlaylistBrowserNS::DynamicModel::enable( bool enable )
{
    emit enableDynamicMode( enable );    
}

void
PlaylistBrowserNS::DynamicModel::changePlaylist( int i )
{
    emit changeActive( qMax( 0, i ) );
}


Dynamic::DynamicPlaylistPtr
PlaylistBrowserNS::DynamicModel::createDefaultPlaylist()
{
    return Dynamic::DynamicPlaylistPtr(
        new Dynamic::BiasedPlaylist( "Random Playlist", QList<Dynamic::Bias*>() ) );
}

void
PlaylistBrowserNS::DynamicModel::insertPlaylist( Dynamic::DynamicPlaylistPtr playlist )
{
    // TODO: does one already exist with that name ?

    m_playlistHash[ playlist->title() ] = playlist;
    m_playlistList.append( playlist );
}

Dynamic::DynamicPlaylistPtr
PlaylistBrowserNS::DynamicModel::setActivePlaylist( const QString& name )
{
    // TODO: how does this deal with unsaved ?
    //
    Dynamic::DynamicPlaylistPtr p = m_playlistHash[ name ];
    if( p == Dynamic::DynamicPlaylistPtr() )
        debug() << "Failed to retrive biased playlist: " << name;
    m_activePlaylist = m_playlistList.indexOf( p );

    emit activeChanged();

    return p;
}

Dynamic::DynamicPlaylistPtr
PlaylistBrowserNS::DynamicModel::setActivePlaylist( int index )
{
    // The modified playlist is always the last one in the list,
    // so remove it if there is one.
    if( m_activeUnsaved && index !=  m_playlistList.size()-1 )
    {
        beginRemoveRows( QModelIndex(), m_playlistList.size()-1, m_playlistList.size()-1 );
        m_playlistList.pop_back();
        m_activeUnsaved = false;
        endRemoveRows();
    }

    Dynamic::DynamicPlaylistPtr p = m_playlistList[ index ];
    if( p == Dynamic::DynamicPlaylistPtr() )
        debug() << "Failed to retrive biased playlist: " << index;
    m_activePlaylist = index;

    emit activeChanged();

    return p;
}

Dynamic::DynamicPlaylistPtr
PlaylistBrowserNS::DynamicModel::activePlaylist()
{
    return m_playlistList[m_activePlaylist];
}



Dynamic::DynamicPlaylistPtr
PlaylistBrowserNS::DynamicModel::defaultPlaylist()
{
    return m_playlistList[m_defaultPlaylist];
}

int
PlaylistBrowserNS::DynamicModel::playlistIndex( const QString& title ) const
{
    for( int i = 0; i < m_playlistList.size(); ++i )
    {
        if( m_playlistList[i]->title() == title )
            return i;
    }

    return -1;
}


QModelIndex 
PlaylistBrowserNS::DynamicModel::index( int row, int column, const QModelIndex& parent ) const
{
    Q_UNUSED(parent)
    if( rowCount() <= row ) return QModelIndex();

    return createIndex( row, column, (void*)m_playlistList[row].data() );
}

bool
PlaylistBrowserNS::DynamicModel::isActiveUnsaved() const
{
    return m_activeUnsaved;
}

bool
PlaylistBrowserNS::DynamicModel::isActiveDefault() const
{
    return m_activePlaylist == m_defaultPlaylist;
}


QVariant 
PlaylistBrowserNS::DynamicModel::data ( const QModelIndex & i, int role ) const
{
    if( !i.isValid() ) return QVariant();

    Dynamic::DynamicPlaylistPtr item = m_playlistList[i.row()];


    switch( role )
    {
        case Qt::UserRole:
            return QVariant::fromValue( item );
        case Qt::DisplayRole:
        case Qt::EditRole:
            if( i.row() == m_activePlaylist && m_activeUnsaved )
                return item->title() + " (" + i18n("modified") + ")";
            else
                return item->title();
        default:
            return QVariant();
    }
}

QModelIndex
PlaylistBrowserNS::DynamicModel::parent( const QModelIndex& i ) const
{
    Q_UNUSED(i)
    return QModelIndex();
}

int
PlaylistBrowserNS::DynamicModel::rowCount( const QModelIndex& ) const
{
    return m_playlistList.size();
}


int
PlaylistBrowserNS::DynamicModel::columnCount( const QModelIndex& ) const
{
    return 1;
}

void
PlaylistBrowserNS::DynamicModel::loadPlaylists()
{
    DEBUG_BLOCK

    QFile file( Amarok::saveLocation() + "dynamic.xml" );
    if( !file.open( QIODevice::ReadWrite ) )
    {
        error() << "Can't open dynamic.xml";
        return;
    }

    QString errorMsg;
    if( !m_savedPlaylists.setContent( &file, &errorMsg ) )
    {
        error() << "Can't parse dynamic.xml";
        error() << errorMsg;
        return;
    }

    for( int i = 0; i < m_savedPlaylists.childNodes().size(); ++ i )
    {
        if( !m_savedPlaylists.childNodes().at(i).isElement() )
            continue;

        QDomElement e = m_savedPlaylists.childNodes().at(i).toElement();
        if( e.tagName() == "biasedPlaylist" )
        {
            insertPlaylist( Dynamic::DynamicPlaylistPtr( Dynamic::BiasedPlaylist::fromXml(e) ) );
            m_playlistElements.append( e );
        }
    }
}


const Meta::TrackList&
PlaylistBrowserNS::DynamicModel::universe()
{
    if( !m_universeCurrent )
        computeUniverseSet();

    QMutexLocker locker(&m_universeMutex);
    return m_universe;
}

void
PlaylistBrowserNS::DynamicModel::playlistModified( Dynamic::BiasedPlaylistPtr p )
{
    DEBUG_BLOCK

    // this shouldn't happen
    if( p != m_playlistList[m_activePlaylist] )
    {
        error() << "Non-active playlist changed somehow.";
        return;
    }

    if( m_activeUnsaved )
        return;

    // spawn an unmodified version, put it in the current slot,
    // and add the modified version to the end
    Dynamic::DynamicPlaylistPtr unmodified;

    if( m_activePlaylist == m_defaultPlaylist )
        unmodified = createDefaultPlaylist();
    else
        unmodified = Dynamic::BiasedPlaylist::fromXml( m_playlistElements[m_activePlaylist] );


    Dynamic::DynamicPlaylistPtr modified = m_playlistList[m_activePlaylist];


    m_playlistList[m_activePlaylist] = unmodified;
    m_playlistHash[unmodified->title()] = unmodified;

    beginInsertRows( QModelIndex(), m_playlistList.size(), m_playlistList.size() );
    m_playlistList.append( modified );

    m_activeUnsaved = true;

    endInsertRows();

    emit changeActive( m_playlistList.size() - 1 );
}


void
PlaylistBrowserNS::DynamicModel::saveActive( const QString& newTitle )
{
    DEBUG_BLOCK

    if( m_activeUnsaved )
    {
        m_playlistList.back()->setTitle( newTitle );

        QDomElement e = m_playlistList[m_activePlaylist]->xml();
        m_savedPlaylists.appendChild( e );

        // REPLACE PLAYLIST
        if( m_playlistHash.contains( newTitle ) )
        {
            int index = m_playlistList.indexOf( m_playlistHash[ newTitle ] );
            m_playlistList[index] = m_playlistList.last();
            m_playlistHash[newTitle]  = m_playlistList.last();
            m_savedPlaylists.removeChild( m_playlistElements[index] );
            m_playlistElements[index] = e;

            m_activeUnsaved = false;

            emit changeActive( index );


            beginRemoveRows( QModelIndex(), m_playlistList.size()-1, m_playlistList.size()-1 );
            m_playlistList.pop_back();
            endRemoveRows();
        }
        // NEW PLAYLIST
        else
        {
            m_playlistHash[newTitle] = m_playlistList.last();
            m_playlistElements.append( e );

            m_activeUnsaved = false;

            QModelIndex last = index( m_playlistList.size()-1, 0 );
            emit dataChanged( last, last );
            emit changeActive( m_playlistList.size()-1 );
        }
    }
    else
    {
        // TRYING TO SAVE UNMODIFIED PLAYLIST
        if( m_playlistHash.contains( newTitle ) )
        {
            return;
        }
        // COPY PLAYLIST
        else
        {
            Dynamic::DynamicPlaylistPtr copy;

            if( m_activePlaylist == m_defaultPlaylist )
                copy = createDefaultPlaylist();
            else
                copy = Dynamic::BiasedPlaylist::fromXml( m_playlistElements[m_activePlaylist] );

            copy->setTitle( newTitle );

            insertPlaylist( copy );
            m_playlistElements.append( copy->xml() );
            m_savedPlaylists.appendChild( m_playlistElements.back() );

            emit changeActive( m_playlistList.size()-1 );
        }
    }

    QFile file( Amarok::saveLocation() + "dynamic.xml" );
    if( !file.open( QIODevice::WriteOnly ) )
    {
        error() << "Can't open dynamic.xml.";
        return;
    }

    QTextStream stream( &file );
    m_savedPlaylists.save( stream, 0 );
}


void
PlaylistBrowserNS::DynamicModel::universeNeedsUpdate()
{
    m_universeCurrent = false;
}


void
PlaylistBrowserNS::DynamicModel::computeUniverseSet()
{
    DEBUG_BLOCK
    if( !m_universeMutex.tryLock() )
        return;

    m_universe.clear();

    QueryMaker* qm = new MetaQueryMaker( CollectionManager::instance()->queryableCollections() );
    qm->orderByRandom();
    qm->setQueryType( QueryMaker::Track );

    BlockingQuery bq( qm );

    debug() << "Starting query for universe.";

    bq.startQuery();

    debug() << "Finished query for universe.";

    QHash<QString, Meta::TrackList> trackLists = bq.tracks();

    int size = 0;
    foreach( Meta::TrackList ts, trackLists )
    {
        size += ts.size();
    }

    debug() << "Results from: " << trackLists.keys();

    foreach( Meta::TrackList ts, trackLists )
        m_universe += ts;

    m_universeCurrent = true;
    m_universeMutex.unlock();
}

