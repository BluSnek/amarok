/****************************************************************************************
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "PhotosEngine.h"

// Amarok
#include "Amarok.h"
#include "ContextObserver.h"
#include "ContextView.h"
#include "Debug.h"
#include "EngineController.h"

// KDE
#include <KIO/Job>


// Qt
#include <QDomDocument>
#include <QPixmap>


#define DEBUG_PREFIX "PhotosEngine"

using namespace Context;

PhotosEngine::PhotosEngine( QObject* parent, const QList<QVariant>& /*args*/ )
        : DataEngine( parent )
        , ContextObserver( ContextView::self() )
        , m_jobFlickr( 0 )
        , m_nbFlickr( -1 )
        , m_nbPhotos( 10 )
        , m_requested( true )
{
    m_sources << "flickr" ;
    update();
}

PhotosEngine::~PhotosEngine()
{
    DEBUG_BLOCK
    m_photos.clear();
}

QStringList 
PhotosEngine::sources() const
{
    return m_sources;
}

bool 
PhotosEngine::sourceRequestEvent( const QString& name )
{
    m_requested = true; // someone is asking for data, so we turn ourselves on :)
    QStringList tokens = name.split( ':' );
    
    // user has change the number of photos to download
    if ( tokens.contains( "nbphotos" ) && tokens.size() > 1 )
    {
        if ( ( tokens.at( 1 ) == QString( "nbphotos" ) ) && ( tokens.size() > 2 ) )
        {
            m_nbPhotos = tokens.at( 2 ).toInt();
            return false;
        }
    }
        
    removeAllData( name );
    setData( name, QVariant() );
    update();
    return true;
}

void 
PhotosEngine::message( const ContextState& state )
{
    if ( state == Current && m_requested )
        update();
}

void 
PhotosEngine::metadataChanged( Meta::TrackPtr track )
{
    const bool hasChanged = track->artist()->name() != m_artist;
    if ( hasChanged )
        update();
}

void PhotosEngine::update()
{
    DEBUG_BLOCK
    QString tmpYoutStr;
    // prevent
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    if ( !currentTrack || !currentTrack->artist() )
        return;
    else if ( currentTrack->artist()->name() == m_artist )
        return;
    else
    {
        unsubscribeFrom( m_currentTrack );
        m_currentTrack = currentTrack;
        subscribeTo( currentTrack );

        if ( !currentTrack )
            return;

        // Save artist
        m_artist = currentTrack->artist()->name();

        m_nbFlickr=-1;
            
        removeAllData( "photos" );
        m_photos.clear();
        m_listJob.clear();
        
        // Show the information
        setData( "photos", "message", "Fetching" );
        setData( "photos", "artist", m_artist );

        // Query flickr, order by relevance, 10 max
        // Flickr : http://api.flickr.com/services/rest/?method=flickr.photos.search&api_key=9c5a288116c34c17ecee37877397fe31&text=ARTIST&per_page=20
        KUrl flickrUrl(
            QString( "http://api.flickr.com/services/rest/?method=flickr.photos.search&api_key=9c5a288116c34c17ecee37877397fe31&text=" )
            + m_artist + QString( "&per_page=" ) + QString().setNum( m_nbPhotos ) + QString( "&sort=relevance&media=photos" ) );
        debug()<< "Flickr : " << flickrUrl.toMimeDataString() ;
        m_jobFlickr = KIO::storedGet( flickrUrl, KIO::Reload, KIO::HideProgressInfo );
        connect( m_jobFlickr, SIGNAL( result( KJob* ) ), SLOT( resultFlickr( KJob* ) ) );

    }
}

void PhotosEngine::resultFlickr( KJob* job )
{

    if ( !m_jobFlickr ) //track changed while we were fetching
        return;

    DEBUG_BLOCK
    if ( job->error() != KJob::NoError && job == m_jobFlickr ) // It's the correct job but it errored out
    {
        setData( "photos", "message", i18n( "Unable to retrieve Flickr : %1", job->errorString() ) );
        debug() << "Unable to retrieve Flickr information: " << job->errorString();
        m_jobFlickr = 0; // clear job
        m_nbFlickr = 0; //say that we didn't fetch any youtube songs (which is true !)
        resultFinalize();
    }
    // Get the result
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QDomDocument xmlDoc;
    xmlDoc.setContent( storedJob->data() );
    QDomNodeList xmlNodeList = xmlDoc.elementsByTagName( "photo" );

    QTime tim, time( 0, 0 );
    m_nbFlickr = 0;
    for ( uint i = 0; i < xmlNodeList.length() ; i++ )
    {
        QDomElement nd = xmlNodeList.at( i ).toElement();
        QString url = "http://farm" + nd.attribute( "farm" ) + ".static.flickr.com/" + nd.attribute( "server" ) + "/" + nd.attribute( "id" ) +"_"+ nd.attribute( "secret" ) +".jpg";
        debug() << url;
        // Insert the item in the list
        m_listJob << url;
            
        // Send a job to get the downloadable link
        KJob *jobu = KIO::storedGet( KUrl( url ), KIO::Reload, KIO::HideProgressInfo );
        connect( jobu, SIGNAL( result( KJob* ) ), SLOT( resultImageFetcher( KJob* ) ) );
    }
    m_nbFlickr += xmlNodeList.length();
    // Check how many clip we've find and send message if all the job are finished but no clip were find
    debug() << "Flickr fetch : " << m_nbFlickr << " photos ";
    
    m_jobFlickr = 0;
    resultFinalize();
}

void PhotosEngine::resultImageFetcher( KJob *job )
{
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QString jobUrl( storedJob->url().toMimeDataString() );    
    if ( m_listJob.contains( jobUrl ) )
    {
        if ( job->error() != KJob::NoError )
        {
            DEBUG_BLOCK
            debug() << "PhotosEngine | Unable to retrieve an image: " << job->errorString();
            m_listJob.removeOne( jobUrl );
            resultFinalize();
            return;
        }
        
        QPixmap *pix = new QPixmap;
        if ( pix->loadFromData( storedJob->data() ) ) {;}
        m_photos << pix;

        m_listJob.removeOne( jobUrl );
        resultFinalize();
    }
}


void PhotosEngine::resultFinalize()
{
    if ( m_nbFlickr==0 )
    {
        DEBUG_BLOCK
        debug() << "No Photos found";
        setData( "photos", "message", i18n( "No photos found for this artist ..." ) );
        return;
    }
    // else if nb job finished and they have been called 
    else if ( m_listJob.empty() && m_nbFlickr!=-1 )
    {      
        DEBUG_BLOCK
        debug() << "PhotosEngine total Fetched : " << m_photos.size() << " entries";

        // remove previous message
        removeData( "photos", "message" );

        // if the song hasn't change while fetchin, we sen the info
        if ( m_currentTrack != The::engineController()->currentTrack() )
            return;

        QVariant var;
        var.setValue< QList< QPixmap *> > ( m_photos );
        setData( "photos", "data", var );
    }
}

#include "PhotosEngine.moc"

