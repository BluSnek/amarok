/****************************************************************************************
 * Copyright (c) 2008 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2013 Matěj Laitl <matej@laitl.cz>                                      *
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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_TRACKLOADER_H
#define AMAROK_TRACKLOADER_H

#include "amarok_export.h"
#include "core/meta/forward_declarations.h"
#include "core/playlists/Playlist.h"

#include <KFileItem>

class KJob;
namespace KIO {
    class Job;
    class UDSEntry;
    typedef QList<UDSEntry>  UDSEntryList;
    typedef QList<KFileItem> KFileItemList;
}

class AMAROK_EXPORT TrackLoader : public QObject, public Playlists::PlaylistObserver
{
    Q_OBJECT

    public:
        /**
         * BlockingLoading: the tracks are loaded synchronously in the main thread,
         *                  but you have a guarantee that their metadata is valid
         *                  from start
         * AsyncLoading: the tracks and loaded using MetaProxy::Tracks, initial metadata
         *               are just stubs and the real ones are fetched in the banground,
         *               then you get metadataUpdated() */
        enum LoadingMode {
            BlockingLoading,
            AsyncLoading
        };
        TrackLoader( LoadingMode loadingMode = AsyncLoading );
        ~TrackLoader();

        void init( const QList<KUrl> &urls );
        void init( const QList<QUrl> &urls );

        /* PlaylistObserver methods */
        virtual void tracksLoaded( Playlists::PlaylistPtr playlist );

    signals:
        void finished( const Meta::TrackList &tracks );

    private slots:
        void directoryListResults( KIO::Job *job, const KIO::UDSEntryList &list );
        void listJobFinished();

    private:
        void finishUrlList();
        /** Completes the initialization */
        void finish();
        /**
         * Probe the file @file, if it is a playlist or a track, add it to m_tracks
         */
        void appendFile( const KUrl &url );

        static bool directorySensitiveLessThan( const KFileItem &item1, const KFileItem &item2 );

        /**
         * the number of directory list operations. This is used so that
         * the last directory operations knows its the last
         */
        int m_listOperations;
        /**
         * The number of unprocessed entities.
         * Entity is track or playlist
         */
        int m_entities;
        /**
         * Store list of url which should be processed
         */
        QList<KUrl> m_urlsToLoad;
        LoadingMode m_loadingMode;
        KIO::KFileItemList m_expanded;
        Meta::TrackList m_tracks; //!< the tracks found. they get all sorted at the end.
};

#endif // AMAROK_TRACKLOADER_H
