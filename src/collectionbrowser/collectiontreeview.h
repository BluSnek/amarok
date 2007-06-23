/******************************************************************************
 * copyright: (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>       *
 *                                                                            *
 *   This program is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License version 2           *
 *   as published by the Free Software Foundation.                            *
 ******************************************************************************/

#ifndef COLLECTIONTREEVIEW_H
#define COLLECTIONTREEVIEW_H

#include "collectionsortfilterproxymodel.h"
#include "collectionbrowser/collectiontreeitem.h"

#include <QSortFilterProxyModel>
#include <QTimer>
#include <QTreeView>

class QSortFilterProxyModel;
class CollectionSortFilterProxyModel;
class CollectionTreeItemModel;

class CollectionTreeView: public QTreeView {
    Q_OBJECT
    public:
        CollectionTreeView( QWidget *parent = 0 );
        ~CollectionTreeView();
    
        QSortFilterProxyModel* filterModel() const;
    
        void setLevels( const QList<int> &levels );
        void setLevel( int level, int type );
    
        void setModel ( QAbstractItemModel * model );
	    void contextMenuEvent(QContextMenuEvent* event);

    public slots:
        void slotSetFilterTimeout();

    protected:
        void mousePressEvent( QMouseEvent *event );
        void mouseMoveEvent( QMouseEvent *event );

    protected slots:

        virtual void selectionChanged ( const QItemSelection & selected, const QItemSelection & deselected );
        void slotExpand( const QModelIndex &index );
        void slotCollapsed( const QModelIndex &index );

    private:
        CollectionSortFilterProxyModel *m_filterModel;
        CollectionTreeItemModel *m_treeModel;
        QTimer m_filterTimer;
        QPoint m_dragStartPosition;

    signals:

        void itemSelected( CollectionTreeItem * item );
};

#endif
