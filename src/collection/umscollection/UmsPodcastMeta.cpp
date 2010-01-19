/****************************************************************************************
 * Copyright (c) 2010 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "UmsPodcastMeta.h"
#include "meta/PlaylistFileSupport.h"

using namespace Meta;

UmsPodcastEpisodePtr
UmsPodcastEpisode::fromPodcastEpisodePtr( PodcastEpisodePtr episode )
{
    return UmsPodcastEpisodePtr::dynamicCast( episode );
}

PodcastEpisodePtr
UmsPodcastEpisode::toPodcastEpisodePtr( UmsPodcastEpisodePtr episode )
{
    return PodcastEpisodePtr::dynamicCast( episode );
}

UmsPodcastEpisode::UmsPodcastEpisode( UmsPodcastChannelPtr channel )
        : Meta::PodcastEpisode( UmsPodcastChannel::toPodcastChannelPtr( channel ) )
{
}

UmsPodcastEpisode::~UmsPodcastEpisode()
{
}

void
UmsPodcastEpisode::setLocalUrl( KUrl localUrl )
{
    m_localUrl = localUrl;
}

void
UmsPodcastEpisode::setLocalFile( MetaFile::TrackPtr localFile )
{
    m_localFile = localFile;
}

QString
UmsPodcastEpisode::name() const
{
    if( m_localFile.isNull() )
        return m_title;

    return m_localFile->name();
}

QString
UmsPodcastEpisode::prettyName() const
{
    /*for now just do the same as name, but in the future we might want to used a cleaned
      up string using some sort of regex tag rewrite for podcasts. decapitateString on
      steroides. */
    return name();
}

bool
UmsPodcastEpisode::isEditable() const
{
     if( m_localFile.isNull() )
         return false;

     return m_localFile->isEditable();
}

void
UmsPodcastEpisode::setTitle( const QString &title )
{
    if( !m_localFile.isNull() )
    {
        m_localFile->setTitle( title );
    }

    m_title = title;
}

AlbumPtr
UmsPodcastEpisode::album() const
{
    if( m_localFile.isNull() )
        return m_albumPtr;

    return m_localFile->album();
}

ArtistPtr
UmsPodcastEpisode::artist() const
{
    if( m_localFile.isNull() )
        return m_artistPtr;

    return m_localFile->artist();
}

ComposerPtr
UmsPodcastEpisode::composer() const
{
    if( m_localFile.isNull() )
        return m_composerPtr;

    return m_localFile->composer();
}

GenrePtr
UmsPodcastEpisode::genre() const
{
    if( m_localFile.isNull() )
        return m_genrePtr;

    return m_localFile->genre();
}

YearPtr
UmsPodcastEpisode::year() const
{
    if( m_localFile.isNull() )
        return m_yearPtr;

    return m_localFile->year();
}

UmsPodcastChannelPtr
UmsPodcastChannel::fromPodcastChannelPtr( PodcastChannelPtr channel )
{
    return UmsPodcastChannelPtr::dynamicCast( channel );
}

PodcastChannelPtr
UmsPodcastChannel::toPodcastChannelPtr( UmsPodcastChannelPtr channel )
{
    return PodcastChannelPtr::dynamicCast( channel );
}

PodcastChannelList
UmsPodcastChannel::toPodcastChannelList( UmsPodcastChannelList umsChannels )
{
    PodcastChannelList channels;
    foreach( UmsPodcastChannelPtr umsChannel, umsChannels )
        channels << UmsPodcastChannel::toPodcastChannelPtr(  umsChannel );
    return channels;
}

UmsPodcastChannel::UmsPodcastChannel( UmsPodcastProvider *provider )
        : Meta::PodcastChannel()
        , m_provider( provider )
{

}

UmsPodcastChannel::~UmsPodcastChannel()
{

}

void
UmsPodcastChannel::setPlaylistFileSource( const KUrl &playlistFilePath )
{
    m_playlistFilePath = playlistFilePath;
    m_playlistFile = Meta::loadPlaylistFile( playlistFilePath );

    //now parse the playlist and use it to create out episode list
}
