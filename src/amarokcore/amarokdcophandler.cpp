/***************************************************************************
                         amarokdcophandler.cpp  -  DCOP Implementation
                            -------------------
   begin                : Sat Oct 11 2003
   copyright            : (C) 2003 by Stanislav Karchebny
   email                : berkus@users.sf.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "amarokconfig.h"
#include "amarokdcophandler.h"
#include "app.h"
#include "engine/enginebase.h"
#include "enginecontroller.h"
#include "playlist.h"
#include "osd.h"

#include <dcopclient.h>

namespace amaroK
{

    DcopHandler::DcopHandler()
        : DCOPObject( "player" )
    {
        // Register with DCOP
        if ( !kapp->dcopClient() ->isRegistered() ) {
            kapp->dcopClient() ->registerAs( "amarok", false );
            kapp->dcopClient() ->setDefaultObject( objId() );
        }
    }

    void DcopHandler::play()
    {
        EngineController::instance() ->play();
    }

    void DcopHandler::playPause()
    {
        EngineController::instance() ->playPause();
    }

    void DcopHandler::stop()
    {
        EngineController::instance() ->stop();
    }


    void DcopHandler::next()
    {
        EngineController::instance() ->next();
    }


    void DcopHandler::prev()
    {
        EngineController::instance() ->previous();
    }


    void DcopHandler::pause()
    {
        EngineController::instance()->pause();
    }

    bool DcopHandler::isPlaying()
    {
        return EngineController::engine()->state() == EngineBase::Playing;
    }

// Now for the DCOP output stuff
// I renamed _all_ id3 tag output DCOP calls to "current....." for consistency reasons.
// Also, I replaced the prettyTitle with 2 calls, one for the artist and one for the title (more flexible)
   
    QString DcopHandler::currentArtist()
    {
        return EngineController::instance()->bundle().artist();
    }
    
    QString DcopHandler::currentTitle()
    {
        return EngineController::instance()->bundle().title();
    }
    
    QString DcopHandler::currentAlbum()
    {
        return EngineController::instance()->bundle().album();
    }
    
// Changed DCOP time output to mm:ss, by using MetaBundle::prettyLength ;-)
// prettyLength also adds an "0" when sec < 10
    
    QString DcopHandler::currentTotalTime()
    {
        return MetaBundle::prettyLength( EngineController::instance()->bundle().length() );
    }

    QString DcopHandler::currentPosition()
    {
        return MetaBundle::prettyLength( EngineController::engine() ->position() / 1000 );
    }

// Some additional DCOP output, very useful e.g. for IRC-scripts

    QString DcopHandler::currentGenre()
    {
        return EngineController::instance()->bundle().genre();
    }

    QString DcopHandler::currentYear()
    {
        return EngineController::instance()->bundle().year();
    }
        
    QString DcopHandler::currentComment()
    {
        return EngineController::instance()->bundle().comment();
    }

    QString DcopHandler::currentBitrate()
    {
        return EngineController::instance()->bundle().prettyBitrate();
    }

// Ok, that should be enough, have fun :-)
    
    void DcopHandler::seek(int s)
    {
        EngineBase* const engine = EngineController::engine();
        if ( s > 0 && engine->state() != EngineBase::Empty )
        {
            engine ->seek( s * 1000 );
        }
    }

    void DcopHandler::addMedia(const KURL &url)
    {
        pApp->playlist()->insertMedia(url);
    }

    void DcopHandler::addMediaList(const KURL::List &urls)
    {
        pApp->playlist()->insertMedia(urls);
    }

    void DcopHandler::setVolume(int volume)
    {
        EngineController::instance()->setVolume(volume);
    }

    void DcopHandler::volumeUp()
    {
        EngineController::instance()->increaseVolume();
    }

    void DcopHandler::volumeDown()
    {
        EngineController::instance()->decreaseVolume();
    }

    void DcopHandler::enableOSD(bool enable)
    {
        pApp->osd()->setEnabled(enable);
        AmarokConfig::setOsdEnabled(enable);
    }

} //namespace amaroK

#include "amarokdcophandler.moc"
