/* 
   Copyright (c) 2004 Christian Muehlhaeuser <chris@chris.de>
   Copyright (c) 2004 Sami Nieminen <sami.nieminen@iki.fi>
   Copyright (c) 2006 Shane King <kde@dontletsstart.com>
   Copyright (c) 2006 Iain Benson <iain@arctos.me.uk>
   Copyright (c) 2006 Alexandre Oliveira <aleprj@gmail.com>
   Copyright (c) 2006 Andy Kelk <andy@mopoke.co.uk>

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

#define DEBUG_PREFIX "Scrobbler"

#include "scrobbler.h"

#include "amarok.h"
#include "amarokconfig.h"
#include "config-amarok.h"
#include "debug.h"r
#include "enginecontroller.h"
#include "metabundle.h"
#include "statusbar.h"

#include <KApplication>
#include <KCodecs>
#include <KIO/Job>
#include <kio/jobclasses.h>
#include <KLocale>
#include <KStandardDirs>
#include <KUrl>

#include <QtAlgorithms>
#include <QDateTime>
#include <QHashIterator>
#include <QTextStream>

#include <unistd.h>

//some setups require this
#undef PROTOCOL_VERSION


////////////////////////////////////////////////////////////////////////////////
// CLASS Scrobbler
////////////////////////////////////////////////////////////////////////////////

Scrobbler* Scrobbler::instance()
{
    static Scrobbler scrobbler;
    return &scrobbler;
}


Scrobbler::Scrobbler()
    : EngineObserver( EngineController::instance() )
    , m_similarArtistsJob( 0 )
    , m_validForSending( false )
    , m_startPos( 0 )
    , m_submitter( new ScrobblerSubmitter() )
    , m_item( new SubmitItem() )
{}


Scrobbler::~Scrobbler()
{
    delete m_item;
    delete m_submitter;
}


/**
 * Queries similar artists from Audioscrobbler.
 */
void Scrobbler::similarArtists( const QString & artist )
{
    QString safeArtist = artist;
    if ( AmarokConfig::retrieveSimilarArtists() )
    {
//         Request looks like this:
//         http://ws.audioscrobbler.com/1.0/artist/Metallica/similar.xml

        m_similarArtistsBuffer = QByteArray();
        m_artist = artist;

        m_similarArtistsJob = KIO::get( "http://ws.audioscrobbler.com/1.0/artist/" + safeArtist + "/similar.xml", false, false );

        connect( m_similarArtistsJob, SIGNAL( result( KIO::Job* ) ),
                 this,                  SLOT( audioScrobblerSimilarArtistsResult( KIO::Job* ) ) );
        connect( m_similarArtistsJob, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
                 this,                  SLOT( audioScrobblerSimilarArtistsData( KIO::Job*, const QByteArray& ) ) );
    }
}


/**
 * Called when the similar artists TransferJob finishes.
 */
void Scrobbler::audioScrobblerSimilarArtistsResult( KIO::Job* job ) //SLOT
{
    if ( m_similarArtistsJob != job )
        return; //not the right job, so let's ignore it

    if ( job->error() )
    {
        warning() << "KIO error! errno: " << job->error();
        return;
    }

//     Result looks like this:
//        <?xml version="1.0" encoding="UTF-8"?>
//        <similarartists artist="Metallica" streamable="1" picture="http://static.last.fm/proposedimages/sidebar/6/1000024/288059.jpg" mbid="">
//          <artist>
//            <name>Iron Maiden</name>
//            <mbid></mbid>
//            <match>100</match>
//            <url>http://www.last.fm/music/Iron+Maiden</url>
//            <image_small>http://static.last.fm/proposedimages/thumbnail/6/1000107/264195.jpg</image_small>
//            <image>http://static.last.fm/proposedimages/sidebar/6/1000107/264195.jpg</image>
//            <streamable>1</streamable>
//          </artist>
//        </similarartists>

    QDomDocument document;
    if ( !document.setContent( m_similarArtistsBuffer ) )
    {
        debug() << "Couldn't read similar artists response";
        return;
    }

    QDomNodeList values = document.elementsByTagName( "similarartists" )
        .item( 0 ).childNodes();

    QStringList suggestions;
    for ( int i = 0; i < values.count() && i < 30; i++ ) // limit to top 30 artists
        suggestions << values.item( i ).namedItem( "name" ).toElement().text();

    debug() << "Suggestions retrieved (" << suggestions.count() << ")";
    if ( !suggestions.isEmpty() )
        emit similarArtistsFetched( m_artist, suggestions );

    m_similarArtistsJob = 0;
}


/**
 * Called when similar artists data is received for the TransferJob.
 */
void Scrobbler::audioScrobblerSimilarArtistsData( KIO::Job* job, const QByteArray& data ) //SLOT
{
    if ( m_similarArtistsJob != job )
        return; //not the right job, so let's ignore it

    uint oldSize = m_similarArtistsBuffer.size();
    m_similarArtistsBuffer.resize( oldSize + data.size() );
    memcpy( m_similarArtistsBuffer.data() + oldSize, data.data(), data.size() );
}


/**
 * Called when the signal is received.
 */
void Scrobbler::engineNewMetaData( const MetaBundle& bundle, bool trackChanged )
{
    //debug() << "engineNewMetaData: " << bundle.artist() << ":" << bundle.album() << ":" << bundle.title() << ":" << trackChanged;
    if ( !trackChanged )
    {
        debug() << "It's still the same track.";
        m_item->setArtist( bundle.artist() );
        m_item->setAlbum( bundle.album() );
        m_item->setTitle( bundle.title() );
        return;
    }

    //to work around xine bug, we have to explictly prevent submission the first few seconds of a track
    //http://sourceforge.net/tracker/index.php?func=detail&aid=1401026&group_id=9655&atid=109655
    m_timer.stop();
    m_timer.setSingleShot( true );
    m_timer.start( 10000 );

    m_startPos = 0;

    // Plugins must not submit tracks played from online radio stations, even
    // if they appear to be providing correct metadata.
    if ( !bundle.streamUrl().isEmpty() )
    {
        debug() << "Won't submit: It's a stream.";
        m_validForSending = false;
    }
    else if( bundle.podcastBundle() != NULL )
    {
        debug() << "Won't submit: It's a podcast.";
        m_validForSending = false;
    }
    else
    {
        *m_item = SubmitItem( bundle.artist(), bundle.album(), bundle.title(), bundle.length() );
        m_validForSending = true; // check length etc later
    }
}


/**
 * Called when cue file detects track change
 */
void Scrobbler::subTrack( long currentPos, long startPos, long endPos )
{
    //debug() << "subTrack: " << currentPos << ":" << startPos << ":" << endPos;
    *m_item = SubmitItem( m_item->artist(), m_item->album(), m_item->title(), endPos - startPos );
    if ( currentPos <= startPos + 2 ) // only submit if starting from the start of the track (need to allow 2 second difference for rounding/delay)
    {
        m_startPos = startPos * 1000;
        m_validForSending = true;
    }
    else
    {
        debug() << "Won't submit: Detected cuefile jump to " << currentPos - startPos << " seconds into track.";
        m_validForSending = false;
    }
}


/**
 * Called when the signal is received.
 */
void Scrobbler::engineTrackPositionChanged( long position, bool userSeek )
{
    //debug() << "engineTrackPositionChanged: " << position << ":" << userSeek;
    if ( !m_validForSending )
        return;

    if ( userSeek )
    {
        m_validForSending = false;
        debug() << "Won't submit: Seek detected.";
        return;
    }

    if ( m_timer.isActive() )
        return;

    // Each track must be submitted to the server when it is 50% or 240
    // seconds complete, whichever comes first.
    if ( position - m_startPos > 240 * 1000 || position - m_startPos > 0.5 * m_item->length() * 1000 )
    {
        if ( m_item->valid() )
            m_submitter->submitItem( new SubmitItem( *m_item ) );
        else
            debug() << "Won't submit: No artist, no title, or less than 30 seconds.";
        m_validForSending = false;
    }
}


/**
 * Applies settings from the config dialog.
 */
void Scrobbler::applySettings()
{
    m_submitter->configure( AmarokConfig::scrobblerUsername(), AmarokConfig::scrobblerPassword(), AmarokConfig::submitPlayedSongs() );
}


////////////////////////////////////////////////////////////////////////////////
// CLASS SubmitItem
////////////////////////////////////////////////////////////////////////////////


SubmitItem::SubmitItem(
    const QString& artist,
    const QString& album,
    const QString& title,
    int length,
    bool now)
{
    m_artist = artist;
    m_album = album;
    m_title = title;
    m_length = length;
    m_playStartTime = now ? QDateTime::currentDateTime().toUTC().toTime_t() : 0;
}


SubmitItem::SubmitItem( const QDomElement& element )
{
    m_artist = element.namedItem( "artist" ).toElement().text();
    m_album = element.namedItem( "album" ).toElement().text();
    m_title = element.namedItem( "title" ).toElement().text();
    m_length = element.namedItem( "length" ).toElement().text().toInt();
    m_playStartTime = element.namedItem( "playtime" ).toElement().text().toUInt();
}


SubmitItem::SubmitItem()
    : m_length( 0 )
    , m_playStartTime( 0 )
{
}


bool SubmitItem::operator==( const SubmitItem& item )
{
    bool result = true;

    if ( m_artist != item.artist() || m_album != item.album() || m_title != item.title() ||
         m_length != item.length() || m_playStartTime != item.playStartTime() )
    {
        result = false;
    }

    return result;
}


QDomElement SubmitItem::toDomElement( QDomDocument& document ) const
{
    QDomElement item = document.createElement( "item" );
    // TODO: In the future, it might be good to store url too
    //item.setAttribute("url", item->url().url());

    QDomElement artist = document.createElement( "artist" );
    QDomText artistText = document.createTextNode( m_artist );
    artist.appendChild( artistText );
    item.appendChild( artist );

    QDomElement album = document.createElement( "album" );
    QDomText albumText = document.createTextNode( m_album );
    album.appendChild( albumText );
    item.appendChild( album );

    QDomElement title = document.createElement( "title" );
    QDomText titleText = document.createTextNode( m_title );
    title.appendChild( titleText );
    item.appendChild( title );

    QDomElement length = document.createElement( "length" );
    QDomText lengthText = document.createTextNode( QString::number( m_length ) );
    length.appendChild( lengthText );
    item.appendChild( length );

    QDomElement playtime = document.createElement( "playtime" );
    QDomText playtimeText = document.createTextNode( QString::number( m_playStartTime ) );
    playtime.appendChild( playtimeText );
    item.appendChild( playtime );

    return item;
}


////////////////////////////////////////////////////////////////////////////////
// CLASS SubmitQueue
////////////////////////////////////////////////////////////////////////////////


bool
SubmitQueue::compareItems( SubmitItem *sItem1, SubmitItem *sItem2 )
{
    return !( sItem1 == sItem2 ) && sItem1->playStartTime() < sItem2->playStartTime();
}


////////////////////////////////////////////////////////////////////////////////
// CLASS ScrobblerSubmitter
////////////////////////////////////////////////////////////////////////////////

QString ScrobblerSubmitter::PROTOCOL_VERSION = "1.1";
QString ScrobblerSubmitter::CLIENT_ID = "ark";
QString ScrobblerSubmitter::CLIENT_VERSION = "1.4";
QString ScrobblerSubmitter::HANDSHAKE_URL = "http://post.audioscrobbler.com/?hs=true";


ScrobblerSubmitter::ScrobblerSubmitter()
    : m_username( 0 )
    , m_password( 0 )
    , m_submitUrl( 0 )
    , m_challenge( 0 )
    , m_scrobblerEnabled( false )
    , m_holdFakeQueue( false )
    , m_inProgress( false )
    , m_needHandshake( true )
    , m_prevSubmitTime( 0 )
    , m_interval( 0 )
    , m_backoff( 0 )
    , m_lastSubmissionFinishTime( 0 )
    , m_fakeQueueLength( 0 )
{
    connect( &m_timer, SIGNAL(timeout()), this, SLOT(scheduledTimeReached()) );
    readSubmitQueue();
}


ScrobblerSubmitter::~ScrobblerSubmitter()
{
    // need to rescue current submit. This may meant it gets submitted twice,
    // but last.fm handles that, and it's better than losing it when you quit
    // while a submit is happening
    //for ( Q3PtrDictIterator<SubmitItem> it( m_ongoingSubmits ); it.current(); ++it )
    for( QHashIterator<KIO::Job*, SubmitItem*> iter( m_ongoingSubmits ); iter.hasNext(); )
        m_submitQueue.append( iter.next().value() );
    m_ongoingSubmits.clear();
    qStableSort( m_submitQueue.begin(), m_submitQueue.end(), SubmitQueue::compareItems );

    saveSubmitQueue();

    qDeleteAll( m_submitQueue );
    m_submitQueue.clear();
    qDeleteAll( m_fakeQueue );
    m_fakeQueue.clear();
}


/**
 * Performs handshake with Audioscrobbler.
 */
void ScrobblerSubmitter::performHandshake()
{
    QString handshakeUrl;
    uint currentTime = QDateTime::currentDateTime().toUTC().toTime_t();

    if ( PROTOCOL_VERSION == "1.1" )
    {
        // Audioscrobbler protocol 1.1 (current)
        // http://post.audioscrobbler.com/?hs=true
        // &p=1.1
        // &c=<clientid>
        // &v=<clientver>
        // &u=<user>
        handshakeUrl =
                HANDSHAKE_URL +
                QString(
                "&p=%1"
                "&c=%2"
                "&v=%3"
                "&u=%4" )
                .arg( PROTOCOL_VERSION )
                .arg( CLIENT_ID )
                .arg( CLIENT_VERSION )
                .arg( m_username );
    }

    else if ( PROTOCOL_VERSION == "1.2" )
    {
        // Audioscrobbler protocol 1.2 (RFC)
        // http://post.audioscrobbler.com/?hs=true
        // &p=1.2
        // &c=<clientid>
        // &v=<clientversion>
        // &u=<username>
        // &t=<unix_timestamp>
        // &a=<passcode>
        handshakeUrl =
                HANDSHAKE_URL +
                QString(
                "&p=%1"
                "&c=%2"
                "&v=%3"
                "&u=%4"
                "&t=%5"
                "&a=%6" )
                .arg( PROTOCOL_VERSION )
                .arg( CLIENT_ID )
                .arg( CLIENT_VERSION )
                .arg( m_username )
                .arg( currentTime )
                .arg( QString::fromAscii( KMD5( KMD5( m_password.toUtf8() ).hexDigest() + QString::number( currentTime ).toAscii() ).hexDigest() ) );
    }

    else
    {
        debug() << "Handshake not implemented for protocol version: " << PROTOCOL_VERSION;
        return;
    }

    debug() << "Handshake url: " << handshakeUrl;

    m_submitResultBuffer = "";

    m_inProgress = true;
    KIO::TransferJob* job = KIO::storedGet( handshakeUrl, false, false );
    connect( job, SIGNAL( result( KIO::Job* ) ), SLOT( audioScrobblerHandshakeResult( KIO::Job* ) ) );
}


/**
 * Sets item for submission to Audioscrobbler. Actual submission
 * depends on things like (is scrobbling enabled, are Audioscrobbler
 * profile details filled in etc).
 */
void ScrobblerSubmitter::submitItem( SubmitItem* item )
{
    if ( m_scrobblerEnabled ) {
        enqueueItem( item );

        if ( item->playStartTime() == 0 )
            m_holdFakeQueue = true; // hold on to fake queue until we get it all and can compute when to submit
        else if ( !schedule( false ) )
            announceSubmit( item, 1, false ); // couldn't perform submit immediately, let user know
    }
}


/**
 * Flushes the submit queues
 */
void ScrobblerSubmitter::performSubmit()
{
    QString data;

    // Audioscrobbler accepts max 10 tracks on one submit.
    SubmitItem* items[10];
    for ( int submitCounter = 0; submitCounter < 10; submitCounter++ )
        items[submitCounter] = 0;

    if ( PROTOCOL_VERSION == "1.1" )
    {
        // Audioscrobbler protocol 1.1 (current)
        // http://post.audioscrobbler.com/v1.1-lite.php
        // u=<user>
        // &s=<MD5 response>&
        // a[0]=<artist 0>&t[0]=<track 0>&b[0]=<album 0>&
        // m[0]=<mbid 0>&l[0]=<length 0>&i[0]=<time 0>&
        // a[1]=<artist 1>&t[1]=<track 1>&b[1]=<album 1>&
        // m[1]=<mbid 1>&l[1]=<length 1>&i[1]=<time 1>&
        // ...
        // a[n]=<artist n>&t[n]=<track n>&b[n]=<album n>&
        // m[n]=<mbid n>&l[n]=<length n>&i[n]=<time n>&


        data =
                "u=" + KUrl::toPercentEncoding( m_username, "/" ) +
                "&s=" +
                KUrl::toPercentEncoding( KMD5( KMD5( m_password.toUtf8() ).hexDigest() +
                m_challenge.toUtf8() ).hexDigest(), "/" );

        for ( int submitCounter = 0; submitCounter < 10; submitCounter++ )
        {
            SubmitItem* itemFromQueue = dequeueItem();
            if ( itemFromQueue == 0 )
            {
                if( submitCounter == 0 )
                {
                    // this shouldn't happen, since we shouldn't be scheduled until we have something to do!
                    debug() << "Nothing to submit!";
                    return;
                }
                else
                {
                    break;
                }
            }
            else
                data += '&';

            items[submitCounter] = itemFromQueue;
            QDateTime playStartTime = QDateTime();
            playStartTime.setTime_t( itemFromQueue->playStartTime() );

            const QString count = QString::number( submitCounter );

            // FIXME: we have to find something different for doing the encode_string_no_slash to utf-8
            data +=
                    "a["  + count + "]=" + KUrl::toPercentEncoding( itemFromQueue->artist(), "/" ) +
                    "&t[" + count + "]=" + KUrl::toPercentEncoding( itemFromQueue->title(), "/" ) +
                    "&b[" + count + "]=" + KUrl::toPercentEncoding( itemFromQueue->album(), "/" ) +
                    "&m[" + count + "]=" +
                    "&l[" + count + "]=" + QString::number( itemFromQueue->length() ) +
                    "&i[" + count + "]=" + KUrl::toPercentEncoding( playStartTime.toString( "yyyy-MM-dd hh:mm:ss" ), "/" );
        }
    }

    else
    {
        debug() << "Submit not implemented for protocol version: " << PROTOCOL_VERSION;
        return;
    }

    debug() << "Submit data: " << data;

    m_submitResultBuffer = "";

    m_inProgress = true;
    KIO::TransferJob* job = KIO::http_post( m_submitUrl, data.toUtf8(), false );
    job->addMetaData( "content-type", "Content-Type: application/x-www-form-urlencoded" );

    // Loop in reverse order, which helps when items are later fetched from
    // m_ongoingSubmits and possibly put back to queue, in correct order
    // (i.e. oldest first).
    for ( int submitCounter = 9; submitCounter >= 0; submitCounter-- )
        if ( items[submitCounter] != 0 )
            m_ongoingSubmits.insert( job, items[submitCounter] );

    Amarok::StatusBar::instance()->newProgressOperation( job )
            .setDescription( i18n( "Submitting to last.fm" ) );

    connect( job, SIGNAL( result( KIO::Job* ) ),
             this,  SLOT( audioScrobblerSubmitResult( KIO::Job* ) ) );
    connect( job, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
             this,  SLOT( audioScrobblerSubmitData( KIO::Job*, const QByteArray& ) ) );
}


/**
 * Configures the username/password and whether to scrobble
 */
void ScrobblerSubmitter::configure( const QString& username, const QString& password, bool enabled )
{
    if ( username != m_username || password != m_password )
        m_needHandshake = true;

    m_username = username;
    m_password = password;
    m_scrobblerEnabled = enabled;
    if ( enabled )
        schedule( false );
    else
    {
        // If submit is disabled, clear submitqueue.
        qDeleteAll( m_ongoingSubmits );
        m_ongoingSubmits.clear();
        qDeleteAll( m_submitQueue );
        m_submitQueue.clear();
        qDeleteAll( m_fakeQueue );
        m_fakeQueue.clear();
        m_fakeQueueLength = 0;
        m_timer.stop();
    }
}


/**
 * Sync from external device complete, can send them off
 */
void ScrobblerSubmitter::syncComplete()
{
    m_holdFakeQueue = false;
    saveSubmitQueue();
    schedule( false );
}


/**
 * Called when timer set up in the schedule function goes off.
 */
void ScrobblerSubmitter::scheduledTimeReached()
{
    if ( m_needHandshake || m_challenge.isEmpty() )
        performHandshake();
    else
        performSubmit();
}

/**
 * Called when handshake TransferJob has finished and data is received.
 */
void ScrobblerSubmitter::audioScrobblerHandshakeResult( KIO::Job* job ) //SLOT
{
    m_prevSubmitTime = QDateTime::currentDateTime().toUTC().toTime_t();
    m_inProgress = false;

    if ( job->error() ) {
        warning() << "KIO error! errno: " << job->error();
        schedule( true );
        return;
    }

    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    m_submitResultBuffer = QString::fromUtf8( storedJob->data().data(), storedJob->data().size() );

//     debug()
//         << "Handshake result received: "
//         << endl << m_submitResultBuffer;

    // UPTODATE
    // <md5 challenge>
    // <url to submit script>
    // INTERVAL n (protocol 1.1)
    if (m_submitResultBuffer.startsWith( "UPTODATE" ) )
    {
        m_challenge = m_submitResultBuffer.section( "\n", 1, 1 );
        m_submitUrl = m_submitResultBuffer.section( "\n", 2, 2 );
        QString interval = m_submitResultBuffer.section( "\n", 3, 3 );

        if ( interval.startsWith( "INTERVAL" ) )
            m_interval = interval.mid( 9 ).toUInt();
    }
    // UPDATE <updateurl (optional)>
    // <md5 challenge>
    // <url to submit script>
    // INTERVAL n (protocol 1.1)
    else if ( m_submitResultBuffer.startsWith( "UPDATE" ) )
    {
        warning() << "A new version of Amarok is available";

        m_challenge = m_submitResultBuffer.section( "\n", 1, 1 );
        m_submitUrl = m_submitResultBuffer.section( "\n", 2, 2 );
        QString interval = m_submitResultBuffer.section( "\n", 3, 3 );
        if ( interval.startsWith( "INTERVAL" ) )
            m_interval = interval.mid( 9 ).toUInt();
    }
    // FAILED <reason (optional)>
    // INTERVAL n (protocol 1.1)
    else if ( m_submitResultBuffer.startsWith( "FAILED" ) )
    {
        QString reason = m_submitResultBuffer.mid( 0, m_submitResultBuffer.indexOf( "\n" ) );
        if ( reason.length() > 6 )
            reason = reason.mid( 7 ).trimmed();

        warning() << "Handshake failed (" << reason << ")";
        QString interval = m_submitResultBuffer.section( "\n", 1, 1 );
        if ( interval.startsWith( "INTERVAL" ) )
            m_interval = interval.mid( 9 ).toUInt();
    }
    // BADUSER (protocol 1.1) or BADAUTH (protocol 1.2)
    // INTERVAL n (protocol 1.1)
    else if ( m_submitResultBuffer.startsWith( "BADUSER" ) ||
              m_submitResultBuffer.startsWith( "BADAUTH" ) )
    {
        warning() << "Handshake failed (Authentication failed)";
        QString interval = m_submitResultBuffer.section( "\n", 1, 1 );
        if ( interval.startsWith( "INTERVAL" ) )
            m_interval = interval.mid( 9 ).toUInt();
    }
    else
        warning() << "Unknown handshake response: " << m_submitResultBuffer;

    debug() << "Handshake result parsed: challenge=" << m_challenge << ", submitUrl=" << m_submitUrl;

    schedule( m_challenge.isEmpty() ); // schedule to submit or re-attempt handshake
}


/**
 * Called when submit TransferJob has finished and data is received.
 */
void ScrobblerSubmitter::audioScrobblerSubmitResult( KIO::Job* job ) //SLOT
{
    m_prevSubmitTime = QDateTime::currentDateTime().toUTC().toTime_t();
    m_inProgress = false;

    if ( job->error() ) {
        warning() << "KIO error! errno: " << job->error();
        enqueueJob( job );
        return;
    }

//     debug()
//         << "Submit result received: "
//         << endl << m_submitResultBuffer;

    // OK
    // INTERVAL n (protocol 1.1)
    if (m_submitResultBuffer.startsWith( "OK" ) )
    {
        debug() << "Submit successful";
        QString interval = m_submitResultBuffer.section( "\n", 1, 1 );
        if ( interval.startsWith( "INTERVAL" ) )
            m_interval = interval.mid( 9 ).toUInt();

        finishJob( job );
    }
    // FAILED <reason (optional)>
    // INTERVAL n (protocol 1.1)
    else if ( m_submitResultBuffer.startsWith( "FAILED" ) )
    {
        QString reason = m_submitResultBuffer.mid( 0, m_submitResultBuffer.indexOf( "\n" ) );
        if ( reason.length() > 6 )
            reason = reason.mid( 7 ).trimmed();

        warning() << "Submit failed (" << reason << ")";

        QString interval = m_submitResultBuffer.section( "\n", 1, 1 );
        if ( interval.startsWith( "INTERVAL" ) )
            m_interval = interval.mid( 9 ).toUInt();

        enqueueJob( job );
    }
    // BADAUTH
    // INTERVAL n (protocol 1.1)
    else if ( m_submitResultBuffer.startsWith( "BADAUTH" ) )
    {
        warning() << "Submit failed (Authentication failed)";

        QString interval = m_submitResultBuffer.section( "\n", 1, 1 );
        if ( interval.startsWith( "INTERVAL" ) )
            m_interval = interval.mid( 9 ).toUInt();

        m_challenge = QString();
        enqueueJob( job );
    }
    else
    {
        warning() << "Unknown submit response";
        enqueueJob( job );
    }
}


/**
 * Receives the data from the TransferJob.
 */
void ScrobblerSubmitter::audioScrobblerSubmitData(
    KIO::Job*, const QByteArray& data ) //SLOT
{
    // Append new chunk of string
    m_submitResultBuffer += QString::fromUtf8( data, data.size() );
}


/**
 * Checks if it is possible to try to submit the data to Audioscrobbler.
 */
bool ScrobblerSubmitter::canSubmit() const
{
    if ( !m_scrobblerEnabled || m_username.isEmpty() || m_password.isEmpty() )
    {
        debug() << "Unable to submit - no uname/pass or disabled";
        return false;
    }

    return true;
}


/**
 * Enqueues the given item for later submission.
 */
void ScrobblerSubmitter::enqueueItem( SubmitItem* item )
{
    // Maintain max size of the queue, Audioscrobbler won't accept too old
    // submissions anyway.
    for ( uint size = m_fakeQueue.count() + m_submitQueue.count(); size >= 500; size-- )
    {
        SubmitItem* itemFromQueue =  0;
        if( !m_fakeQueue.isEmpty() )
        {
            itemFromQueue = m_fakeQueue.first();
            m_fakeQueue.removeFirst();
        }

        if ( itemFromQueue )
        {
            debug() << "Dropping " << itemFromQueue->artist()
                    << " - " << itemFromQueue->title() << " from fake queue";
            m_fakeQueueLength -= itemFromQueue->length();
        }

        delete itemFromQueue;
    }

    for ( uint size = m_submitQueue.count(); size >= 500; size-- )
    {
        SubmitItem* itemFromQueue = 0;
        if( !m_submitQueue.isEmpty() )
        {
            itemFromQueue = m_submitQueue.first();
            m_submitQueue.removeFirst();
            debug() << "Dropping " << itemFromQueue->artist()
                     << " - " << itemFromQueue->title() << " from submit queue";
        }

        delete itemFromQueue;
    }

    if( item->playStartTime() == 0 )
    {
        m_fakeQueue.append( item );
        qStableSort( m_fakeQueue.begin(), m_fakeQueue.end(), SubmitQueue::compareItems );
        m_fakeQueueLength += item->length();
    }
    else
    {
        m_submitQueue.append( item );
        qStableSort( m_submitQueue.begin(), m_submitQueue.end(), SubmitQueue::compareItems );
    }

    if( !m_holdFakeQueue )
    {
       // Save submit queue to disk so it is more uptodate in case of crash.
       saveSubmitQueue();
    }
}


/**
 * Dequeues one item from the queue.
 */
SubmitItem* ScrobblerSubmitter::dequeueItem()
{
    SubmitItem* item = 0;
    if( m_lastSubmissionFinishTime > 0 && !m_holdFakeQueue && !m_fakeQueue.isEmpty() )
    {
        uint limit = QDateTime::currentDateTime().toUTC().toTime_t();

        if ( !m_submitQueue.isEmpty() )
            if ( m_submitQueue.first()->playStartTime() <= limit )
                limit = m_submitQueue.first()->playStartTime();

        if( m_lastSubmissionFinishTime + m_fakeQueue.first()->length() <= limit )
        {
            item = m_fakeQueue.first();
            m_fakeQueue.removeFirst();
            // don't backdate earlier than we have to
            if( m_lastSubmissionFinishTime + m_fakeQueueLength < limit )
                item->m_playStartTime = limit - m_fakeQueueLength;
            else
                item->m_playStartTime = m_lastSubmissionFinishTime;
            m_fakeQueueLength -= item->length();
        }
    }

    if( !item && !m_submitQueue.isEmpty() )
    {
        item = m_submitQueue.first();
        m_submitQueue.removeFirst();
    }

    if( item )
    {
        if( item->playStartTime() < m_lastSubmissionFinishTime )
        {
//            debug() << "play times screwed up? - " << item->artist() << " - " << item->title() << ": " << item->playStartTime() << " < " << m_lastSubmissionFinishTime;
        }
        int add = 30;
        if( item->length() / 2 + 1 > add )
            add = item->length() / 2 + 1;
        if( item->playStartTime() + add > m_lastSubmissionFinishTime )
            m_lastSubmissionFinishTime = item->playStartTime() + add;

        // Save submit queue to disk so it is more uptodate in case of crash.
        saveSubmitQueue();
    }

    return item;
}


/**
 * Enqueues items associated with the job. This is used when the job
 * has failed (e.g. network problems).
 */
void ScrobblerSubmitter::enqueueJob( KIO::Job* job )
{
    SubmitItem *lastItem = 0;
    SubmitItem *item = 0;
    int counter = 0;
    while ( ( item = m_ongoingSubmits.take( job ) ) != 0 )
    {
        counter++;
        lastItem = item;
        enqueueItem( item );
    }

    if( lastItem )
        announceSubmit( lastItem, counter, false );

    schedule( true ); // arrange to flush queue after failure
}


/**
 * Deletes items associated with the job. This is used when the job
 * has succeeded.
 */
void ScrobblerSubmitter::finishJob( KIO::Job* job )
{
    SubmitItem *firstItem = 0;
    SubmitItem *item = 0;
    int counter = 0;
    while ( ( item = m_ongoingSubmits.take( job ) ) != 0 )
    {
        counter++;
        if ( firstItem == 0 )
            firstItem = item;
        else
            delete item;
    }

    if( firstItem )
        announceSubmit( firstItem, counter, true );
    delete firstItem;

    schedule( false ); // arrange to flush rest of queue
}


/**
 * Announces on StatusBar if the submit was successful or not.
 *
 * @param item One of the items
 * @param tracks Amount of tracks that were submitted
 * @param success Indicates if the submission was successful or not
 */
void ScrobblerSubmitter::announceSubmit( SubmitItem *item, int tracks, bool success ) const
{
    QString _long, _short;

    if ( success )
    {
        if ( tracks == 1 )
            _short = i18n( "'%1' submitted to last.fm" ).arg( item->title() );
        else
        {
            _short = i18n( "Several tracks submitted to last.fm" );

            _long = "<p>";
            _long  = i18np( "'%1' and one other track submitted",
                           "'%1' and %1 other tracks submitted", tracks-1 )
                            .arg( item->title() );
        }
    }
    else
    {
        if ( tracks == 1 )
            _short = i18n( "Failed to submit '%1' to last.fm" ).arg( item->title() );
        else
        {
            _short = i18n( "Failed to submit several tracks to last.fm" );
            _long  = "<p>";
            _long  = i18np( "Failed to submit '%1' and one other track",
                           "Failed to submit '%1' and %1 other tracks", tracks-1 )
                      .arg( item->title() );
        }
    }

    if ( m_submitQueue.count() + m_fakeQueue.count() > 0 )
    {
        _long += "<p>";
        _long += i18np( "One track still in queue", "%1 tracks still in queue",
                m_submitQueue.count() + m_fakeQueue.count() );
    }

    Amarok::StatusBar::instance()->shortLongMessage( _short, _long );
}


void ScrobblerSubmitter::saveSubmitQueue()
{
    QFile file( m_savePath );

    if( !file.open( QIODevice::WriteOnly ) )
    {
        debug() << "[SCROBBLER] Couldn't write submit queue to file: " << m_savePath;
        return;
    }

    if ( m_lastSubmissionFinishTime == 0 )
        m_lastSubmissionFinishTime = QDateTime::currentDateTime().toUTC().toTime_t();

    QDomDocument newdoc;
    QDomElement submitQueue = newdoc.createElement( "submit" );
    submitQueue.setAttribute( "product", "Amarok" );
    submitQueue.setAttribute( "version", APP_VERSION );
    submitQueue.setAttribute( "lastSubmissionFinishTime", m_lastSubmissionFinishTime );

    for ( int idx = 0; idx < m_submitQueue.count(); idx++ )
    {
        SubmitItem *item = m_submitQueue.at( idx );
        QDomElement i = item->toDomElement( newdoc );
        submitQueue.appendChild( i );
    }

    for ( int idx = 0; idx < m_fakeQueue.count(); idx++ )
    {
        SubmitItem *item = m_fakeQueue.at( idx );
        QDomElement i = item->toDomElement( newdoc );
        submitQueue.appendChild( i );
    }

    QDomNode submitNode = newdoc.importNode( submitQueue, true );
    newdoc.appendChild( submitNode );

    QTextStream stream( &file );
    stream.setCodec( "UTF8" );
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    stream << newdoc.toString();
    file.close();
}


void ScrobblerSubmitter::readSubmitQueue()
{
    m_savePath = Amarok::saveLocation() + "submit.xml";
    QFile file( m_savePath );

    if ( !file.open( QIODevice::ReadOnly ) )
    {
        debug() << "Couldn't open file: " << m_savePath;
        return;
    }

    QTextStream stream( &file );
    stream.setCodec( "UTF8" );

    QDomDocument d;
    if( !d.setContent( stream.readAll() ) )
    {
        debug() << "Couldn't read file: " << m_savePath;
        return;
    }

    uint last = 0;
    if( d.namedItem( "submit" ).isElement() )
        last = d.namedItem( "submit" ).toElement().attribute( "lastSubmissionFinishTime" ).toUInt();
    if(last && last > m_lastSubmissionFinishTime)
        m_lastSubmissionFinishTime = last;

    const QString ITEM( "item" ); //so we don't construct these QStrings all the time

    for( QDomNode n = d.namedItem( "submit" ).firstChild(); !n.isNull() && n.nodeName() == ITEM; n = n.nextSibling() )
        enqueueItem( new SubmitItem( n.toElement() ) );
}


/**
 * Schedules an Audioscrobbler handshake or submit as required.
 * Returns true if an immediate submit was possible
 */
bool ScrobblerSubmitter::schedule( bool failure )
{
    m_timer.stop();
    if ( m_inProgress || !canSubmit() )
        return false;

    uint when, currentTime = QDateTime::currentDateTime().toUTC().toTime_t();
    if ( currentTime - m_prevSubmitTime > m_interval )
        when = 0;
    else
        when = m_interval - ( currentTime - m_prevSubmitTime );

    if ( failure )
    {
        m_backoff = qMin( qMax( m_backoff * 2, unsigned( MIN_BACKOFF ) ), unsigned( MAX_BACKOFF ) );
        when = qMax( m_backoff, m_interval );
    }
    else
        m_backoff = 0;

    if ( m_needHandshake || m_challenge.isEmpty() )
    {
        m_challenge = QString();
        m_needHandshake = false;

        if ( when == 0 )
        {
            debug() << "Performing immediate handshake";
            performHandshake();
        }
        else
        {
            debug() << "Performing handshake in " << when << " seconds";
            m_timer.setSingleShot( true );
            m_timer.start( when * 1000 );
        }
    }
    else if ( !m_submitQueue.isEmpty() || !m_holdFakeQueue && !m_fakeQueue.isEmpty() )
    {
        // if we only have stuff in the fake queue, we need to only schedule for when we can actually do something with it
        if ( m_submitQueue.isEmpty() && m_lastSubmissionFinishTime + m_fakeQueue.first()->length() > currentTime + when )
            when = m_lastSubmissionFinishTime + m_fakeQueue.first()->length() - currentTime;

        if ( when == 0 )
        {
            debug() << "Performing immediate submit";
            performSubmit();
            return true;
        }
        else
        {
            debug() << "Performing submit in " << when << " seconds";
            m_timer.setSingleShot( true );
            m_timer.start( when * 1000 );
        }
    } else {
        debug() << "Nothing to schedule";
    }

    return false;
}


#include "scrobbler.moc"
