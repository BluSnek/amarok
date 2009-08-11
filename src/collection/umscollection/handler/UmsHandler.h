/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#ifndef UMSHANDLER_H
#define UMSHANDLER_H

// Taglib includes
//#include <audioproperties.h>
//#include <fileref.h>

//#include "UmsArtworkCapability.h"
//#include "UmsPlaylistCapability.h"
//#include "UmsReadCapability.h"
//#include "UmsWriteCapability.h"

#include "MediaDeviceMeta.h"
#include "MediaDeviceHandler.h"

#include "mediadevicecollection_export.h"

#include <KDiskFreeSpaceInfo>

#include <KIO/Job>
#include "kjob.h"
#include <ctime> // for kjob.h
#include <KTempDir>
#include <threadweaver/Job.h>

#include <QObject>
#include <QMap>
#include <QMultiMap>
#include <QMutex>

namespace Solid {
    class StorageAccess;
}

class UmsCollection;

class KDirLister;
class KFileItem;
class KUrl;

class QListIterator<KUrl>;
class QString;
class QMutex;

typedef QMultiMap<QString, Meta::TrackPtr> TitleMap;

namespace Meta
{

    typedef QMap<QString, Meta::TrackPtr> TrackMap;

/* The backend for all Ums calls */
class MEDIADEVICECOLLECTION_EXPORT UmsHandler : public Meta::MediaDeviceHandler
{
    Q_OBJECT

    public:
        UmsHandler( UmsCollection *mc, const QString& mountPoint );
        virtual ~UmsHandler();

        virtual void init(); // collection
        virtual bool isWritable() const;

        virtual QString prettyName() const;

        virtual QList<QAction *> collectionActions();

        /// Capability-related methods

        virtual bool hasCapabilityInterface( Handler::Capability::Type type ) const;
        virtual Handler::Capability* createCapabilityInterface( Handler::Capability::Type type );

        //friend class Handler::UmsArtworkCapability;
        //friend class Handler::UmsPlaylistCapability;
        //friend class Handler::UmsReadCapability;
        //friend class Handler::UmsWriteCapability;

        /// Ums-Specific Methods
        QMap<Meta::TrackPtr, QString> tracksFailed() const { return m_tracksFailed; }
        QString mountPoint() const { return m_mountPoint; }
        void setMountPoint( const QString &mp ) { m_mountPoint = mp; }

    public slots:
        virtual void writeDatabase();

    protected:
        /// Functions for PlaylistCapability
        /**
         * Writes to the device's database if it has one, otherwise
         * simply calls slotDatabaseWritten to continue the workflow.
         */
#if 0
        virtual void prepareToParsePlaylists();
        virtual bool isEndOfParsePlaylistsList();
        virtual void prepareToParseNextPlaylist();
        virtual void nextPlaylistToParse();

        virtual bool shouldNotParseNextPlaylist();

        virtual void prepareToParsePlaylistTracks();
        virtual bool isEndOfParsePlaylist();
        virtual void prepareToParseNextPlaylistTrack();
        virtual void nextPlaylistTrackToParse();
#endif
        virtual QStringList supportedFormats();
#if 0
        virtual void findPathToCopy( const Meta::TrackPtr &srcTrack, const Meta::MediaDeviceTrackPtr &destTrack );
        virtual bool libCopyTrack( const Meta::TrackPtr &srcTrack, Meta::MediaDeviceTrackPtr &destTrack );
        virtual bool libDeleteTrackFile( const Meta::MediaDeviceTrackPtr &track );
        virtual void libCreateTrack( const Meta::MediaDeviceTrackPtr &track );
        virtual void libDeleteTrack( const Meta::MediaDeviceTrackPtr &track );

        virtual Meta::MediaDeviceTrackPtr libGetTrackPtrForTrackStruct();

        virtual QString libGetPlaylistName();
        void setAssociatePlaylist( const Meta::MediaDevicePlaylistPtr &playlist );
        void libSavePlaylist( const Meta::MediaDevicePlaylistPtr &playlist, const QString& name );
        void deletePlaylist( const Meta::MediaDevicePlaylistPtr &playlist );
        void renamePlaylist( const Meta::MediaDevicePlaylistPtr &playlist );
#endif
#if 0

        virtual void libSetTitle( Meta::MediaDeviceTrackPtr &track, const QString& title );
        virtual void libSetAlbum( Meta::MediaDeviceTrackPtr &track, const QString& album );
        virtual void libSetArtist( Meta::MediaDeviceTrackPtr &track, const QString& artist );
        virtual void libSetComposer( Meta::MediaDeviceTrackPtr &track, const QString& composer );
        virtual void libSetGenre( Meta::MediaDeviceTrackPtr &track, const QString& genre );
        virtual void libSetYear( Meta::MediaDeviceTrackPtr &track, const QString& year );
        virtual void libSetLength( Meta::MediaDeviceTrackPtr &track, int length );
        virtual void libSetTrackNumber( Meta::MediaDeviceTrackPtr &track, int tracknum );
        virtual void libSetComment( Meta::MediaDeviceTrackPtr &track, const QString& comment );
        virtual void libSetDiscNumber( Meta::MediaDeviceTrackPtr &track, int discnum );
        virtual void libSetBitrate( Meta::MediaDeviceTrackPtr &track, int bitrate );
        virtual void libSetSamplerate( Meta::MediaDeviceTrackPtr &track, int samplerate );
        virtual void libSetBpm( Meta::MediaDeviceTrackPtr &track, float bpm );
        virtual void libSetFileSize( Meta::MediaDeviceTrackPtr &track, int filesize );
        virtual void libSetPlayCount( Meta::MediaDeviceTrackPtr &track, int playcount );
        virtual void libSetLastPlayed( Meta::MediaDeviceTrackPtr &track, uint lastplayed );
        virtual void libSetRating( Meta::MediaDeviceTrackPtr &track, int rating ) ;
        virtual void libSetType( Meta::MediaDeviceTrackPtr &track, const QString& type );
        virtual void libSetPlayableUrl( Meta::MediaDeviceTrackPtr &destTrack, const Meta::TrackPtr &srcTrack );

        // TODO: MediaDeviceTrackPtr
        virtual void libSetCoverArt( Itdb_Track *umstrack, const QPixmap &image );
        virtual void setCoverArt( Itdb_Track *umstrack, const QString &path );

        virtual void prepareToCopy();
        virtual void prepareToDelete();
#endif
    private:
        enum FileType
        {
            mp3,
            ogg,
            flac,
            mp4
        };

        /// Functions for ReadCapability
#if 0
        virtual void prepareToParseTracks();
        virtual bool isEndOfParseTracksList();
        virtual void prepareToParseNextTrack();
        virtual void nextTrackToParse();

        virtual void setAssociateTrack( const Meta::MediaDeviceTrackPtr track );

        virtual QString libGetTitle( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetAlbum( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetArtist( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetComposer( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetGenre( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetYear( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetLength( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetTrackNumber( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetComment( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetDiscNumber( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetBitrate( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetSamplerate( const Meta::MediaDeviceTrackPtr &track );
        virtual float   libGetBpm( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetFileSize( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetPlayCount( const Meta::MediaDeviceTrackPtr &track );
        virtual uint    libGetLastPlayed( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetRating( const Meta::MediaDeviceTrackPtr &track ) ;
        virtual QString libGetType( const Meta::MediaDeviceTrackPtr &track );
        virtual KUrl    libGetPlayableUrl( const Meta::MediaDeviceTrackPtr &track );
        virtual QPixmap libGetCoverArt( const Meta::MediaDeviceTrackPtr &track );
#endif


        virtual float usedCapacity() const;
        virtual float totalCapacity() const;
        /// Ums Methods

#if 0
        /* File I/O Methods */
        // TODO: abstract copy/delete methods (not too bad)
        bool kioCopyTrack( const KUrl &src, const KUrl &dst );
        void deleteFile( const KUrl &url );
#endif
        /**
        * Handler Variables
        */

        KDirLister           *m_dirLister;

        QListIterator<KUrl>  *m_currtrackurl;

        // For space checks
        QString               m_filepath;
        float                 m_capacity;

        /* Lockers */
        QMutex            m_joblocker; // lets only 1 job finish at a time
        int               m_jobcounter; // keeps track of copy jobs present

        /* Copy/Delete Variables */
        Meta::TrackList   m_tracksToDelete;

        QMap<KUrl, Meta::TrackPtr> m_trackscopying; // associates source url to track of source url
        QMap<Meta::TrackPtr, KUrl> m_trackdesturl; // keeps track of destination url for new tracks, mapped from source track

        QMap<KUrl, Meta::TrackPtr> m_tracksdeleting; // associates source url to track of source url being deleted

        /* Ums Connection */
        bool    m_autoConnect;
        QString m_mountPoint;
        bool    m_wasMounted;
        QString m_name;

        /* Success/Failure */
        bool m_dbChanged;
        bool m_copyFailed;
        bool m_isCanceled;
        bool m_wait;

        /* Miscellaneous Variables */

        // Hash that associates an MetaFile::Track to every Track*
        QHash<Meta::MediaDeviceTrackPtr, Meta::TrackPtr> m_umstrackhash;

        // Hash that associates an Itdb_Playlist* to every PlaylistPtr
        //QHash<Meta::MediaDevicePlaylistPtr, Itdb_Playlist*> m_itdbplaylisthash;

        // tracks that failed to copy
        QMap<Meta::TrackPtr, QString> m_tracksFailed;

        // tempdir for covers
        KTempDir *m_tempdir;
        QSet<QString> m_coverArt;

    private slots:
        void dirListerParseCompleted();
#if 0
        void fileTransferred( KJob *job );
        void fileDeleted( KJob *job );

        void slotCopyingDone( KIO::Job* job, KUrl from, KUrl to, time_t mtime, bool directory, bool renamed );
#endif
};


}
#endif
