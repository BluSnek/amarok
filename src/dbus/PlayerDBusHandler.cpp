/******************************************************************************
 * Copyright (C) 2008 Ian Monroe <ian@monroe.nu>                              *
 *           (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                     *
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

#include "PlayerDBusHandler.h"

#include "amarokconfig.h"
#include "App.h"
#include "EngineController.h"
#include "meta/Meta.h"
#include "PlayerAdaptor.h"
#include "playlist/PlaylistModel.h"

namespace Amarok
{
    PlayerDBusHandler::PlayerDBusHandler()
        : QObject(kapp)
    {
        new PlayerAdaptor( this );
        setObjectName("PlayerDBusHandler");
//TODO: signal: trackchange, statusChange,capChangeSlot

        QDBusConnection::sessionBus().registerObject("/Player", this);
    }

    //0 = Playing, 1 = Paused, 2 = Stopped.
    DBusStatus PlayerDBusHandler::GetStatus()
    {
        DBusStatus status;
        switch( The::engineController()->state() )
        {
            case Phonon::PlayingState:
            case Phonon::BufferingState:
                status.Play = 0; //Playing
            case Phonon::PausedState:
                status.Play = 1; //Paused
            case Phonon::LoadingState:
            case Phonon::StoppedState:
            case Phonon::ErrorState:
                status.Play = 2; //Stopped
        };
        if ( AmarokConfig::randomMode() )
            status.Random = 1;
        else
            status.Random = 0;
        if ( Amarok::repeatTrack() )
            status.Repeat = 1;
        else
            status.Repeat = 0;
        if ( Amarok::repeatPlaylist() || AmarokConfig::randomMode() )
            status.RepeatPlaylist = 1;
        else
            status.RepeatPlaylist = 0; //the music will not end if we play random
        return status;
    }

    void PlayerDBusHandler::PlayPause()
    {
        The::engineController() ->playPause();
    }

    void PlayerDBusHandler::Pause()
    {
        The::engineController()->pause();
    }

    void PlayerDBusHandler::Play()
    {
        The::engineController() ->play();
    }

    //position is specified in milliseconds
    int PlayerDBusHandler::PositionGet()
    {
        return The::engineController()->trackPosition() * 1000;
    }

    void PlayerDBusHandler::PositionSet( int time )
    {
        if ( time > 0 && The::engineController()->state() != Phonon::StoppedState )
            The::engineController()->seek( time );
    }

    void PlayerDBusHandler::Stop()
    {
        The::engineController()->stop();
    }

    int PlayerDBusHandler::VolumeGet()
    {
        return The::engineController()->volume();
    }

    void PlayerDBusHandler::VolumeSet( int vol )
    {
        The::engineController()->setVolume(vol);
    }

    QVariantMap PlayerDBusHandler::GetMetadata()
    {
        QVariantMap map;
        Meta::TrackPtr track = The::engineController()->currentTrack();
        if( track ) {
            //general meta info:
            map["title"] = track->name(); 
            if( track->artist() )
                map["artist"] = track->artist()->name();
            if( track->album() )
                map["album"] = track->album()->name();
            map["tracknumber"] = track->trackNumber();
            map["time"] = track->length();
            map["mtime"] = track->length() * 1000;
            if( track->genre() )
                map["genre"] = track->genre()->name();
            map["comment"] = track->comment();
            map["rating"] = track->rating()/2;  //out of 5, not 10.
            if( track->year() )
                map["year"] = track->year()->name();
            //TODO: external service meta info:

            //technical meta info:
            map["audio-bitrate"] = track->bitrate();
            map["audio-samplerate"] = track->sampleRate();
            //amarok has no video-bitrate
        }

        return map;
    }

    int PlayerDBusHandler::GetCaps()
    {
        int caps = NONE;
        Meta::TrackPtr track = The::engineController()->currentTrack();
        caps |= CAN_HAS_TRACKLIST;
        if ( track ) caps |= CAN_PROVIDE_METADATA;
        if ( GetStatus().Play == 0 /*playing*/ ) caps |= CAN_PAUSE;
        if ( ( GetStatus().Play == 1 /*paused*/ ) || ( GetStatus().Play == 2 /*stoped*/ ) ) caps |= CAN_PLAY;
        if ( ( GetStatus().Play == 0 /*playing*/ ) || ( GetStatus().Play == 1 /*paused*/ ) ) caps |= CAN_SEEK;
        if ( ( The::playlistModel()->activeRow() >= 0 ) && ( The::playlistModel()->activeRow() <= The::playlistModel()->rowCount() ) )
        {
            caps |= CAN_GO_NEXT;
            caps |= CAN_GO_PREV;
        }
        return caps;
    }

    void PlayerDBusHandler::capsChangeSlot()
    {
        emit CapsChange( GetCaps() );
    }

} // namespace Amarok

#include "PlayerDBusHandler.moc"
