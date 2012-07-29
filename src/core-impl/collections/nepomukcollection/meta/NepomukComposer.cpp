/****************************************************************************************
 * Copyright (c) 2008 Daniel Winter <dw@danielwinter.de>                                *
 * Copyright (c) 2012 Phalgun Guduthur <me@phalgun.in>                                  *
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

#include "NepomukComposer.h"

#include "core/meta/Meta.h"

#include <QString>

using namespace Meta;

NepomukComposer::NepomukComposer( const QString &name )
    : Meta::Composer()
    , m_name( name )
{

}

TrackList
NepomukComposer::tracks()
{
    return m_tracks;
}

QString
NepomukComposer::name() const
{
    return m_name;
}

void
NepomukComposer::addTrack( const TrackPtr trackPtr )
{
    m_tracks.append( trackPtr );
}
