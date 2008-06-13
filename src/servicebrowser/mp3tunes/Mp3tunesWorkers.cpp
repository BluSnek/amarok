/***************************************************************************
 *   Copyright (c) 2007  Casey Link <unnamedrambler@gmail.com>             *
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

#include "Mp3tunesWorkers.h"

#include "Mp3tunesMeta.h"
#include "Debug.h"

#include <QStringList>

Mp3tunesLoginWorker::Mp3tunesLoginWorker( Mp3tunesLocker* locker, const QString & username, const QString & password ) : ThreadWeaver::Job()
{
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( completeJob() ) );
    m_locker = locker;
    m_username = username;
    m_password = password;
    m_sessionId = QString();
}

Mp3tunesLoginWorker::~Mp3tunesLoginWorker()
{
}

void Mp3tunesLoginWorker::run()
{
    DEBUG_BLOCK
    if(m_locker != 0) {
        debug() << "Calling Locker login..";
        m_sessionId = m_locker->login(m_username, m_password);
        debug() << "Login Complete. SessionId = " << m_sessionId;
    } else {
        debug() << "Locker is NULL";
    }
}

void Mp3tunesLoginWorker::completeJob()
{
    DEBUG_BLOCK
    debug() << "Login Job complete";
    emit( finishedLogin( m_sessionId ) );
    deleteLater();
}
/* ARTIST FETCHER */
Mp3tunesArtistFetcher::Mp3tunesArtistFetcher( Mp3tunesLocker * locker )
{
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( completeJob() ) );
    m_locker = locker;
}

Mp3tunesArtistFetcher::~Mp3tunesArtistFetcher()
{
}

void Mp3tunesArtistFetcher::run()
{
    DEBUG_BLOCK
    if(m_locker != 0) {
        debug() << "Artist Fetch Start";
        QList<Mp3tunesLockerArtist> list = m_locker->artists();
        debug() << "Artist Fetch End. Total artists: " << list.count();
        m_artists = list;
    } else {
        debug() << "Locker is NULL";
    }
}

void Mp3tunesArtistFetcher::completeJob()
{
    emit( artistsFetched( m_artists ) );
    deleteLater();
}

/*  ALBUM w/ Artist Id FETCHER */
Mp3tunesAlbumWithArtistIdFetcher::Mp3tunesAlbumWithArtistIdFetcher( Mp3tunesLocker * locker, int artistId )
{
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( completeJob() ) );
    m_locker = locker;
    m_artistId = artistId;
}

Mp3tunesAlbumWithArtistIdFetcher::~Mp3tunesAlbumWithArtistIdFetcher()
{
}

void Mp3tunesAlbumWithArtistIdFetcher::run()
{
    DEBUG_BLOCK
    if(m_locker != 0) {
        debug() << "Album Fetch Start";
        QList<Mp3tunesLockerAlbum> list = m_locker->albumsWithArtistId( m_artistId );
        debug() << "Album Fetch End. Total albums: " << list.count();
        m_albums = list;
    } else {
                debug() << "Locker is NULL";
    }
}

void Mp3tunesAlbumWithArtistIdFetcher::completeJob()
{
    emit( albumsFetched( m_albums ) );
    deleteLater();
}

/*  TRACK w/ albumId FETCHER */
Mp3tunesTrackWithAlbumIdFetcher::Mp3tunesTrackWithAlbumIdFetcher( Mp3tunesLocker * locker, int albumId )
{
    DEBUG_BLOCK
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( completeJob() ) );
    m_locker = locker;
    debug() << "Constructor albumId: " << albumId;
    m_albumId = albumId;
}

Mp3tunesTrackWithAlbumIdFetcher::~Mp3tunesTrackWithAlbumIdFetcher()
{
}

void Mp3tunesTrackWithAlbumIdFetcher::run()
{
    DEBUG_BLOCK
    if(m_locker != 0) {
        debug() << "Track Fetch Start for album " << m_albumId;
        QList<Mp3tunesLockerTrack> list = m_locker->tracksWithAlbumId( m_albumId );
        debug() << "Track Fetch End. Total tracks: " << list.count();
        m_tracks = list;
    } else {
            debug() << "Locker is NULL";
    }
}

void Mp3tunesTrackWithAlbumIdFetcher::completeJob()
{
    DEBUG_BLOCK
    emit( tracksFetched( m_tracks ) );
    deleteLater();
}

/*  SEARCH MONKEY */
Mp3tunesSearchMonkey::Mp3tunesSearchMonkey( Mp3tunesLocker * locker, QString query, int searchFor )
{
    DEBUG_BLOCK
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( completeJob() ) );
    m_locker = locker;
    m_searchFor = searchFor;
    m_query = query;
}

Mp3tunesSearchMonkey::~Mp3tunesSearchMonkey()
{}

void Mp3tunesSearchMonkey::run()
{
    DEBUG_BLOCK
    if(m_locker != 0) {
        Mp3tunesSearchResult container;
        debug() << "Searching query: " << m_query << "    bitmask: " << m_searchFor;
        container.searchFor = (Mp3tunesSearchResult::SearchType) m_searchFor;
        if( !m_locker->search(container, m_query) )
        {
            //TODO proper error handling
            debug() << "!!!Search Failed query: " << m_query << "    bitmask: " << m_searchFor;
        }
        m_result = container;
    } else {
        debug() << "Locker is NULL";
    }
}

void Mp3tunesSearchMonkey::completeJob()
{
    DEBUG_BLOCK
    emit( searchComplete( m_result.artistList ) );
    emit( searchComplete( m_result.albumList ) );
    emit( searchComplete( m_result.trackList ) );
    deleteLater();
}
