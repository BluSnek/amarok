/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2009 Teo Mrnjavac <teo.mrnjavac@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "SortProxy.h"

#include "Debug.h"
#include "SortAlgorithms.h"

namespace Playlist
{
//To 7/5/2009: Attention coding style police guys: this is very WiP and if you see notes
//to self and debug spam here pretty please let it be until I remove it.

SortProxy* SortProxy::s_instance = 0;

SortProxy*
SortProxy::instance()
{
    if ( s_instance == 0 )
        s_instance = new SortProxy();
    return s_instance;
}

SortProxy::SortProxy()
    : ProxyBase()
{
    DEBUG_BLOCK
    debug() << "Instantiating SortProxy";
    m_belowModel = FilterProxy::instance();
    setSourceModel( ( FilterProxy * )m_belowModel );
    setDynamicSortFilter( false );

    //As this Proxy doesn't add or remove tracks, and unique track IDs must be left untouched
    //by sorting, they may be just blindly forwarded
    connect( sourceModel(), SIGNAL( insertedIds( const QList<quint64>& ) ), this, SIGNAL( insertedIds( const QList< quint64>& ) ) );
    connect( sourceModel(), SIGNAL( removedIds( const QList<quint64>& ) ), this, SIGNAL( removedIds( const QList< quint64 >& ) ) );

    //needed by GroupingProxy:
    connect( sourceModel(), SIGNAL( layoutChanged() ), this, SIGNAL( layoutChanged() ) );
    connect( sourceModel(), SIGNAL( filterChanged() ), this, SIGNAL( filterChanged() ) );
    connect( sourceModel(), SIGNAL( modelReset() ), this, SIGNAL( modelReset() ) );
}

SortProxy::~SortProxy()
{}

bool
SortProxy::lessThan( const QModelIndex & left, const QModelIndex & right ) const
{
    int rowA = left.row();
    int rowB = right.row();
    multilevelLessThan mlt = multilevelLessThan( sourceModel(), m_scheme );
    return mlt( rowA, rowB );
}

void
SortProxy::updateSortMap( SortScheme *scheme)
{
    emit layoutAboutToBeChanged();  //NOTE to self: do I need this or sort() takes care of it?
    m_scheme = scheme;
    sort( 0 );  //0 is a dummy column
    emit layoutChanged();
}


// Pass-through public methods, basically identical to those in Playlist::FilterProxy, that
// pretty much just forward stuff through the stack of proxies start here.
// Please keep them sorted alphabetically.  -- To

void
SortProxy::clearSearchTerm()
{
    m_belowModel->clearSearchTerm();
}

int
SortProxy::currentSearchFields()
{
    return m_belowModel->currentSearchFields();
}

QString
SortProxy::currentSearchTerm()
{
    return m_belowModel->currentSearchTerm();
}

QVariant
SortProxy::data( const QModelIndex & index, int role ) const
{
    //HACK around incomplete index causing a crash...
    //note to self by To: is this still needed?
    QModelIndex newIndex = this->index( index.row(), index.column() );

    QModelIndex sourceIndex = mapToSource( newIndex );
    return m_belowModel->data( sourceIndex, role );
}

bool
SortProxy::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent )
{
    return m_belowModel->dropMimeData( data, action, row, column, parent ); //TODO: this might need rowToSource
}

void
SortProxy::filterUpdated()
{
    FilterProxy::instance()->filterUpdated();
    //was:
    //m_belowModel->filterUpdated();
}

int
SortProxy::find( const QString &searchTerm, int searchFields )
{
    return rowFromSource( m_belowModel->find( searchTerm, searchFields ) );
}

int
SortProxy::findNext( const QString &searchTerm, int selectedRow, int searchFields )
{
    return rowFromSource( m_belowModel->findNext( searchTerm, selectedRow, searchFields ) );
}

int
SortProxy::findPrevious( const QString &searchTerm, int selectedRow, int searchFields )
{
    return rowFromSource( m_belowModel->findPrevious( searchTerm, selectedRow, searchFields ) );
}

Qt::ItemFlags
SortProxy::flags( const QModelIndex &index ) const
{
    //FIXME: This call is the same in all proxies but I think it should use a mapToSource()
    //       every time. Needs to be checked.       --To
    return m_belowModel->flags( index );
}

QMimeData *
SortProxy::mimeData( const QModelIndexList &indexes ) const
{
    return m_belowModel->mimeData( indexes );     //TODO: probably needs mapToSource!!!
}

QStringList
SortProxy::mimeTypes() const
{
    return m_belowModel->mimeTypes();
}

int
SortProxy::rowCount(const QModelIndex& parent) const
{
    return m_belowModel->rowCount( parent );
}

bool
SortProxy::rowExists( int row ) const
{
    QModelIndex index = this->index( row, 0 );
    return index.isValid();
}

int
SortProxy::rowFromSource( int row ) const
{
    QModelIndex sourceIndex = sourceModel()->index( row, 0 );
    QModelIndex index = mapFromSource( sourceIndex );

    if ( !index.isValid() )
        return -1;
    return index.row();
}

int
SortProxy::rowToSource( int row ) const
{
    QModelIndex index = this->index( row, 0 );
    QModelIndex sourceIndex = mapToSource( index );

    if ( !sourceIndex.isValid() )
        return -1;
    return sourceIndex.row();
}

void
SortProxy::setActiveRow( int row )
{
    m_belowModel->setActiveRow( rowToSource( row ) );
}

Qt::DropActions
SortProxy::supportedDropActions() const
{
    return m_belowModel->supportedDropActions();
}

int
SortProxy::totalLength() const
{
    return m_belowModel->totalLength();
}

Meta::TrackPtr
SortProxy::trackAt(int row) const
{
    return m_belowModel->trackAt( rowToSource( row ) );
}

}   //namespace Playlist
