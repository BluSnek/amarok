/****************************************************************************************
* Copyright (c) 2009 Nathan Sala <sala.nathan@gmail.com>                               *
* Copyright (c) 2009 Oleksandr Khayrullin <saniokh@gmail.com>                          *
* Copyright (c) 2009 Joffrey Clavel <jclavel@clabert.info>                             *
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

#include "SimilarArtistsEngine.h"
#include <lastfm/Artist>

#include "Amarok.h"
#include "Debug.h"
#include "ContextObserver.h"
#include "ContextView.h"
#include "EngineController.h"
#include <QDomElement>

using namespace Context;




SimilarArtistsEngine::SimilarArtistsEngine( QObject* parent, const QList<QVariant>& /*args*/ )
    : DataEngine( parent )
    , ContextObserver( ContextView::self() )
    , m_similarArtistsJob( 0 )
    , m_currentSelection( "artist" )
    , m_requested( true )
    , m_sources( "current" )
    , m_triedRefinedSearch( 0 )
{
    update();
}

SimilarArtistsEngine::~SimilarArtistsEngine()
{
    DEBUG_BLOCK
    delete m_similarArtistsJob;
    
}

QMap<int, QString> SimilarArtistsEngine::similarArtists(const QString &artist_name)
{
    lastfm::Artist artist(artist_name);
    return artist.getSimilar(artist.getSimilar());
}


QStringList SimilarArtistsEngine::sources() const
{
    return m_sources;
}

bool SimilarArtistsEngine::sourceRequestEvent( const QString& name )
{
    DEBUG_BLOCK

    m_requested = true; // someone is asking for data, so we turn ourselves on :)
    QStringList tokens = name.split( ':' );

    // user has changed the maximum artists returned.
    if ( tokens.contains( "maxArtists" ) && tokens.size() > 1 )
        if ( ( tokens.at( 1 ) == QString( "maxArtists" ) )  && ( tokens.size() > 2 ) )
            m_maxArtists = tokens.at( 2 ).toInt();
    
    // otherwise, it comes from the engine, a new track is playing.
    removeAllData( name );
    setData( name, QVariant());
    update();

    return true;
}

void SimilarArtistsEngine::message( const ContextState& state )
{
    if( state == Current && m_requested )
        update();
}

void SimilarArtistsEngine::metadataChanged( Meta::TrackPtr track )
{
    Q_UNUSED( track )
    DEBUG_BLOCK
    
    update();
}

void SimilarArtistsEngine::update()
{
    DEBUG_BLOCK

    // We've got a new track, great, let's fetch some info from SimilarArtists !
    m_triedRefinedSearch = 0;
    QString artistName;
    

    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();

    unsubscribeFrom( m_currentTrack );
    m_currentTrack = currentTrack;
    subscribeTo( currentTrack );

    if ( !currentTrack )
        return;
    
    DataEngine::Data data;
    // default, or applet told us to fetch artist
    if( selection() == "artist" ) 
    {
        if( currentTrack->artist() )
        {
            if ( ( currentTrack->playableUrl().protocol() == "lastfm" ) ||
                ( currentTrack->playableUrl().protocol() == "daap" ) ||
                !The::engineController()->isStream() )
                artistName = currentTrack->artist()->name();
            else
                artistName = currentTrack->artist()->prettyName();
        }
        
        if (artistName.compare( "") == 0) {
            setData( "similarArtists", "artist", "Unknown artist" );
        } else {
            setData( "similarArtists", "artist", artistName );
        }
    }

     QPixmap cover = m_currentTrack->album()->image( 156 );
     setData( "similarArtists", "cover",  QVariant( cover ) );

    similarArtistsRequest( artistName );
    QVariant variant ( QMetaType::type( "SimilarArtist::SimilarArtistsList" ), &m_similarArtists );

    setData ( "similarArtists", "SimilarArtists", variant );
}

void
SimilarArtistsEngine::similarArtistsRequest(const QString& artist_name)
{
    DEBUG_BLOCK
    
    QUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/2.0/" );
    url.addQueryItem( "method", "artist.getSimilar" );
    url.addQueryItem( "api_key", "402d3ca8e9bc9d3cf9b85e1202944ca5" );
    url.addQueryItem( "artist", artist_name.toLocal8Bit() );

    m_artist=artist_name;

    // for test when i haven't network
    //m_similarArtists.append( SimilarArtist(m_artist,1,url,url, m_artist ));

    m_similarArtistsJob = KIO::storedGet( url, KIO::NoReload, KIO::HideProgressInfo );
    connect( m_similarArtistsJob, SIGNAL(result( KJob* )), SLOT(similarArtistsParse( KJob* )) );
}


void
SimilarArtistsEngine::similarArtistsParse( KJob* job ) // SLOT
{
    DEBUG_BLOCK
    m_similarArtists.clear();

    if( !m_similarArtistsJob ) return; //track changed while we were fetching

    // It's the correct job but it errored out
    if( job->error() != KJob::NoError && job == m_similarArtistsJob )
    {
        //TODO display a error message
        //setData( "similarArtists", "message", i18n( "Unable to retrieve SimilarArtists information: %1", job->errorString() ) );
        m_similarArtistsJob = 0; // clear job
        return;
    }    
        
    // not the right job, so let's ignore it
    if( job != m_similarArtistsJob)
        return;
        
    if( job )
    {
        KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
        m_xml = QString::fromUtf8( storedJob->data().data(), storedJob->data().size() );
    }
    else
    {
        return;
    }

    QDomDocument doc;
    doc.setContent( m_xml );

    const QDomNode events = doc.documentElement().namedItem( "similarartists" );

    QDomNode n = events.firstChild();
    while( !n.isNull() )
    {
        // Similar artist name
        QDomNode nameNode = n.namedItem( "name" );
        QDomElement eName = nameNode.toElement();
        QString name;
        if( !eName.isNull() )
        {
            name = eName.text();
        }

        // Match of the similar artist
        QDomNode matchNode = n.namedItem( "match" );
        QDomElement eMatch = matchNode.toElement();
        int match;
        if( !eMatch.isNull() )
        {
            match = eMatch.text().toFloat(); //implicite cast
        }

        // Url of the similar artist on last.fm
        QDomNode urlNode = n.namedItem( "url" );
        QDomElement eUrl = urlNode.toElement();
        KUrl url;
        if( !eUrl.isNull() )
        {
            url = KUrl(eUrl.text());
        }
  

        // Url of the large similar artist image
        QDomNode imageUrlNode = n.namedItem( "image" );
        QDomElement imageUrlElement = imageUrlNode.toElement();
        KUrl imageUrl;
        if( !imageUrlElement.isNull() )
        {
            while ( imageUrlElement.attributeNode( "size" ).value() != "large" )
            {
                imageUrlElement = imageUrlElement.nextSiblingElement();
            }
            imageUrl = KUrl( imageUrlElement.text() );
        }

        
        m_similarArtists.append( SimilarArtist(name,match,url,imageUrl, m_artist ));

        n = n.nextSibling();
    }

    m_similarArtistsJob=0;
    update();
}


void
SimilarArtistsEngine::reloadSimilarArtists()
{

}

#include "SimilarArtistsEngine.moc"


