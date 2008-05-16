/***************************************************************************
 *   Copyright (C) 2005 Max Howell <max.howell@methylblue.com>             *
 *             (C) 2004 Frederik Holljen <fh@ez.no>                        *
 *             (C) 2005 Gábor Lehel <illissius@gmail.com>                  *
 *             (C) 2007 Seb Ruiz <ruiz@kde.org>                            *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/

#include "ContextStatusBar.h"

#include "amarokconfig.h"
#include "Amarok.h"
#include "Debug.h"
#include "EngineController.h"
#include "meta/Meta.h"
#include "meta/MetaUtility.h"
#include "meta/SourceInfoCapability.h"
#include "TheInstances.h"

#include <KHBox>
#include <KIconLoader>
#include <KLocale>

#include <QLabel>
#include <QLayout>
#include <QTextDocument>
#include <QTimer>

// stuff that must be included last
//#include "startupTips.h"
#include "timeLabel.h"


using namespace Amarok;


KAction *action( const char *name ) { return (KAction*)Amarok::actionCollection()->action( name ); }

ContextStatusBar* ContextStatusBar::s_instance = 0;

ContextStatusBar::ContextStatusBar( QWidget *parent, const char *name )
        : KDE::StatusBar( parent, name )
        , EngineObserver( The::engineController() )
{
    s_instance = this; //static member
    setSizeGripEnabled( false );
}

void
ContextStatusBar::engineStateChanged( Phonon::State state, Phonon::State /*oldState*/ )
{
    switch ( state ) {
    case Phonon::StoppedState:
    case Phonon::LoadingState:
        setMainText( QString() );
        break;

    case Phonon::PausedState:
        m_mainTextLabel->setText( i18n( "Amarok is paused" ) ); // display TEMPORARY message
        break;

    case Phonon::PlayingState:
        resetMainText(); // if we were paused, this is necessary
        break;

    case Phonon::ErrorState:
    case Phonon::BufferingState:
        break;

    }
}

void
ContextStatusBar::engineNewTrackPlaying()
{
    Meta::TrackPtr track = The::engineController()->currentTrack();

    if( !track )
        return;

    QString title       = Qt::escape( track->name() );
    QString prettyTitle = Qt::escape( track->prettyName() );
    QString artist      = track->artist() ? Qt::escape( track->artist()->name() ) : QString();
    QString album       = track->album() ? Qt::escape( track->album()->name() ) : QString();
    QString length      = Qt::escape( Meta::secToPrettyTime( track->length() ) );

    if ( artist == "Mike Oldfield" && title == "Amarok" ) {
        longMessage( i18n(
                "<p>One of Mike Oldfield's best pieces of work, Amarok, inspired the name behind "
                "the audio-player you are currently using. Thanks for choosing Amarok!</p>"
                "<p align=right>Mark Kretschmann<br/>Max Howell<br/>Chris Muehlhaeuser<br/>"
                "The many other people who have helped make Amarok what it is</p>" ), KDE::StatusBar::Information );
    }

    // ugly because of translation requirements
    if( !title.isEmpty() && !artist.isEmpty() && !album.isEmpty() )
        title = i18nc( "track by artist on album", "<b>%1</b> by <b>%2</b> on <b>%3</b>", title, artist, album );

    else if( !title.isEmpty() && !artist.isEmpty() )
        title = i18nc( "track by artist", "<b>%1</b> by <b>%2</b>", title, artist );

    else if( !album.isEmpty() )
        // we try for pretty title as it may come out better
        title = i18nc( "track on album", "<b>%1</b> on <b>%2</b>", prettyTitle, album );
    else
        title = "<b>" + prettyTitle + "</b>";

    if( title.isEmpty() )
        title = i18n( "Unknown track" );


    // check if we have any source info:
    hideMainTextIcon();

    Meta::SourceInfoCapability *sic = track->as<Meta::SourceInfoCapability>();
    if( sic )
    {
        //is the source defined
        QString source = sic->sourceName();
        if ( !source.isEmpty() ) {
            title += ' ' + i18n("from") + " <b>" + source + "</b>";
            setMainTextIcon( sic->emblem() );
        }

        delete sic;

    }

    // don't show '-' or '?'
    if( length.length() > 1 ) {
        title += " (";
        title += length;
        title += ')';
    }

    setMainText( i18n( "Playing: %1", title ) );
}

///////////////////
//MessageQueue
///////////////////

MessageQueue::MessageQueue()
    : m_queueMessages(true)
{}
MessageQueue*
MessageQueue::instance()
{
    static MessageQueue mq;
    return &mq;
}

void
MessageQueue::addMessage(const QString& message)
{
    if(m_queueMessages)
        m_messages.push(message);
    else
        ContextStatusBar::instance()->longMessage(message);
}

void
MessageQueue::sendMessages()
{
     m_queueMessages = false;
     while(! m_messages.isEmpty())
     {
        ContextStatusBar::instance()->longMessage(m_messages.pop());
     }
}


namespace The {
    AMAROK_EXPORT Amarok::ContextStatusBar* statusBar() { return Amarok::ContextStatusBar::instance(); }
}

