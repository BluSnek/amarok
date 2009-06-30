/***************************************************************************
 *   Copyright © 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef AMAROK_PROXYBASE_H
#define AMAROK_PROXYBASE_H

#include "AbstractModel.h"
#include "playlist/PlaylistItem.h"

#include <QSortFilterProxyModel>

namespace Playlist
{

/**
 * A ProxyModel that implements all the common forwarders for the interface of any
 * playlist proxy.
 * @author Téo Mrnjavac <teo.mrnjavac@gmail.com>
 */
class ProxyBase : public QSortFilterProxyModel, public AbstractModel
{
    Q_OBJECT
public:
    /**
     * Constructor.
     */
    ProxyBase( QObject *parent = 0 );

    /**
     * Destructor.
     */
    virtual ~ProxyBase();

// Common public forwarder methods that pretty much just forward stuff through the stack of
// proxies start here.
// Please keep them sorted alphabetically.  -- Téo

    /**
     * Returns the currently active row, translated to proxy rows
     * (or -1 if the current row is not represented by this proxy).
     * @return The currently active (playing) row in proxy terms.
     */
    virtual int activeRow() const;

   /**
     * Returns the number of columns exposed by the current proxy.
     * The default implementation forwards the column count of the model below it.
     * @param parent the parent of the columns to count.
     * @return the number of columns.
     */
    virtual int columnCount( const QModelIndex& i ) const;

    /**
     * Clears the current search term.
     */
    virtual void clearSearchTerm();

    /**
     * Get the current search fields bitmask.
     * @return The current search fields.
     */
    virtual int currentSearchFields();

    /**
     * Get the current search term.
     * @return The curent search term.
     */
    virtual QString currentSearchTerm();

    /**
     * Forwards the request down the proxy stack and gets the data at an index.
     * @param index the index for which to retrieve the data from the model.
     * @return the data from the model.
     */
    virtual QVariant data( const QModelIndex& index, int role ) const;

    /**
     * Handles the data supplied by a drag and drop operation that ended with the given
     * action.
     * @param data the MIME data.
     * @param action the drop action of the current drag and drop operation.
     * @param row the row where the operation ended.
     * @param column the column where the operation ended.
     * @param parent the parent index.
     * @return true if the data and action can be handled by the model; otherwise false.
     */
    virtual bool dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);

    /**
     * Notify FilterProxy that the search term of searched fields has changed. Since this
     * call does not use the parent's filter values, this method needs to be called when the
     * values change.
     */
    virtual void filterUpdated();

    /**
     * Forwards a search down through the stack of ProxyModels.
     * Find the first track in the playlist that matches the search term in one of the
     * specified search fields. Playlist::Model::find() emits found() or notFound() depending
     * on whether a match is found.
     * @param searchTerm The term to search for.
     * @param searchFields A bitmask specifying the fields to look in.
     * @return The row of the first found match, -1 if no match is found.
     */
    virtual int find( const QString &searchTerm, int searchFields );

    /**
     * Forwards through the stack of ProxyModels a top to bottom search for the next item.
     * Find the first track below a given row that matches the search term in one of the
     * specified search fields. Playlist::Model::findNext() emits found() or notFound()
     * depending on whether a match is found. If no row is found below the current row, the
     * function wraps around and returns the first match. If no match is found at all, -1
     * is returned.
     * @param searchTerm The term to search for.
     * @param selectedRow The offset row.
     * @param searchFields A bitmask specifying the fields to look in.
     * @return The row of the first found match below the offset, -1 if no match is found.
     */
    virtual int findNext( const QString &searchTerm, int selectedRow, int searchFields );

    /**
     * Forwards through the stack of ProxyModels a bottom to top search for the next item.
     * Find the first track above a given row that matches the search term in one of the
     * specified search fields. Playlist::Model::findPrevious() emits found() or notFound()
     * depending on whether a match is found. If no row is found above the current row, the
     * function wraps around and returns the last match. If no match is found at all, -1
     * is returned.
     * @param searchTerm The term to search for.
     * @param selectedRow The offset row.
     * @param searchFields A bitmask specifying the fields to look in.
     * @return The row of the first found match above the offset, -1 if no match is found.
     */
    virtual int findPrevious( const QString &searchTerm, int selectedRow, int searchFields );

    /**
     * Returns the item flags for the given index.
     * @param index the index to retrieve the flags for.
     * @return the item flags.
     */
    virtual Qt::ItemFlags flags( const QModelIndex& index ) const;

    /**
     * Returns an object that contains serialized items of data corresponding to the list
     * of indexes specified.
     * @param indexes a list of indexes.
     * @return the MIME data corresponding to the indexes.
     */
    virtual QMimeData* mimeData( const QModelIndexList &indexes ) const;

    /**
     * Returns a list of MIME types that can be used to describe a list of model indexes.
     * @return a QStringList of MIME types.
     */
    virtual QStringList mimeTypes() const;

    /**
     * Returns the number of rows exposed by the current proxy.
     * The default implementation forwards the row count of the model below it.
     * @param parent the parent of the rows to count.
     * @return the number of rows.
     */
    virtual int rowCount( const QModelIndex &parent = QModelIndex() ) const;

    /**
     * Checks if a row exists in the current proxy.
     * @param row the row in the proxy.
     * @return true is the row exists, otherwise false.
     */
    virtual bool rowExists( int row ) const;

    /**
     * Sets the currently active (playing) row, translated for the current proxy.
     * @param row the row to be set as active.
     */
    virtual void setActiveRow( int row );

    /**
     * Returns the drop actions supported by this proxy.
     * The default implementation returns the drop actions supported by the proxy or model
     * below the current proxy.
     * @return the drop actions.
     */
    virtual Qt::DropActions supportedDropActions() const;

    /**
     * Returns the total length of the playlist.
     * The default implementation forwards the total time from the proxy or model below the
     * current proxy.
     * @return the total length of the playlist.
     */
    virtual int totalLength() const;

    /**
     * Returns a pointer to the track at a given row in the current proxy.
     * @param row the row to return the track pointer for.
     * @return a pointer to the track at the given row.
     */
    virtual Meta::TrackPtr trackAt( int row ) const;


//FIXME: When every proxy talks only to the proxy below it, these should be made protected
//       here and and in subclasses that reimplement them. For now, they have to be public
//       otherwise it won't compile.    -- Téo 21/6/2009
//protected:
    /**
     * Converts a row index that's valid in the proxy below this one to a row index valid
     * in this proxy.
     * The default implementation returns the same row, and results in a perfectly pass-
     * -through proxy. Reimplement this method with mapFromSource and sanity checks if your
     * proxy adds, removes or sorts rows.
     * @param row the row index to be converted.
     * @return the index of the row that's valid in this proxy.
     */
    virtual inline int rowFromSource( int row ) const
    { return row; }

    /**
     * Converts a row index that's valid in this proxy to a row index valid in the proxy
     * below this one.
     * The default implementation returns the same row, and results in a perfectly pass-
     * -through proxy. Reimplement this method with mapToSource and sanity checks if your
     * proxy adds, removes or sorts rows.
     * @param row the row index to be converted.
     * @return the index of the row that's valid in the proxy below this one.
     */
    virtual inline int rowToSource( int row ) const
    { return row; }

signals:
    /**
     * Signal forwarded from the source model. IDs are unique so they shouldn't be modified
     * by the proxies.
     * @param the list of id's added that are also represented by this proxy.
     */
    void insertedIds( const QList<quint64>& );

    /**
     * Signal forwarded from the source model. IDs are unique so they shouldn't be modified
     * by the proxies.
     * @param the list of id's removed that are also represented by this proxy.
     */
    void removedIds( const QList<quint64>& );

protected:
    AbstractModel *m_belowModel;
};

}   //namespace Playlist

#endif  //AMAROK_PROXYBASE_H
