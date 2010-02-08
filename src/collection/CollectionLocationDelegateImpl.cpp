/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "CollectionLocationDelegateImpl.h"

#include <KLocale>
#include <KMessageBox>

bool
CollectionLocationDelegateImpl::reallyDelete( CollectionLocation *loc, const Meta::TrackList &tracks ) const
{
    QStringList files;
    foreach( Meta::TrackPtr track, tracks )
        files << track->prettyUrl();

    // NOTE: taken from SqlCollection
    // TODO put the delete confirmation code somewhere else?
    const QString text( i18ncp( "@info", "Do you really want to delete this track? It will be removed from disk as well as your collection.",
                                "Do you really want to delete these %1 tracks? They will be removed from disk as well as your collection.", tracks.count() ) );
    const bool del = KMessageBox::warningContinueCancelList(0,
                                                     text,
                                                     files,
                                                     i18n("Delete Files"),
                                                     KStandardGuiItem::del() ) == KMessageBox::Continue;

    return del;
}

bool CollectionLocationDelegateImpl::errorDeleting( CollectionLocation* loc, const Meta::TrackList& tracks ) const
{
    QStringList files;
    foreach( Meta::TrackPtr track, tracks )
        files << track->prettyUrl();
    const QString text( i18ncp( "@info", "There was a problem and this track could not be removed. Make sure the directory is writeable.",
                                "There was a problem and %1 tracks could not be removed. Make sure the directory is writeable.", files.count() ) );
                                KMessageBox::informationList(0,
                                                             text,
                                                             files,
                                                             i18n("Unable to be removed tracks") );
    return false;
}

