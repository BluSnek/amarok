/***************************************************************************
 * copyright            : (C) 2008 Bonne Eggletson <b.eggleston@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy 
 * defined in Section 14 of version 3 of the license.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/


#include "PlaylistViewCommon.h"

#include "tagdialog.h"
#include "PlaylistModel.h"
#include "covermanager/CoverFetchingActions.h"
#include "meta/CurrentTrackActionsCapability.h"
#include "popupdropper/PopupDropperAction.h"

#include <QObject>
#include <QModelIndex>

#include <KMenu>
#include <KAction>

//using namespace Playlist::ViewCommon;

void Playlist::ViewCommon::trackMenu( QWidget *parent, const QModelIndex *index, const QPoint &pos, bool coverActions)
{
    Meta::TrackPtr track = index->data( Playlist::ItemRole ).value< Playlist::Item* >()->track();

    KMenu          *menu = new KMenu( parent );
    KAction  *playAction = new KAction( KIcon( "media-playback-start-amarok" ), i18n( "&Play" ), parent );

    QObject::connect( playAction, SIGNAL( triggered() ), parent, SLOT( playTrack() ) );

    menu->addAction( playAction );
  ( menu->addAction( i18n( "Queue Track" ), parent, SLOT( queueItem() ) ) )->setEnabled( false );
  ( menu->addAction( i18n( "Stop Playing After Track" ), parent, SLOT( stopAfterTrack() ) ) )->setEnabled( false );
    menu->addSeparator();
  ( menu->addAction( i18n( "Remove From Playlist" ), parent, SLOT( removeSelection() ) ) )->setEnabled( true );
    menu->addSeparator();
    menu->addAction( i18n( "Edit Track Information" ), parent, SLOT( editTrackInformation() ) );

    //lets see if this is the currently playing tracks, and if it has CurrentTrackActionsCapability
    if( index->data( Playlist::ActiveTrackRole ).toBool() )
    {
        if( track->hasCapabilityInterface( Meta::Capability::CurrentTrackActions ) )
        {
            Meta::CurrentTrackActionsCapability *cac = track->as<Meta::CurrentTrackActionsCapability>();
            if( cac )
            {
                QList<PopupDropperAction *> actions = cac->customActions();

                menu->addSeparator();
                foreach( PopupDropperAction *action, actions )
                    menu->addAction( action );
            }
        }
    }

    if( coverActions )
    {
        Meta::AlbumPtr album = track->album(); 
        if( album )
        {
            Meta::CustomActionsCapability *cac = album->as<Meta::CustomActionsCapability>();
            if( cac )
            {
                QList<PopupDropperAction *> actions = cac->customActions();

                menu->addSeparator();
                foreach( PopupDropperAction *action, actions )
                    menu->addAction( action );
            }
        }
    }

    menu->exec( pos  );
}

