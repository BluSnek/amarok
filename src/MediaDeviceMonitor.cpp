/* 
   Copyright (C) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#define DEBUG_PREFIX "MediaDeviceMonitor"

#include "MediaDeviceMonitor.h"

#include "Debug.h"

#include "MediaDeviceCache.h"

//solid specific includes
#include <solid/devicenotifier.h>
#include <solid/device.h>
#include <solid/storageaccess.h>
#include <solid/storagedrive.h>

MediaDeviceMonitor* MediaDeviceMonitor::s_instance = 0;

MediaDeviceMonitor::MediaDeviceMonitor() : QObject()
{
    DEBUG_BLOCK
    s_instance = this;
}

MediaDeviceMonitor::~MediaDeviceMonitor()
{
    s_instance = 0;
}

void
MediaDeviceMonitor::init()
{
    DEBUG_BLOCK


    /* Refresh cache */
//    MediaDeviceCache::instance()->refreshCache();
//    QStringList udiList = MediaDeviceCache::instance()->getAll();

    /* check cache for supported devices */

//    checkDevices( udiList );


    // connect to device cache so new devices are tested too
    connect(  MediaDeviceCache::instance(),  SIGNAL(  deviceAdded( const QString& ) ),
              SLOT(  deviceAdded( const QString& ) ) );
    connect(  MediaDeviceCache::instance(),  SIGNAL(  deviceRemoved( const QString& ) ),
              SLOT(  deviceRemoved( const QString& ) ) );
    connect(  MediaDeviceCache::instance(), SIGNAL( accessibilityChanged( bool, const QString & ) ),
              SLOT(  slotAccessibilityChanged( bool, const QString & ) ) );


    return;
}

void
MediaDeviceMonitor::refreshDevices()
{
    /* Refresh cache */
    MediaDeviceCache::instance()->refreshCache();
    QStringList udiList = MediaDeviceCache::instance()->getAll();

    /* check cache for supported devices */

    checkDevices( udiList );
}

void
MediaDeviceMonitor::checkDevices( const QStringList &udiList )
{
    /* poll udi list for supported devices */
    foreach(const QString &udi, udiList )
    {
        /* if iPod found, emit signal */
        if( isIpod( udi ) )
        {
            emit ipodDetected( MediaDeviceCache::instance()->volumeMountPoint(udi), udi );
        }

    }
}

void
MediaDeviceMonitor::deviceAdded(  const QString &udi )
{
    DEBUG_BLOCK

            debug() << "New device added, testing...";

    // TODO: write convenience method that takes in 1 udi

    QStringList udiList;

    udiList.append( udi );

    // send new udi for testing

    checkDevices( udiList );

    return;
}

void
MediaDeviceMonitor::deviceRemoved( const QString &udi )
{
    DEBUG_BLOCK

            // NOTE: perhaps a simple forwarding of signals would do
            // via a connect

            emit deviceRemoved( udi );

    return;
}

// TODO: this logic isn't entirely right, fix it

void
MediaDeviceMonitor::slotAccessibilityChanged( bool accessible, const QString & udi)
{
    DEBUG_BLOCK
            debug() << "Accessibility changed to: " << ( accessible ? "true":"false" );
    if ( !accessible )
        deviceRemoved( udi );
    else
        deviceAdded( udi );

}

bool
MediaDeviceMonitor::isIpod( const QString &udi )
{

    Solid::Device device;

    device = Solid::Device(udi);
    /* going until we reach a vendor, e.g. Apple */
    while ( device.isValid() && device.vendor().isEmpty() )
    {
        device = Solid::Device( device.parentUdi() );
    }

    debug() << "Device udi: " << udi;
    debug() << "Device name: " << MediaDeviceCache::instance()->deviceName(udi);
    debug() << "Mount point: " << MediaDeviceCache::instance()->volumeMountPoint(udi);
    if ( device.isValid() )
    {
        debug() << "vendor: " << device.vendor() << ", product: " << device.product();
    }

    /* if iPod found, return true */
    return (device.product() == "iPod");

}

