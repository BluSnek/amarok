/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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

#include "ScriptableServiceManager.h"


#include "collection/support/MemoryCollection.h"
#include "debug.h"
#include "ScriptableServiceCollection.h"
#include "ScriptableServiceCollection.h"
#include "ScriptableServiceMeta.h"
#include <scriptableservicemanageradaptor.h>
#include "servicemetabase.h"
#include "servicebrowser.h"

#include <kiconloader.h>

using namespace Meta;

ScriptableServiceManager * ScriptableServiceManager::s_instance = 0;

ScriptableServiceManager * ScriptableServiceManager::instance()
{
    if ( s_instance == 0 )
        s_instance = new ScriptableServiceManager();

    return s_instance;
}

ScriptableServiceManager::ScriptableServiceManager( )
{
    DEBUG_BLOCK

    new ScriptableServiceManagerAdaptor( this );
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject("/ScriptableServiceManager", this);
}


bool ScriptableServiceManager::initService( const QString &name, int levels, const QString &shortDescription,  const QString &rootHtml, bool showSearchBar ) {

    DEBUG_BLOCK

    debug() << "initializing scripted service: " << name;

    if ( !m_serviceMap.contains( name ) ) {
        debug() << "no such service script found";
        return false;
    }

    ScriptableService * service = m_serviceMap.value( name );
    service->setIcon( KIcon( "get-hot-new-stuff-amarok" ) );
    service->setShortDescription( shortDescription );
    service->init( levels, rootHtml, showSearchBar );
    m_rootHtml = rootHtml;

    emit addService( service );

    return true;
}


int ScriptableServiceManager::insertItem(const QString & serviceName, int level, int parentId, const QString & name, const QString & infoHtml, const QString & callbackData, const QString & playableUrl)
{
    DEBUG_BLOCK
    if ( !m_serviceMap.contains( serviceName ) ) {
        //invalid service name
        return -1;
    }

    m_serviceMap[serviceName]->insertItem( level, parentId, name, infoHtml, callbackData, playableUrl );
}


void ScriptableServiceManager::donePopulating(const QString & serviceName, int parentId)
{
    DEBUG_BLOCK
    debug() << "Service name: " << serviceName << ", parent id: " << parentId;
    if ( !m_serviceMap.contains( serviceName ) ) {
        //invalid service name
        return;
    }

    m_serviceMap[serviceName]->donePopulating( parentId );


}


void ScriptableServiceManager::addRunningScript( const QString & name, AmarokProcIO * script )
{
    DEBUG_BLOCK

    debug() << "adding service script named: " << name;
    
    if ( m_serviceMap.contains( name ) ) {
        debug() << "service already running";
        return;
    }
    
    ScriptableService * service = new ScriptableService ( name, script );
    m_serviceMap[name] = service;

    debug() << "sedning init message to script";
    script->writeStdin( "init" );
}

void ScriptableServiceManager::removeRunningScript(const QString & name)
{
    if ( !m_serviceMap.contains( name ) ) {
        debug() << "no such service to remove";
        return;
    }

    m_serviceMap.take( name );

    //service gets deleted by serviceBrowser
    ServiceBrowser::instance()->removeService( name );
}


namespace The {

    ScriptableServiceManager * scriptableServiceManager() {
        return ScriptableServiceManager::instance();
    }
}


#include "ScriptableServiceManager.moc"




