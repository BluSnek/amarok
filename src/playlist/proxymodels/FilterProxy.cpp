/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
 * Copyright (c) 2010 Nanno Langstraat <langstr@gmail.com>                              *
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

#include "FilterProxy.h"

#include "Amarok.h"
#include "amarokconfig.h"

namespace Playlist {

FilterProxy::FilterProxy( AbstractModel *belowModel, QObject *parent )
    : ProxyBase( belowModel, parent )
{
    setDynamicSortFilter( true );    // Tell QSortFilterProxyModel: keep the filter correct when the underlying source model changes.

    KConfigGroup config = Amarok::config("Playlist Search");
    m_showOnlyMatches = config.readEntry( "ShowOnlyMatches", true );
}

FilterProxy::~FilterProxy()
{
}

bool FilterProxy::filterAcceptsRow( int source_row, const QModelIndex & source_parent ) const
{
    Q_UNUSED( source_parent );

    if ( m_showOnlyMatches ) {
        return matchesCurrentSearchTerm( source_row );
    } else {
        return true;
    }
}

void FilterProxy::filterUpdated()
{
    if ( m_showOnlyMatches )
        invalidateFilter();    // Tell QSortFilterProxyModel: re-filter
    //else
    //  Search criteria are not being used for filtering, so we can ignore the update
}

int
FilterProxy::find( const QString &searchTerm, int searchFields )
{
    m_currentSearchTerm = searchTerm;
    m_currentSearchFields = searchFields;

    // Don't call 'filterUpdated()': our client must do that as part of the API.
    // This allows client 'PrettyListView' to give the user the time to type a few
    // characters before we do a filter run that might block for a few seconds.

    return -1;
}

void
FilterProxy::showOnlyMatches( bool onlyMatches )
{
    m_showOnlyMatches = onlyMatches;

    //make sure to update model when mode changes ( as we might have ignored any
    //number of changes to the search term )
    invalidateFilter();    // Tell QSortFilterProxyModel: re-filter.
}

void FilterProxy::clearSearchTerm()
{
    m_currentSearchTerm.clear();
    m_currentSearchFields = 0;
    m_belowModel->clearSearchTerm();

    // Don't call 'filterUpdated()': our client must do that as part of the API.
    // This allows client 'PrettyListView' to give the user the time to type a few
    // characters before we do a filter run that might block for a few seconds.
}

bool
FilterProxy::matchesCurrentSearchTerm( int sourceModelRow ) const
{
    if ( m_currentSearchTerm.isEmpty() )
        return true;

    return rowMatch( sourceModelRow, m_currentSearchTerm, m_currentSearchFields );
}

}

#include "FilterProxy.moc"
