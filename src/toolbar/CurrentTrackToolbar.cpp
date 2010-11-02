/****************************************************************************************
 * Copyright (c) 2009  Nikolaj Hald Nielsen <nhn@kde.org>                               *
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
 
#include "CurrentTrackToolbar.h"

#include "EngineController.h"
#include "GlobalCurrentTrackActions.h"
#include "core/capabilities/CurrentTrackActionsCapability.h"


CurrentTrackToolbar::CurrentTrackToolbar( QWidget * parent )
    : QToolBar( parent )
{
    setToolButtonStyle( Qt::ToolButtonIconOnly );
    setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred );
    //setIconDimensions( 16 );
    setContentsMargins( 0, 0, 0, 0 );

    EngineController *engine = The::engineController();

    connect( engine, SIGNAL( trackChanged( Meta::TrackPtr ) ),
             this, SLOT( handleAddActions() ) );
}

CurrentTrackToolbar::~CurrentTrackToolbar()
{}

void CurrentTrackToolbar::handleAddActions()
{
    clear();

    Meta::TrackPtr track = The::engineController()->currentTrack();

    foreach( QAction* action, The::globalCurrentTrackActions()->actions() )
        addAction( action );
    
    if( track && track->hasCapabilityInterface( Capabilities::Capability::CurrentTrackActions ) )
    {
        Capabilities::CurrentTrackActionsCapability *cac = track->create<Capabilities::CurrentTrackActionsCapability>();
        if( cac )
        {
            QList<QAction *> currentTrackActions = cac->customActions();
            foreach( QAction *action, currentTrackActions )
                addAction( action );

        }
        delete cac;
    }
}


