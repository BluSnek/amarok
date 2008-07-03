/******************************************************************************
 * Copyright (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>        *
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

#ifndef COLLECTIONTREEVIEW_H
#define COLLECTIONTREEVIEW_H

#include "CollectionSortFilterProxyModel.h"
#include "CollectionTreeItem.h"
#include "playlist/PlaylistModel.h"

#include <QSet>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QTreeView>

class QSortFilterProxyModel;
class CollectionSortFilterProxyModel;
class CollectionTreeItemModelBase;
class PopupDropper;
class PopupDropperAction;


typedef QList<PopupDropperAction *> PopupDropperActionList;

class CollectionTreeView: public QTreeView
{
        Q_OBJECT

    public:
        CollectionTreeView( QWidget *parent = 0 );
        ~CollectionTreeView();

        QSortFilterProxyModel* filterModel() const;

        AMAROK_EXPORT void setLevels( const QList<int> &levels );
        void setLevel( int level, int type );

        void setModel ( QAbstractItemModel * model );
        void contextMenuEvent(QContextMenuEvent* event);

        void setShowYears( bool show ) { m_showYears = show; }
        const bool showYears() const { return m_showYears; }

        void setShowTrackNumbers( bool show ) { m_showTrackNumbers = show; }
        const bool showTrackNumbers() const { return m_showTrackNumbers; }

    public slots:
        void slotSetFilterTimeout();

        /**
         * Bypass the filter timeout if we really need to start filtering *now*
         */
        void slotFilterNow();

    protected:
        void mousePressEvent( QMouseEvent *event );
        void mouseReleaseEvent( QMouseEvent *event );
        void mouseDoubleClickEvent( QMouseEvent *event );
        void startDrag(Qt::DropActions supportedActions);
        //void changeEvent ( QEvent * event );
    protected slots:
        virtual void selectionChanged ( const QItemSelection & selected, const QItemSelection & deselected );
        void slotExpand( const QModelIndex &index );
        void slotCollapsed( const QModelIndex &index );

        void slotPlayChildTracks();
        void slotAppendChildTracks();
        void slotEditTracks();
        void slotCopyTracks();
        void slotMoveTracks();
        void slotOrganize();
        void newPalette( const QPalette & palette );

    private:
        // Utility function to play all items
        // that have this as a parent..
        void playChildTracks( CollectionTreeItem *item, Playlist::AddOptions insertMode ) const;
        void playChildTracks( const QSet<CollectionTreeItem*> &items, Playlist::AddOptions insertMode ) const;
        void editTracks( const QSet<CollectionTreeItem*> &items ) const;
        void organizeTracks( const QSet<CollectionTreeItem*> &items ) const;
        void copyTracks( const QSet<CollectionTreeItem*> &items, Collection *destination, bool removeSources ) const;
        PopupDropperActionList getActions( const QModelIndexList &indcies );

        bool onlyOneCollection(  const QModelIndexList &indcies );
        Collection * getCollection( const QModelIndexList &indcies );
        QHash<PopupDropperAction*, Collection*> getCopyActions( const QModelIndexList &indcies );
        QHash<PopupDropperAction*, Collection*> getMoveActions( const QModelIndexList &indcies );
        
        CollectionSortFilterProxyModel *m_filterModel;
        CollectionTreeItemModelBase *m_treeModel;
        QTimer m_filterTimer;
        QPoint m_dragStartPosition;
        bool m_showTrackNumbers;
        bool m_showYears;
        PopupDropper* m_pd;
        PopupDropperAction* m_appendAction;
        PopupDropperAction* m_loadAction;
        PopupDropperAction* m_editAction;
        PopupDropperAction* m_organizeAction;

        PopupDropperAction * m_caSeperator;
        PopupDropperAction * m_cmSeperator;


        QHash<PopupDropperAction*, Collection*> m_currentCopyDestination;
        QHash<PopupDropperAction*, Collection*> m_currentMoveDestination;

        QSet<CollectionTreeItem*> m_currentItems;

    signals:
        void itemSelected( CollectionTreeItem * item );
};

#endif
