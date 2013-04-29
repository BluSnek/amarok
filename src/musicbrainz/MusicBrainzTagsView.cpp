/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
 * Copyright (c) 2013 Alberto Villa <avilla@FreeBSD.org>                                *
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

#include "MusicBrainzTagsView.h"

#include "core/support/Debug.h"
#include "MusicBrainzTagsModel.h"

#include <KActionMenu>
#include <KLocalizedString>
#include <KStandardDirs>

#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QMenu>
#include <QSortFilterProxyModel>

#define DEBUG_PREFIX "MusicBrainzTagsView"

MusicBrainzTagsView::MusicBrainzTagsView( QWidget *parent )
    : QTreeView( parent )
{
    m_artistIcon = KIcon( KStandardDirs::locate( "data", "amarok/images/mb_aicon.png" ) );
    m_releaseIcon = KIcon( KStandardDirs::locate( "data", "amarok/images/mb_licon.png" ) );
    m_trackIcon = KIcon( KStandardDirs::locate( "data", "amarok/images/mb_ticon.png" ) );
}

MusicBrainzTagsModel *
MusicBrainzTagsView::sourceModel() const
{
    QSortFilterProxyModel *model = qobject_cast<QSortFilterProxyModel *>( this->model() );
    if( !model )
        return 0;

    MusicBrainzTagsModel *sourceModel = qobject_cast<MusicBrainzTagsModel *>( model->sourceModel() );
    return sourceModel;
}

void
MusicBrainzTagsView::contextMenuEvent( QContextMenuEvent *event )
{
    QModelIndex index = indexAt( event->pos() );
    if( !index.isValid() || !index.internalPointer() )
    {
        event->ignore();
        return;
    }

    QAbstractItemModel *model = this->model();
    if( !model )
        return;

    if( ~index.flags() & Qt::ItemIsUserCheckable )
    {
        event->ignore();
        return;
    }

    QMenu *menu = new QMenu( this );
    QList<QAction *> actions;

    if( model->rowCount() > 1 && !index.data( MusicBrainzTagsModel::ReleasesRole ).isNull() )
    {
        QAction *action = new QAction( KIcon( "filename-album-amarok" ),
                                       i18n( "Choose Best Matches from This Album" ), menu );
        connect( action, SIGNAL(triggered()), SLOT(chooseBestMatchesFromRelease()) );
        menu->addAction( action );
        menu->addSeparator();
    }

    QVariantMap artists;
    if( !index.data( MusicBrainzTagsModel::ArtistsRole ).toList().isEmpty() )
        artists = index.data( MusicBrainzTagsModel::ArtistsRole ).toList().first().toMap();
    if( !artists.isEmpty() )
    {
        KActionMenu *action = new KActionMenu( m_artistIcon, i18n( "Go to Artist Page" ), menu );
        if( artists.size() > 1 )
        {
            foreach( const QVariant &id, artists.keys() )
            {
                QAction *subAction = new QAction( artists.value( id.toString() ).toString(), action );
                subAction->setData( id );
                connect( subAction, SIGNAL(triggered()), SLOT(openArtistPage()) );
                action->addAction( subAction );
            }
        }
        else
        {
            action->setData( artists.keys().first() );
            connect( action, SIGNAL(triggered()), SLOT(openArtistPage()) );
        }
        actions << action;
    }

    if( !index.data( MusicBrainzTagsModel::ReleasesRole ).toList().isEmpty() )
    {
        QAction *action = new QAction( m_releaseIcon, i18n( "Go to Album Page" ), menu );
        connect( action, SIGNAL(triggered()), SLOT(openReleasePage()) );
        actions << action;
    }

    if( !index.data( MusicBrainzTagsModel::TracksRole ).toList().isEmpty() )
    {
        QAction *action = new QAction( m_trackIcon, i18n( "Go to Track Page" ), menu );
        connect( action, SIGNAL(triggered()), SLOT(openTrackPage()) );
        actions << action;
    }

    if( actions.isEmpty() )
    {
        delete menu;
        event->ignore();
        return;
    }

    menu->addActions( actions );
    menu->exec( event->globalPos() );
    event->accept();
}

void
MusicBrainzTagsView::collapseChosen()
{
    QAbstractItemModel *model = this->model();
    if( !model )
        return;

    for( int i = 0; i < model->rowCount(); i++ )
    {
        QModelIndex index = model->index( i, 0 );
        if( index.isValid() &&
            index.data( MusicBrainzTagsModel::ChosenStateRole ) == MusicBrainzTagsModel::Chosen )
            collapse( index );
    }
}

void
MusicBrainzTagsView::expandUnchosen()
{
    QAbstractItemModel *model = this->model();
    if( !model )
        return;

    for( int i = 0; i < model->rowCount(); i++ )
    {
        QModelIndex index = model->index( i, 0 );
        if( index.isValid() &&
            index.data( MusicBrainzTagsModel::ChosenStateRole ) == MusicBrainzTagsModel::Unchosen )
            expand( index );
    }
}

void
MusicBrainzTagsView::chooseBestMatchesFromRelease() const
{
    QModelIndex index = selectedIndexes().first();
    if( !index.isValid() || !index.internalPointer() )
        return;

    MusicBrainzTagsModel *sourceModel = this->sourceModel();
    if( !sourceModel )
        return;

    QStringList releases = index.data( MusicBrainzTagsModel::ReleasesRole ).toStringList();
    if( releases.isEmpty() )
        return;

    sourceModel->chooseBestMatchesFromRelease( releases );
}

void
MusicBrainzTagsView::openArtistPage() const
{
    QModelIndex index = selectedIndexes().first();
    if( !index.isValid() || !index.internalPointer() )
        return;

    QAction *action = qobject_cast<QAction *>( sender() );
    if( !action )
        return;

    QString artistID = action->data().toString();
    if( artistID.isEmpty() )
        return;

    QString url = QString( "http://musicbrainz.org/artist/%1.html" ).arg( artistID );

    QDesktopServices::openUrl( url );
}

void
MusicBrainzTagsView::openReleasePage() const
{
    QModelIndex index = selectedIndexes().first();
    if( !index.isValid() || !index.internalPointer() )
        return;

    QString releaseID = index.data( MusicBrainzTagsModel::ReleasesRole ).toStringList().first();
    if( releaseID.isEmpty() )
        return;

    QString url = QString( "http://musicbrainz.org/release/%1.html" ).arg( releaseID );

    QDesktopServices::openUrl( url );
}

void
MusicBrainzTagsView::openTrackPage() const
{
    QModelIndex index = selectedIndexes().first();
    if( !index.isValid() || !index.internalPointer() )
        return;

    QString trackID = index.data( MusicBrainzTagsModel::TracksRole ).toStringList().first();
    if( trackID.isEmpty() )
        return;

    QString url = QString( "http://musicbrainz.org/recording/%1.html" ).arg( trackID );

    QDesktopServices::openUrl( url );
}

#include "MusicBrainzTagsView.moc"
