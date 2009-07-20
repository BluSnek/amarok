/*
 *  Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "ProxyCollectionMeta.h"

#include "ProxyCollection.h"

#include <QSet>

ProxyCollection::Track::Track( ProxyCollection::Collection *coll, const Meta::TrackPtr &track )
        : Meta::Track()
        , Meta::Observer()
        , m_collection( coll )
        , m_name( track->name() )
{
    subscribeTo( track );
    m_tracks.append( track );
}

ProxyCollection::Track::~Track()
{
}

QString
ProxyCollection::Track::name() const
{
    return m_name;
}

QString
ProxyCollection::Track::prettyName() const
{
    return m_name;
}

KUrl
ProxyCollection::Track::playableUrl() const
{
    Meta::TrackPtr bestPlayableTrack;
    bool bestPlayableTrackIsLocalFile = false;
    foreach( const Meta::TrackPtr &track, m_tracks )
    {
        if( track->isPlayable() )
        {
            bool local = track->playableUrl().isLocalFile();
            if( local && !bestPlayableTrackIsLocalFile )
            {
                bestPlayableTrack = track;
                bestPlayableTrackIsLocalFile = true;
            }
            else if( !local && !bestPlayableTrack )
            {
                bestPlayableTrack = track;
            }
        }
    }
    if( bestPlayableTrack )
        return bestPlayableTrack->playableUrl();

    return KUrl();
}

QString
ProxyCollection::Track::prettyUrl() const
{
    return QString();
}

QString
ProxyCollection::Track::uidUrl() const
{
    //this is where it gets interesting
    //a uidUrl for a proxyTrack probably has to be generated
    //from the parts of the key in ProxyCollection::Collection
    //need to think about this some more
    return QString();
}

bool
ProxyCollection::Track::isPlayable() const
{
    foreach( const Meta::TrackPtr &track, m_tracks )
    {
        if( track->isPlayable() )
            return true;
    }
    return false;
}

ProxyCollection::Album::Album( ProxyCollection::Collection *coll, Meta::AlbumPtr album )
        : Meta::Album()
        , Meta::Observer()
        , m_collection( coll )
        , m_name( album->name() )
{
    m_albums.append( album );
    if( album->hasAlbumArtist() )
        m_albumArtist = Meta::ArtistPtr( m_collection->getArtist( album->albumArtist() ) );
}

ProxyCollection::Album::~Album()
{
}

QString
ProxyCollection::Album::name() const
{
    return m_name;
}

QString
ProxyCollection::Album::prettyName() const
{
    return m_name;
}

Meta::TrackList
ProxyCollection::Album::tracks()
{
    QSet<ProxyCollection::Track*> tracks;
    foreach( Meta::AlbumPtr album, m_albums )
    {
        Meta::TrackList tmp = album->tracks();
        foreach( const Meta::TrackPtr &track, tmp )
        {
            tracks.insert( m_collection->getTrack( track ) );
        }
    }

    Meta::TrackList result;
    foreach( ProxyCollection::Track *track, tracks )
    {
        result.append( Meta::TrackPtr( track ) );
    }
    return result;
}

Meta::ArtistPtr
ProxyCollection::Album::albumArtist() const
{
    return m_albumArtist;
}

bool
ProxyCollection::Album::isCompilation() const
{
    return m_albumArtist.isNull();
}

bool
ProxyCollection::Album::hasAlbumArtist() const
{
    return !m_albumArtist.isNull();
}

void
ProxyCollection::Album::add( Meta::AlbumPtr album )
{
    if( !album || !m_albums.contains( album ) )
        return;

    m_albums.append( album );
    subscribeTo( album );

    notifyObservers();
}

void
ProxyCollection::Album::metadataChanged( Meta::AlbumPtr album )
{
    if( !album || !m_albums.contains( album ) )
        return;

    if( album->name() != m_name ||
        hasAlbumArtist() != album->hasAlbumArtist() ||
        ( hasAlbumArtist() && m_albumArtist->name() != album->albumArtist()->name() ) )
    {
        if( m_albums.count() > 1 )
        {
            m_collection->getAlbum( album );
            unsubscribeFrom( album );
            m_albums.removeAll( album );
        }
        else
        {
            Meta::ArtistPtr albumartist;
            if( album->hasAlbumArtist() )
                 albumartist = Meta::ArtistPtr( m_collection->getArtist( album->albumArtist() ) );

            QString artistname = m_albumArtist ? m_albumArtist->name() : QString();
            m_collection->removeAlbum( m_name, artistname );
            m_name = album->name();
            m_albumArtist = albumartist;
            m_collection->setAlbum( this );

        }
    }

    notifyObservers();
}

ProxyCollection::Artist::Artist( ProxyCollection::Collection *coll, Meta::ArtistPtr artist )
        : Meta::Artist()
        , Meta::Observer()
        , m_collection( coll )
        , m_name( artist->name() )
{
    m_artists.append( artist );
    subscribeTo( artist );
}

ProxyCollection::Artist::~Artist()
{
}

QString
ProxyCollection::Artist::name() const
{
    return m_name;
}

QString
ProxyCollection::Artist::prettyName() const
{
    return m_name;
}

Meta::TrackList
ProxyCollection::Artist::tracks()
{
    QSet<ProxyCollection::Track*> tracks;
    foreach( Meta::ArtistPtr artist, m_artists )
    {
        Meta::TrackList tmp = artist->tracks();
        foreach( const Meta::TrackPtr &track, tmp )
        {
            tracks.insert( m_collection->getTrack( track ) );
        }
    }

    Meta::TrackList result;
    foreach( ProxyCollection::Track *track, tracks )
    {
        result.append( Meta::TrackPtr( track ) );
    }
    return result;
}

Meta::AlbumList
ProxyCollection::Artist::albums()
{
    QSet<ProxyCollection::Album*> albums;
    foreach( Meta::ArtistPtr artist, m_artists )
    {
        Meta::AlbumList tmp = artist->albums();
        foreach( const Meta::AlbumPtr &album, tmp )
        {
            albums.insert( m_collection->getAlbum( album ) );
        }
    }

    Meta::AlbumList result;
    foreach( ProxyCollection::Album *album, albums )
    {
        result.append( Meta::AlbumPtr( album ) );
    }
    return result;
}

void
ProxyCollection::Artist::add( Meta::ArtistPtr artist )
{
    if( !artist || !m_artists.contains( artist ) )
        return;

    m_artists.append( artist );
    subscribeTo( artist );

    notifyObservers();
}

void
ProxyCollection::Artist::metadataChanged( Meta::ArtistPtr artist )
{
    if( !artist || !m_artists.contains( artist ) )
        return;

    if( artist->name() != m_name )
    {
        if( m_artists.count() > 1 )
        {
            m_collection->getArtist( artist );
            unsubscribeFrom( artist );
            m_artists.removeAll( artist );
        }
        else
        {
            //possible race condition here:
            //if another thread creates an Artist with the new name
            //we will have two instances that have the same name!
            //TODO: figure out a way around that
            //the race condition is a problem for all other metadataChanged methods too
            m_collection->removeArtist( m_name );
            m_name = artist->name();
            m_collection->setArtist( this );
        }
    }

    notifyObservers();
}

ProxyCollection::Genre::Genre( ProxyCollection::Collection *coll, Meta::GenrePtr genre )
        : Meta::Genre()
        , Meta::Observer()
        , m_collection( coll )
        , m_name( genre->name() )
{
    m_genres.append( genre );
    subscribeTo( genre );
}

ProxyCollection::Genre::~Genre()
{
}

QString
ProxyCollection::Genre::name() const
{
    return m_name;
}

QString
ProxyCollection::Genre::prettyName() const
{
    return m_name;
}

Meta::TrackList
ProxyCollection::Genre::tracks()
{
    QSet<ProxyCollection::Track*> tracks;
    foreach( Meta::GenrePtr genre, m_genres )
    {
        Meta::TrackList tmp = genre->tracks();
        foreach( const Meta::TrackPtr &track, tmp )
        {
            tracks.insert( m_collection->getTrack( track ) );
        }
    }

    Meta::TrackList result;
    foreach( ProxyCollection::Track *track, tracks )
    {
        result.append( Meta::TrackPtr( track ) );
    }
    return result;
}

void
ProxyCollection::Genre::add( Meta::GenrePtr genre )
{
    if( !genre || !m_genres.contains( genre ) )
        return;

    m_genres.append( genre );
    subscribeTo( genre );

    notifyObservers();
}

void
ProxyCollection::Genre::metadataChanged( Meta::GenrePtr genre )
{
    if( !genre || !m_genres.contains( genre ) )
        return;

    if( genre->name() != m_name )
    {
        if( m_genres.count() > 1 )
        {
            m_collection->getGenre( genre );
            unsubscribeFrom( genre );
            m_genres.removeAll( genre );
        }
        else
        {
            m_collection->removeGenre( m_name );
            m_collection->setGenre( this );
            m_name = genre->name();
        }
    }

    notifyObservers();
}

ProxyCollection::Composer::Composer( ProxyCollection::Collection *coll, Meta::ComposerPtr composer )
        : Meta::Composer()
        , Meta::Observer()
        , m_collection( coll )
        , m_name( composer->name() )
{
    m_composers.append( composer );
    subscribeTo( composer );
}

ProxyCollection::Composer::~Composer()
{
}

QString
ProxyCollection::Composer::name() const
{
    return m_name;
}

QString
ProxyCollection::Composer::prettyName() const
{
    return m_name;
}

Meta::TrackList
ProxyCollection::Composer::tracks()
{
    QSet<ProxyCollection::Track*> tracks;
    foreach( Meta::ComposerPtr composer, m_composers )
    {
        Meta::TrackList tmp = composer->tracks();
        foreach( const Meta::TrackPtr &track, tmp )
        {
            tracks.insert( m_collection->getTrack( track ) );
        }
    }

    Meta::TrackList result;
    foreach( ProxyCollection::Track *track, tracks )
    {
        result.append( Meta::TrackPtr( track ) );
    }
    return result;
}

void
ProxyCollection::Composer::add( Meta::ComposerPtr composer )
{
    if( !composer || !m_composers.contains( composer ) )
        return;

    m_composers.append( composer );
    subscribeTo( composer );

    notifyObservers();
}

void
ProxyCollection::Composer::metadataChanged( Meta::ComposerPtr composer )
{
    if( !composer || !m_composers.contains( composer ) )
        return;

    if( composer->name() != m_name )
    {
        if( m_composers.count() > 1 )
        {
            m_collection->getComposer( composer );
            unsubscribeFrom( composer );
            m_composers.removeAll( composer );
        }
        else
        {
            m_collection->removeComposer( m_name );
            m_collection->setComposer( this );
            m_name = composer->name();
        }
    }

    notifyObservers();
}

ProxyCollection::Year::Year( ProxyCollection::Collection *coll, Meta::YearPtr year )
        : Meta::Year()
        , Meta::Observer()
        , m_collection( coll )
        , m_name( year->name() )
{
    m_years.append( year );
    subscribeTo( year );
}

ProxyCollection::Year::~Year()
{
    //nothing to do
}

QString
ProxyCollection::Year::name() const
{
    return m_name;
}

QString
ProxyCollection::Year::prettyName() const
{
    return m_name;
}

Meta::TrackList
ProxyCollection::Year::tracks()
{
    QSet<ProxyCollection::Track*> tracks;
    foreach( Meta::YearPtr year, m_years )
    {
        Meta::TrackList tmp = year->tracks();
        foreach( const Meta::TrackPtr &track, tmp )
        {
            tracks.insert( m_collection->getTrack( track ) );
        }
    }

    Meta::TrackList result;
    foreach( ProxyCollection::Track *track, tracks )
    {
        result.append( Meta::TrackPtr( track ) );
    }
    return result;
}

void
ProxyCollection::Year::add( Meta::YearPtr year )
{
    if( !year || !m_years.contains( year ) )
        return;

    m_years.append( year );
    subscribeTo( year );

    notifyObservers();
}

void
ProxyCollection::Year::metadataChanged( Meta::YearPtr year )
{
    if( !year || !m_years.contains( year ) )
        return;

    if( year->name() != m_name )
    {
        if( m_years.count() > 1 )
        {
            m_collection->getYear( year );
            unsubscribeFrom( year );
            m_years.removeAll( year );
        }
        else
        {
            if( m_collection->hasYear( year->name() ) )
            {
                unsubscribeFrom( year );
                m_collection->getYear( year );
                m_years.removeAll( year );
                m_collection->removeYear( m_name );
            }
            else
            {
                //be careful with the ordering of instructions here
                //ProxyCollection uses KSharedPtr internally
                //so we have to make sure that there is more than one pointer
                //to this instance by registering this instance under the new name
                //before removing the old one. Otherwise kSharedPtr might delete this
                //instance in removeYear()
                QString tmpName = m_name;
                m_name = year->name();
                m_collection->setYear( this );
                m_collection->removeYear( tmpName );
            }
        }
    }

    notifyObservers();
}
