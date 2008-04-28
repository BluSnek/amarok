/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
#include "ShowInServiceAction.h"

#include "MainWindow.h"
#include "ServiceBrowser.h"

ShowInServiceAction::ShowInServiceAction( ServiceBase * service, Meta::ServiceTrack *track )
    : PopupDropperAction( service )
    , m_track( track )
    , m_service( service )
{
    setIcon ( KIcon( "system-search" ) );
    setText( i18n( "Go to artist in %1 service", service->getName() ) );

    connect( this, SIGNAL( triggered( bool ) ), SLOT( slotTriggered() ) );
}

ShowInServiceAction::~ShowInServiceAction()
{
}

void ShowInServiceAction::slotTriggered()
{
    DEBUG_BLOCK
    
    //artist or album?

    if ( m_service == 0 )
        return;
            
    MainWindow::self()->showBrowser( "Internet" );
    m_service->setFilter( QString( "artist:\"%1\"" ).arg( m_track->artist()->prettyName() ) );
    m_service->sortByArtistAlbum();
    ServiceBrowser::instance()->showService( m_service->getName() );

    //TODO: make sure the service browser is shown
}

#include "ShowInServiceAction.moc"



