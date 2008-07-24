/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
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

#ifndef AMAROK_AMAROKMIMEDATA_H
#define AMAROK_AMAROKMIMEDATA_H

#include "amarok_export.h"
#include "meta/Meta.h"
#include "meta/Playlist.h"
#include "collection/QueryMaker.h"
#include "playlistbrowser/SqlPlaylistGroup.h"

#include <QList>
#include <QMap>
#include <QMimeData>

class AMAROK_EXPORT AmarokMimeData : public QMimeData
{
    Q_OBJECT
    public:
        static const QString TRACK_MIME;
        static const QString PLAYLIST_MIME;

        static const QString PLAYLISTBROWSERGROUP_MIME;

        AmarokMimeData();
        virtual ~AmarokMimeData();

        virtual QStringList formats() const;
        virtual bool hasFormat( const QString &mimeType ) const;

        Meta::TrackList tracks() const;
        void setTracks( const Meta::TrackList &tracks );
        void addTracks( const Meta::TrackList &tracks );

        Meta::PlaylistList playlists() const;
        void setPlaylists( const Meta::PlaylistList &playlists );
        void addPlaylists( const Meta::PlaylistList &playlists );

        SqlPlaylistGroupList sqlPlaylistsGroups() const;
        void setPlaylistGroups( const SqlPlaylistGroupList &groups );
        void addPlaylistGroups( const SqlPlaylistGroupList &groups );


        QList<QueryMaker*> queryMakers();
        void addQueryMaker( QueryMaker *queryMaker );
        void setQueryMakers( const QList<QueryMaker*> &queryMakers );

        /**
            There is a lot of time to run the queries while the user is dragging.
            This method runs all queries passed to this object. It will do nothing if there
            are no queries.
         */
        void startQueries();

    signals:
        void trackListSignal( Meta::TrackList ) const;

    public slots:
        void getTrackListSignal() const;

    protected:
        virtual QVariant retrieveData( const QString &mimeType, QVariant::Type type ) const;

    private slots:
        void newResultReady( const QString &collectionId, const Meta::TrackList &tracks );
        void queryDone();

    private:
        class Private;
        Private * const d;
};


#endif
