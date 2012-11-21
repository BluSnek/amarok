/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "TrackTuple.h"

using namespace StatSyncing;

TrackTuple::TrackTuple()
{
}

void
TrackTuple::insert( const Provider *provider, const TrackPtr &track )
{
    m_map.insert( provider, track );
}

QList<const Provider *>
TrackTuple::providers() const
{
    return m_map.keys();
}

const Provider *
TrackTuple::provider( int i ) const
{
    return m_map.keys().value( i );
}

TrackPtr
TrackTuple::track( const Provider *provider ) const
{
    Q_ASSERT( m_map.contains( provider ) );
    return m_map.value( provider );
}

int
TrackTuple::count() const
{
    return m_map.count();
}

bool
TrackTuple::isEmpty() const
{
    return m_map.isEmpty();
}
