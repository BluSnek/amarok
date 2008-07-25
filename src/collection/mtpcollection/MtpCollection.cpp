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

#define DEBUG_PREFIX "MtpCollection"

#include "MtpCollection.h"
#include "MtpCollectionLocation.h"
#include "MtpMeta.h"

#include "amarokconfig.h"
#include "Debug.h"

//#include "MediaDeviceCache.h"
#include "MediaDeviceMonitor.h"
#include "MemoryQueryMaker.h"

//solid specific includes
//#include <solid/devicenotifier.h>
//#include <solid/device.h>
//#include <solid/storageaccess.h>
//#include <solid/storagedrive.h>

#include <KUrl>


#include <QStringList>


AMAROK_EXPORT_PLUGIN( MtpCollectionFactory )

MtpCollectionFactory::MtpCollectionFactory()
    : CollectionFactory()
{
    //nothing to do
}

MtpCollectionFactory::~MtpCollectionFactory()
{

}

void
MtpCollectionFactory::init()
{
    DEBUG_BLOCK

            // connect to the monitor

        connect( MediaDeviceMonitor::instance(), SIGNAL( mtpDetected( const QString &, const QString & ) ),
                 SLOT( mtpDetected( const QString &, const QString & ) ) );
        connect( MediaDeviceMonitor::instance(), SIGNAL( deviceRemoved( const QString & ) ), SLOT( deviceRemoved( const QString & ) ) );

    // force refresh to scan for mtp
    // NOTE: perhaps a signal/slot mechanism would make more sense
    MediaDeviceMonitor::instance()->refreshDevices();


    return;
}

void
MtpCollectionFactory::mtpDetected( const QString & udi, const QString &serial )
{
    MtpCollection* coll = 0;

     if( !m_collectionMap.contains( udi ) )
        {
               coll = new MtpCollection( udi, serial );
            if ( coll )
            {
                if( !coll->handler()->succeeded() ) // if couldn't connect
                    return;
            
            // TODO: connect to MediaDeviceMonitor signals
            connect( coll, SIGNAL( collectionDisconnected( const QString &) ),
                     SLOT( slotCollectionDisconnected( const QString & ) ) );
           m_collectionMap.insert( udi, coll );
            emit newCollection( coll );
            debug() << "emitting new mtp collection";
            }
        }

}

void
MtpCollectionFactory::deviceRemoved( const QString &udi )
{
    DEBUG_BLOCK
    if (  m_collectionMap.contains( udi ) )
    {
        MtpCollection* coll = m_collectionMap[ udi ];
                if (  coll )
                {
                    m_collectionMap.remove( udi ); // remove from map
                    coll->deviceRemoved();  //collection will be deleted by collectionmanager
                }
                else
                    warning() << "collection already null";
    }
    else
        warning() << "removing non-existent device";

    return;
}

void
MtpCollectionFactory::slotCollectionDisconnected( const QString & udi)
{
    deviceRemoved( udi );
}

void
MtpCollectionFactory::slotCollectionReady()
{
    DEBUG_BLOCK
        MtpCollection *collection = dynamic_cast<MtpCollection*>(  sender() );
    if (  collection )
    {
        debug() << "emitting mtp collection newcollection";
        emit newCollection(  collection );
    }
}



//MtpCollection

MtpCollection::MtpCollection( const QString &udi, const QString &serial )
    : Collection()
    , MemoryCollection()
    , m_udi( udi )
    , m_serial( serial )
    , m_handler( 0 )
{
    DEBUG_BLOCK

    m_handler = new Mtp::MtpHandler( this, this, serial );
    if( m_handler->succeeded() )
        m_handler->parseTracks();
    
        
    
}

void
MtpCollection::copyTrackToDevice( const Meta::TrackPtr &track )
{
    Q_UNUSED( track );
    // TODO: NYI
    //m_handler->copyTrackToDevice( track );
    return;
}

bool
MtpCollection::deleteTrackFromDevice( const Meta::MtpTrackPtr &track )
{
    DEBUG_BLOCK
    return false; // TODO: NYI
    

        // remove the track from the device
    if ( !m_handler->deleteTrackFromDevice( track ) )
        return false;

    // remove the track from the collection maps too
    removeTrack ( track );

    // inform treeview collection has updated
    emit updated();
    debug() << "deleteTrackFromDevice returning true";
    return true;
}

void
MtpCollection::removeTrack( const Meta::MtpTrackPtr &track )
{
    DEBUG_BLOCK

    // get pointers
    
    Meta::MtpArtistPtr artist = Meta::MtpArtistPtr::dynamicCast( track->artist() );
    Meta::MtpAlbumPtr album = Meta::MtpAlbumPtr::dynamicCast( track->album() );
    Meta::MtpGenrePtr genre = Meta::MtpGenrePtr::dynamicCast( track->genre() );
    Meta::MtpComposerPtr composer = Meta::MtpComposerPtr::dynamicCast( track->composer() );
    Meta::MtpYearPtr year = Meta::MtpYearPtr::dynamicCast( track->year() );

    // remove track from metadata's tracklists

    debug() << "Artist name: " << artist->name();
    
    artist->remTrack( track );
    album->remTrack( track );
    genre->remTrack( track );
    composer->remTrack( track );
    year->remTrack( track );

    // if empty, get rid of metadata in general

    if( artist->tracks().isEmpty() )
    {
        m_artistMap.remove( artist->name() );
        debug() << "Artist still in artist map: " << ( m_artistMap.contains( artist->name() ) ? "yes" : "no");
        acquireWriteLock();
        setArtistMap( m_artistMap );
        releaseLock();
    }
    if( album->tracks().isEmpty() )
    {
        m_albumMap.remove( album->name() );
        acquireWriteLock();
        setAlbumMap( m_albumMap );
        releaseLock();
    }
    if( genre->tracks().isEmpty() )
    {
        m_genreMap.remove( genre->name() );
        acquireWriteLock();
        setGenreMap( m_genreMap );
        releaseLock();
    }
    if( composer->tracks().isEmpty() )
    {
        m_composerMap.remove( composer->name() );
        acquireWriteLock();
        setComposerMap( m_composerMap );
        releaseLock();
    }
    if( year->tracks().isEmpty() )
    {
        m_yearMap.remove( year->name() );
        acquireWriteLock();
        setYearMap( m_yearMap );
        releaseLock();
    }

    // remove from trackmap

    m_trackMap.remove( track->name() );
}

void
MtpCollection::updateTags( Meta::MtpTrack *track)
{
    DEBUG_BLOCK
    Meta::MtpTrackPtr trackPtr( track );
    KUrl trackUrl = KUrl::fromPath( trackPtr->url() );

    debug() << "Running updateTrackInDB...";
// TODO: NYI
//    m_handler->updateTrackInDB( trackUrl, Meta::TrackPtr::staticCast( trackPtr ), track->getMtpTrack() );
    
}

void
MtpCollection::writeDatabase()
{
    // NOTE: NYI, possibly unnecessary or different implementation required
//    m_handler->writeITunesDB( false );
}

MtpCollection::~MtpCollection()
{

}

void
MtpCollection::deviceRemoved()
{
    emit remove();
}

void
MtpCollection::startFullScan()
{
    //ignore
}

QueryMaker*
MtpCollection::queryMaker()
{
    return new MemoryQueryMaker( this, collectionId() );
}

QString
MtpCollection::collectionId() const
{
     return m_udi;
}

CollectionLocation*
MtpCollection::location() const
{
    return new MtpCollectionLocation( this );
}

QString
MtpCollection::prettyName() const
{
    // TODO: there's nothing pretty about this name, get a prettier one
    return m_handler->prettyName();
}

QString
MtpCollection::udi() const
{
    return m_udi;
}

void
MtpCollection::setTrackToDelete( const Meta::MtpTrackPtr &track )
{
    m_trackToDelete = track;
}

void
MtpCollection::deleteTrackToDelete()
{
    deleteTrackFromDevice( m_trackToDelete );
}

void
MtpCollection::deleteTrackSlot( Meta::MtpTrackPtr track)
{
    deleteTrackFromDevice( track );
}

void
MtpCollection::slotDisconnect()
{
    emit collectionDisconnected( m_udi );
    //emit remove();
}

void
MtpCollection::copyTracksCompleted()
{
    DEBUG_BLOCK
//        debug() << "Trying to write iTunes database";
//    m_handler->writeITunesDB( false ); // false, since not threaded, implement later
    
}

#include "MtpCollection.moc"

