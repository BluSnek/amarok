/****************************************************************************************
 * Copyright (c) 2009 Nathan Sala <sala.nathan@gmail.com>                               *
 * Copyright (c) 2010 Ludovic Deveaux <deveaux.ludovic31@gmail.com>                     *
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

#include "LastFmEvent.h"

/**
 * Creates an empty LastFmEvent
 */
LastFmEvent::LastFmEvent()
{
    static bool metaTypeRegistered = false;
    if (!metaTypeRegistered)
    {
        qRegisterMetaType<LastFmEvent>("LastFmEvent");
        metaTypeRegistered = true;
    }
}

/**
 * Creates a new LastFmEvent with the 'event' attributes
 */
LastFmEvent::LastFmEvent( const LastFmEvent& event)
{
    foreach( QString currentArtist, event.m_artists )
    {
        m_artists.append(currentArtist);    
    }
    m_name = event.m_name;
    m_date = event.m_date;
    m_smallImageUrl = event.m_smallImageUrl;
    m_url = event.m_url;
    m_location = event.m_location;
}

/**
 * Destroys a LastFmEvent instance
 */
LastFmEvent::~LastFmEvent() {}

QStringList LastFmEvent::artists() const
{
    return m_artists;
}

QDateTime LastFmEvent::date() const
{
    return m_date;
}

QString LastFmEvent::name() const
{
    return m_name;
}

QString LastFmEvent::location() const
{
    return m_location;
}

KUrl LastFmEvent::smallImageUrl() const
{
    return m_smallImageUrl;
}

KUrl LastFmEvent::url() const
{
    return m_url;
}

void LastFmEvent::setArtists( QStringList artists )
{
    m_artists = artists;
}

void LastFmEvent::setDate( QDateTime date )
{
    m_date = date;
}

void LastFmEvent::setName( QString name )
{
    m_name = name;
}

void LastFmEvent::setLocation( QString location )
{
    m_location = location;
}

void LastFmEvent::setSmallImageUrl( KUrl smallImageUrl )
{
    m_smallImageUrl = smallImageUrl;
}

void LastFmEvent::setUrl( KUrl url )
{
    m_url = url;
}
