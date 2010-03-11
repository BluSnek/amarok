/****************************************************************************************
 * Copyright (c) 2004 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2004 Stefan Bogner <bochi@online.ms>                                   *
 * Copyright (c) 2007 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
 * Copyright (c) 2009 Martin Sandsmark <sandsmark@samfundet.no>                         *
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

#ifndef AMAROK_COVERFOUNDDIALOG_H
#define AMAROK_COVERFOUNDDIALOG_H

#include "meta/Meta.h"

#include <KDialog>

#include <QLabel>
#include <QList>
#include <QListWidgetItem>
#include <QObject>
#include <QPixmap>

class KLineEdit;
class KListWidget;
class KPushButton;
class QFrame;
class QGridLayout;

class CoverFoundDialog : public KDialog
{
    Q_OBJECT

public:
    explicit CoverFoundDialog( QWidget *parent,
                               Meta::AlbumPtr album = KSharedPtr< Meta::Album >(),
                               const QList< QPixmap > &covers = QList< QPixmap >() );

    /**
    *   @returns the currently selected cover image
    */
    const QPixmap image() { return m_pixmap; }

signals:
    void newCustomQuery( const QString & );

public slots:
    void add( QPixmap cover );
    void add( QList< QPixmap > covers );

protected:
    void closeEvent( QCloseEvent *event );

private slots:
    void clearView();
    void itemClicked( QListWidgetItem *item );
    void itemDoubleClicked( QListWidgetItem *item );
    void itemMenuRequested( const QPoint &pos );
    void searchButtonPressed();

private:
    void updateGui();
    void updateDetails();
    void updateTitle();

    QFrame         *m_details;       //! Details widget
    QGridLayout    *m_detailsLayout; //! Details widget layout
    KLineEdit      *m_search;        //! Custom search input
    KListWidget    *m_view;          //! View of retreived covers
    KPushButton    *m_save;          //! Save Button

    //! Album associated with the covers
    Meta::AlbumPtr m_album;

    //! Currently selected cover image
    QPixmap m_pixmap;

    Q_DISABLE_COPY( CoverFoundDialog );
};

class CoverFoundItem : public QObject, public QListWidgetItem
{
    Q_OBJECT

public:
    explicit CoverFoundItem( QPixmap pixmap, QListWidget *parent = 0 );
    ~CoverFoundItem() {}

    QPixmap pixmap() const { return m_pixmap; }

public slots:
    /**
     * Opens a pixmap viewer
     */
    void display();

private:
    QPixmap m_pixmap;
};

#endif /* AMAROK_COVERFOUNDDIALOG_H */
