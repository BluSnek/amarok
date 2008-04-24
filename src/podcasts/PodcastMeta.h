/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef PODCASTMETA_H
#define PODCASTMETA_H

#include "Meta.h"
#include "Playlist.h"

#include <kurl.h>
#include <KLocale>

#include <QSharedData>
#include <QString>
#include <QStringList>
#include <QTextStream>

namespace Meta
{

class PodcastMetaCommon;
class PodcastEpisode;
class PodcastChannel;

// typedef KSharedPtr<PodcastMetaCommon> PodcastMetaCommonPtr;
typedef KSharedPtr<PodcastEpisode> PodcastEpisodePtr;
typedef KSharedPtr<PodcastChannel> PodcastChannelPtr;

// typedef QList<PodcastMetaCommon> PodcastMetaCommonList;
typedef QList<PodcastEpisodePtr> PodcastEpisodeList;
typedef QList<PodcastChannelPtr> PodcastChannelList;

class PodcastMetaCommon
{
    public:
        enum Type
        {
            NoType = 0,
            ChannelType,
            EpisodeType
        };

//         PodcastMetaCommon();
        virtual ~PodcastMetaCommon() {}

        virtual QString title() const { return m_title;} ;
        virtual QString description() const { return m_description; };
        virtual QStringList keywords() const { return m_keywords; };
        virtual QString subtitle() const { return m_subtitle; };
        virtual QString summary() const { return m_summary; };
        virtual QString author() const { return m_author; };

        virtual void setTitle( const QString &title ) { m_title = title; };
        virtual void setDescription( const QString &description ) { m_description = description; };
        virtual void setKeywords( const QStringList &keywords ) { m_keywords = keywords; };
        virtual void addKeyword( const QString &keyword ) { m_keywords << keyword; };
        virtual void setSubtitle( const QString &subtitle ) { m_subtitle = subtitle; };
        virtual void setSummary( const QString &summary ) { m_summary = summary; };
        virtual void setAuthor( const QString &author ) { m_author = author; };

        virtual int podcastType() { return NoType; };

    protected:
        QString m_title; //the title
        QString m_description; //a longer description, possibly with HTML markup
        QStringList m_keywords;
        QString m_subtitle; //a short description
        QString m_summary;
        QString m_author;

};

class PodcastEpisode : public Track, public PodcastMetaCommon
{
    public:
        PodcastEpisode() {};
        PodcastEpisode( PodcastChannelPtr channel ) : m_channel( channel ) {};

        virtual ~PodcastEpisode() {};

        //MetaBase methods
        virtual QString name() const { return m_title; };
        virtual QString prettyName() const { return m_title; };

        //Track Methods
        virtual KUrl playableUrl() const { return m_localUrl.isEmpty() ? m_url : m_localUrl; };
        virtual QString prettyUrl() const { return m_url.prettyUrl(); };
        virtual QString url() const { return m_url.url(); };
        virtual bool isPlayable() const { return true; };
        virtual bool isEditable() const { return false; };

        virtual AlbumPtr album() const { return AlbumPtr(); };
        virtual void setAlbum( const QString &newAlbum ) { Q_UNUSED( newAlbum ); };
        virtual ArtistPtr artist() const { return ArtistPtr(); };
        virtual void setArtist( const QString &newArtist ) { Q_UNUSED( newArtist ); };
        virtual ComposerPtr composer() const { return ComposerPtr(); };
        virtual void setComposer( const QString &newComposer ) { Q_UNUSED( newComposer ); };
        virtual GenrePtr genre() const { return GenrePtr(); };
        virtual void setGenre( const QString &newGenre ) { Q_UNUSED( newGenre ); };
        virtual YearPtr year() const { return YearPtr(); };
        virtual void setYear( const QString &newYear ) { Q_UNUSED( newYear ); };

        virtual void setTitle( const QString &title ) { m_title = title; };

        virtual QString comment() const { return QString(); };
        virtual void setComment( const QString &newComment ) { Q_UNUSED( newComment ); };
        virtual double score() const { return 0; };
        virtual void setScore( double newScore ) { Q_UNUSED( newScore ); };
        virtual int rating() const { return 0; };
        virtual void setRating( int newRating ) { Q_UNUSED( newRating ); };
        virtual int length() const { return m_duration; };
        virtual int filesize() const { return m_fileSize; };
        virtual int sampleRate() const { return 0; };
        virtual int bitrate() const { return 0; };
        virtual int trackNumber() const { return m_sequenceNumber; };
        virtual void setTrackNumber( int newTrackNumber ) { Q_UNUSED( newTrackNumber ); };
        virtual int discNumber() const { return 0; };
        virtual void setDiscNumber( int newDiscNumber ) { Q_UNUSED( newDiscNumber ); };
        virtual uint lastPlayed() const { return 0; };
        virtual int playCount() const { return 0; };

        virtual QString type() const { return i18n( "Podcast" ); };

        virtual void beginMetaDataUpdate() {};
        virtual void endMetaDataUpdate() {};
        virtual void abortMetaDataUpdate() {};
        virtual void finishedPlaying( double playedFraction ) { Q_UNUSED( playedFraction ); };
        virtual void addMatchTo( QueryMaker* qm ) { Q_UNUSED( qm ); };
        virtual bool inCollection() const { return false; };
        virtual QString cachedLyrics() const { return QString(); };
        virtual void setCachedLyrics( const QString &lyrics ) { Q_UNUSED( lyrics ); };
        virtual void notifyObservers() const {};

        //PodcastMetaCommon methods
        int podcastType() { return EpisodeType; };

        //PodcastEpisode methods
        virtual KUrl localUrl() const { return m_localUrl; };
        virtual void setLocalUrl( const KUrl &url ) { m_localUrl = url; };
        virtual QString pubDate() const { return m_pubDate; };
        virtual int duration() const { return m_duration; };
        virtual QString guid() const { return m_guid; };

        virtual void setUrl( const KUrl &url ) { m_url = url; };
        virtual void setPubDate( const QString &pubDate ) { m_pubDate = pubDate; };
        virtual void setDuration( int duration ) { m_duration = duration; };
        virtual void setGuid( const QString &guid ) { m_guid = guid; };

        virtual int sequenceNumber() const { return m_sequenceNumber; };
        virtual void setSequenceNumber( int sequenceNumber ) { m_sequenceNumber = sequenceNumber; };

        virtual PodcastChannelPtr channel() { return m_channel; };
        virtual void setChannel( const PodcastChannelPtr channel ) { m_channel = channel; };

    protected:
        PodcastChannelPtr m_channel;

        QString m_guid; //the GUID from the podcast feed
        KUrl m_url; //remote url of the file
        KUrl m_localUrl; //the localUrl, only valid if downloaded
        QString m_mimeType; //the mimetype of the enclosure
        //TODO: convert to QDateTime from a RFC822 format
        QString m_pubDate; //the pubDate from the feed
        int m_duration; //the playlength in seconds
        int m_fileSize; //the size tag from the enclosure
        int m_sequenceNumber; //number of the episode
        bool m_isNew; //listened to or not?

};

class PodcastChannel : public Playlist, public PodcastMetaCommon
{
    public:

        enum FetchType
        {
            DownloadWhenAvailable = 0,
            StreamOrDownloadOnDemand
        };

        virtual ~PodcastChannel() {};

        //Playlist virtual methods
        virtual QString name() const { return title(); };
        virtual QString prettyName() const { return title(); };

        virtual TrackList tracks() { return m_tracks; };

        virtual KUrl retrievableUrl() { return KUrl(); };

        //PodcastMetaCommon methods
        int podcastType() { return ChannelType; };

        //PodcastChannel specific methods
        virtual KUrl url() const { return m_url; };
        virtual KUrl webLink() const { return m_webLink; };
        virtual QPixmap image() const { return m_image; };
        virtual QString copyright() { return m_copyright; };
        virtual QStringList labels() const { return m_labels; };

        virtual void setUrl( const KUrl &url ) { m_url = url; };
        virtual void setWebLink( const KUrl &link ) { m_webLink = link; };
        virtual void setImage( const QPixmap &image ) { m_image = image; };
        virtual void setCopyright( const QString &copyright ) { m_copyright = copyright; };
        virtual void setLabels( const QStringList &labels ) { m_labels = labels; };
        virtual void addLabel( const QString &label ) { m_labels << label; };

        virtual void addEpisode( PodcastEpisodePtr episode ) { m_episodes << episode; };
        virtual PodcastEpisodeList episodes() { return m_episodes; };

        virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const { Q_UNUSED( type ); return false; };

        virtual Meta::Capability* asCapabilityInterface( Meta::Capability::Type type ) { Q_UNUSED( type ); return static_cast<Meta::Capability *>( 0 ); };

        virtual bool load( QTextStream &stream ) { Q_UNUSED( stream ); return false; };

    protected:
        KUrl m_url;
        KUrl m_webLink;
        QPixmap m_image;
        QStringList m_labels;
        QString m_copyright;
        KUrl m_directory; //the local directory to save the files in.
        bool m_autoScan; //should this channel be checked automatically?
        PodcastChannel::FetchType m_fetchType; //'download when available' or 'stream or download on demand'
        bool m_autoTransfer; //copy to mediadevice?
        bool m_hasPurge; //remove old episodes?
        int m_purgeCount; //how many episodes do we keep on disk?

        PodcastEpisodeList m_episodes;
        TrackList m_tracks;
};

}

// Q_DECLARE_METATYPE( Meta::PodcastMetaCommonPtr )
// Q_DECLARE_METATYPE( Meta::PodcastMetaCommonList )
Q_DECLARE_METATYPE( Meta::PodcastEpisodePtr )
Q_DECLARE_METATYPE( Meta::PodcastEpisodeList )
Q_DECLARE_METATYPE( Meta::PodcastChannelPtr )
Q_DECLARE_METATYPE( Meta::PodcastChannelList )

#endif
