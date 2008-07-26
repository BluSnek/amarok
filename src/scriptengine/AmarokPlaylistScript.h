/******************************************************************************
 * Copyright (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                     *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#ifndef AMAROK_PLAYLIST_SCRIPT_H
#define AMAROK_PLAYLIST_SCRIPT_H

#include <KUrl>

#include <QObject>
#include <QtScript>

namespace AmarokScript
{

    struct TrackMeta
    {
        int sampleRate;
        int bitrate;
        double score;
        int rating;
        bool inCollection;
        QString type;
        int Length;
        int fileSize;
        int trackNumber;
        int discNumber;
        int playCount;
        int playable;
        QString album;
        QString artist;
        QString composer;
        QString genre;
        QString year;
        QString comment;
        QString path;
    };

    class AmarokPlaylistScript : public QObject
    {
        Q_OBJECT

        public:
            AmarokPlaylistScript( QScriptEngine* ScriptEngine );
            ~AmarokPlaylistScript();
        public slots:
            int getActiveIndex();
            int getTotalTrackCount();
            QString saveCurrentPlaylist();
            void addMedia( const KUrl &url );
            void addMediaList( const KUrl::List &urls );
            void clearPlaylist();
            void playByIndex( int index );
            void playMedia( const KUrl &url );
            void removeCurrentTrack();
            void removeByIndex( int index );
            void savePlaylist( const QString& path );
            void setStopAfterCurrent( bool on );
            void togglePlaylist();
            QStringList filenames();
            TrackMeta TrackInfo( int index );
        signals:
            void CountChanged( int newCount );
            void GroupingChanged();
            void rowMoved( int from, int to );
            void activeRowChanged( int from, int to );
            void activeRowExplicitlyChanged( int from, int to );

    };
}

Q_DECLARE_METATYPE( AmarokScript::TrackMeta )
#endif
