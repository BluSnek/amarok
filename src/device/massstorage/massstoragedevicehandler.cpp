/*
 *  Copyright (c) 2006-2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#define DEBUG_PREFIX "MassStorageDeviceHandler"

#include "massstoragedevicehandler.h"

AMAROK_EXPORT_PLUGIN( MassStorageDeviceHandlerFactory )

#include "debug.h"

#include <kconfig.h>
#include <kurl.h>

#include <q3valuelist.h>

MassStorageDeviceHandler::MassStorageDeviceHandler(): DeviceHandler()
{
}

MassStorageDeviceHandler::MassStorageDeviceHandler( int deviceId, const QString &mountPoint, const QString &uuid )
    : DeviceHandler()
    , m_deviceID( deviceId )
    , m_mountPoint( mountPoint )
    , m_uuid( uuid )
{
}

MassStorageDeviceHandler::~MassStorageDeviceHandler()
{
}

bool MassStorageDeviceHandler::isAvailable() const
{
    return true;
}


QString MassStorageDeviceHandler::type() const
{
    return "uuid";
}

int MassStorageDeviceHandler::getDeviceID()
{
    return m_deviceID;
}

const QString &MassStorageDeviceHandler::getDevicePath() const
{
    return m_mountPoint;
}

void MassStorageDeviceHandler::getURL( KUrl &absolutePath, const KUrl &relativePath )
{
    absolutePath.setPath( m_mountPoint );
    absolutePath.addPath( relativePath.path() );
    absolutePath.cleanPath();
}

void MassStorageDeviceHandler::getPlayableURL( KUrl &absolutePath, const KUrl &relativePath )
{
    getURL( absolutePath, relativePath );
}

bool MassStorageDeviceHandler::deviceIsMedium( const Medium * m ) const
{
    return m_uuid == m->id();
}

///////////////////////////////////////////////////////////////////////////////
// class MassStorageDeviceHandlerFactory
///////////////////////////////////////////////////////////////////////////////

QString MassStorageDeviceHandlerFactory::type( ) const
{
    return "uuid";
}

bool MassStorageDeviceHandlerFactory::canCreateFromMedium( ) const
{
    return true;
}

bool MassStorageDeviceHandlerFactory::canCreateFromConfig( ) const
{
    return false;
}

bool MassStorageDeviceHandlerFactory::canHandle( const Medium * m ) const
{
    return m && !m->id().isEmpty() && !excludedFilesystem( m->fsType() );
}

MassStorageDeviceHandlerFactory::MassStorageDeviceHandlerFactory( )
{
}

MassStorageDeviceHandlerFactory::~MassStorageDeviceHandlerFactory( )
{
}

DeviceHandler * MassStorageDeviceHandlerFactory::createHandler( KSharedConfigPtr ) const
{
    return 0;
}

DeviceHandler * MassStorageDeviceHandlerFactory::createHandler( const Medium * m ) const
{
    QStringList ids = CollectionManager::instance()->sqlStorage()->query( QString( "SELECT id, label, lastmountpoint "
                                                               "FROM devices WHERE type = 'uuid' "
                                                               "AND uuid = '%1';" ).arg( m->id() ) );
    if ( ids.size() == 3 )
    {
        debug() << "Found existing UUID config for ID " << ids[0] << " , uuid " << m->id();
        CollectionManager::instance()->sqlStorage()->query( QString( "UPDATE devices SET lastmountpoint = '%2' WHERE "
                                                  "id = %1;" ).arg( ids[0] ).arg( m->mountPoint() ) );
        return new MassStorageDeviceHandler( ids[0].toInt(), m->mountPoint(), m->id() );
    }
    else
    {
        int id = CollectionManager::instance()->sqlStorage()->insert( QString( "INSERT INTO devices( type, uuid, lastmountpoint ) "
                                                            "VALUES ( 'uuid', '%1', '%2' );" )
                                                            .arg( m->id() )
                                                            .arg( m->mountPoint() ), "devices" );
        if ( id == 0 )
        {
            warning() << "Inserting into devices failed for type=uuid, uuid=" << m->id();
            return 0;
        }
        debug() << "Created new UUID device with ID " << id << " , uuid " << m->id();
        return new MassStorageDeviceHandler( id, m->mountPoint(), m->id() );
    }
}

bool
MassStorageDeviceHandlerFactory::excludedFilesystem( const QString &fstype ) const
{
    return fstype.isEmpty() ||
           fstype.find( "smb" ) != -1 ||
           fstype.find( "cifs" ) != -1 ||
           fstype.find( "nfs" ) != -1 ||
           fstype == "udf"  ||
           fstype == "iso9660" ;
}
