/***************************************************************************
 *   Copyright (C) 2004 Frederik Holljen <fh@ez.no>                        *
 *             (C) 2004, 2005 Max Howell <max.howell@methylblue.com>       *
 *             (C) 2004-2008 Mark Kretschmann <kretschmann@kde.org>        *
 *             (C) 2006, 2008 Ian Monroe <ian@monroe.nu>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DEBUG_PREFIX "EngineController"

#include "EngineController.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "collection/CollectionManager.h"
#include "StatusBar.h"
#include "Debug.h"
#include "MainWindow.h"
#include "mediabrowser.h"
#include "meta/Meta.h"
#include "meta/MetaConstants.h"
#include "meta/MultiPlayableCapability.h"
#include "playlist/PlaylistModel.h"
#include "PluginManager.h"

#include <KApplication>
#include <KFileItem>
#include <KIO/Job>
#include <KMessageBox>
#include <KRun>

#include <Phonon/AudioOutput>
#include <Phonon/BackendCapabilities>
#include <Phonon/MediaObject>
#include <Phonon/VolumeFaderEffect>

#include <QByteArray>
#include <QFile>
#include <QTimer>


EngineController::ExtensionCache EngineController::s_extensionCache;

class EngineControllerSingleton
{
    public:
        EngineController self;
};
K_GLOBAL_STATIC( EngineControllerSingleton, privateSelf )

EngineController*
The::engineController()
{
    return &privateSelf->self;
}

EngineController::EngineController()
    : m_media( 0 )
    , m_audio( 0 )
    , m_fadeoutTimer( new QTimer( this ) )
{
    DEBUG_BLOCK

    qAddPostRoutine( privateSelf.destroy );  // Ensure that the dtor gets called when QCoreApplication destructs

    PERF_LOG( "EngineController: loading phonon objects" )
    m_media = new Phonon::MediaObject( this );
    m_audio = new Phonon::AudioOutput( Phonon::MusicCategory, this );

    m_path = Phonon::createPath(m_media, m_audio);

    m_media->setTickInterval( 100 );
    debug() << "Tick Interval (actual): " << m_media->tickInterval();
    PERF_LOG( "EngineController: loaded phonon objects" )

    m_fadeoutTimer->setSingleShot( true );

    //TODO: The xine engine does not support crossfading.
    // I cannot get the gstreamer engine to work, will test this once I do.

//     if( AmarokConfig::trackDelayLength() > -1 )
//         m_media->setTransitionTime( AmarokConfig::trackDelayLength() ); // Also Handles gapless.
//     else if( AmarokConfig::crossfadeLength() > 0 )  // TODO: Handle the possible options on when to crossfade.. the values are not documented anywhere however
//         m_media->setTransitionTime( -AmarokConfig::crossfadeLength() );

    connect( m_media, SIGNAL( finished() ), SLOT( slotTrackEnded() ) );
//    connect( m_media, SIGNAL( aboutToFinish() ), SLOT( slotAboutToFinish() ) );
    connect( m_media, SIGNAL( metaDataChanged() ), SLOT( slotMetaDataChanged() ) );
    connect( m_media, SIGNAL( stateChanged( Phonon::State, Phonon::State ) ),
                      SLOT( slotStateChanged( Phonon::State, Phonon::State ) ) );
    connect( m_media, SIGNAL( tick( qint64 ) ), SLOT( slotTick( qint64 ) ) );
    connect( m_media, SIGNAL( totalTimeChanged( qint64 ) ), SLOT( slotTrackLengthChanged( qint64 ) ) );
    connect( m_media, SIGNAL( currentSourceChanged( const Phonon::MediaSource & ) ),
                       SLOT( slotNewTrackPlaying( const Phonon::MediaSource & ) ) );

    connect( m_fadeoutTimer, SIGNAL( timeout() ), SLOT( slotStopFadeout() ) );
}

EngineController::~EngineController()
{
    DEBUG_BLOCK //we like to know when singletons are destroyed

    qRemovePostRoutine( privateSelf.destroy );

    m_media->stop();

    delete m_media;
    delete m_audio;
}


//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
//////////////////////////////////////////////////////////////////////////////////////////

bool
EngineController::canDecode( const KUrl &url ) //static
{
   //NOTE this function must be thread-safe

    const QString fileName = url.fileName();
    const QString ext = Amarok::extension( fileName );

    // We can't use playlists in the engine
    if ( PlaylistManager::isPlaylist( url ) )
        return false;

    // Playing images is pretty stupid
    if ( ext == "png" || ext == "jpg" )
        return false;

    // Ignore protocols "fetchcover" and "musicbrainz", they're not local but we don't really want them in the playlist :)
    if ( url.protocol() == "fetchcover" || url.protocol() == "musicbrainz" )
        return false;

    // Accept non-local files, since we can't test them for validity at this point
    if ( url.protocol() == "http" || url.protocol() == "https" )
        return true;

    // If extension is already in the cache, return cache result
    if ( extensionCache().contains( ext ) )
        return s_extensionCache[ext];

    // If file has 0 bytes, ignore it and return false, so that we don't infect the cache with corrupt files
    if ( !QFileInfo(url.path()).size() )
        return false;

    KFileItem item( KFileItem::Unknown, KFileItem::Unknown, url );
    const bool valid = Phonon::BackendCapabilities::isMimeTypeAvailable( item.mimetype() );

    //we special case this as otherwise users hate us
    if ( !valid && ext.toLower() == "mp3" && !installDistroCodec() )
        The::statusBar()->longMessageThreadSafe(
                i18n( "<p>Phonon claims it <b>cannot</b> play MP3 files.You may want to examine "
                    "the installation of the backend that phonon uses.</p>"
                    "<p>You may find useful information in the <i>FAQ</i> section of the <i>Amarok Handbook</i>.</p>" ), KDE::StatusBar::Error );

    // Cache this result for the next lookup
    if ( !ext.isEmpty() )
        extensionCache().insert( ext, valid );

    return valid;
}

bool
EngineController::installDistroCodec()
{
    KService::List services = KServiceTypeTrader::self()->query( "Amarok/CodecInstall"
        , QString("[X-KDE-Amarok-codec] == 'mp3' and [X-KDE-Amarok-engine] == 'phonon-%1'").arg("xine") );
    //todo - figure out how to query Phonon for the current backend loaded
    if( !services.isEmpty() )
    {
        KService::Ptr service = services.first(); //list is not empty
        QString installScript = service->exec();
        if( !installScript.isNull() ) //just a sanity check
        {
            KGuiItem installButton( i18n( "Install MP3 Support" ) );
            if(KMessageBox::questionYesNo( The::mainWindow()
            , i18n("Amarok currently cannot play MP3 files. Do you want to install support for MP3?")
            , i18n( "No MP3 Support" )
            , installButton
            , KStandardGuiItem::no()
            , "codecInstallWarning" ) == KMessageBox::Yes )
            {
                    KRun::runCommand(installScript, 0);
                    return true;
            }
        }
    }

    return false;
}

void
EngineController::restoreSession()
{
    //here we restore the session
    //however, do note, this is always done, KDE session management is not involved

    if( !AmarokConfig::resumeTrack().isEmpty() )
    {
        const KUrl url = AmarokConfig::resumeTrack();
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( url );
        if( track )
            play( track, AmarokConfig::resumeTime() );
    }
}

void
EngineController::endSession()
{
    //only update song stats, when we're not going to resume it
    if ( !AmarokConfig::resumePlayback() && m_currentTrack )
    {
        trackEnded( trackPosition(), m_currentTrack->length() * 1000, "quit" );
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void
EngineController::play() //SLOT
{
    DEBUG_BLOCK
    if( m_fader )
        m_fader->deleteLater();

    if ( m_media->state() == Phonon::PausedState )
    {
        m_media->play();
    }
    else
    {
        play( The::playlistModel()->activeTrack() );
    }
}

void
EngineController::play( const Meta::TrackPtr& track, uint offset )
{
    DEBUG_BLOCK

    if( !track ) // Guard
        return;

    delete m_multi;
    m_currentTrack = track;
    m_multi = m_currentTrack->as<Meta::MultiPlayableCapability>();

    if( m_multi )
    {
        connect( m_multi, SIGNAL( playableUrlFetched( const KUrl & ) ), this, SLOT( slotPlayableUrlFetched( const KUrl & ) ) );
        m_multi->fetchFirst();
    }
    else
    {
        playUrl( track->playableUrl(), offset );
        emit trackChanged( track );
    }
}

void
EngineController::playUrl( const KUrl &url, uint offset )
{
    DEBUG_BLOCK

    slotStopFadeout();

    debug() << "URL: " << url.url();
    m_media->setCurrentSource( url );

    if( offset )
    {
        m_media->pause();
        m_media->seek( offset );
    }
    m_media->play();
}

void
EngineController::pause() //SLOT
{
    m_media->pause();
}

void
EngineController::stop( bool forceInstant ) //SLOT
{
    DEBUG_BLOCK

    // need to get a new instance of multi if played again
    delete m_multi;

    //let Amarok know that the previous track is no longer playing
    if( m_currentTrack ) {
        debug() << "m_currentTrack != 0";
        m_currentTrack->finishedPlaying( 1.0 );
        trackEnded( trackPosition(), m_currentTrack->length() * 1000, "stop" );
    }

    if( m_fader )
        m_fader->deleteLater();

    if( AmarokConfig::fadeoutLength() && !forceInstant ) {
        stateChangedNotify( Phonon::StoppedState, Phonon::PlayingState ); //immediately disable Stop action

        m_fader = new Phonon::VolumeFaderEffect( this );
        m_path.insertEffect( m_fader );
        m_fader->setFadeCurve( Phonon::VolumeFaderEffect::Fade9Decibel );
        m_fader->fadeOut( AmarokConfig::fadeoutLength() );

        m_fadeoutTimer->start( AmarokConfig::fadeoutLength() + 1000 ); //add 1s for good measure, otherwise seems to cut off early (buffering..)
    }
    else
        m_media->stop();

    emit trackFinished();
}

void
EngineController::playPause() //SLOT
{
    //this is used by the TrayIcon, PlayPauseAction and DCOP

    DEBUG_BLOCK

    if( m_media->state() == Phonon::PlayingState )
    {
        pause();
        emit trackPlayPause( Paused );
    }
    else
    {
        play();
        emit trackPlayPause( Playing );
    }
}

void
EngineController::seek( int ms ) //SLOT
{
    DEBUG_BLOCK

    if( m_media->isSeekable() )
    {
        m_media->seek( static_cast<qint64>( ms ) );
        trackPositionChangedNotify( ms, true ); /* User seek */
        emit trackSeeked( ms );
    }
    else
        debug() << "Stream is not seekable.";
}


void
EngineController::seekRelative( int ms ) //SLOT
{
    qint64 newPos = m_media->currentTime() + ms;
    seek( newPos <= 0 ? 0 : newPos );
}

void
EngineController::seekForward( int ms )
{
    seekRelative( ms );
}

void
EngineController::seekBackward( int ms )
{
    seekRelative( -ms );
}

int
EngineController::increaseVolume( int ticks ) //SLOT
{
    return setVolume( volume() + ticks );
}

int
EngineController::decreaseVolume( int ticks ) //SLOT
{
    return setVolume( volume() - ticks );
}

int
EngineController::setVolume( int percent ) //SLOT
{
    if( percent < 0 ) percent = 0;
    if( percent > 100 ) percent = 100;

    qreal newVolume = percent / 100.0; //Phonon's volume is 0.0 - 1.0
    m_audio->setVolume( newVolume );
    AmarokConfig::setMasterVolume( percent );
    volumeChangedNotify( percent );

    emit volumeChanged( percent );
    return percent;
}

int
EngineController::volume() const
{
    return static_cast<int>( m_audio->volume() * 100.0 );
}

void
EngineController::mute() //SLOT
{
    m_audio->setMuted( !m_audio->isMuted() );
}

Meta::TrackPtr
EngineController::currentTrack() const
{
    Phonon::State state = m_media->state();
    return state == Phonon::ErrorState ? Meta::TrackPtr() : m_currentTrack;
}

int
EngineController::trackLength() const
{
    const qint64 phononLength = m_media->totalTime(); //may return -1
    if( phononLength <= 0 && m_currentTrack ) //this is useful for stuff like last.fm streams
        return m_currentTrack->length();      //where Meta::Track knows the length, but phonon doesn't
    else
        return static_cast<int>( phononLength / 1000 );
}

bool
EngineController::getAudioCDContents(const QString &device, KUrl::List &urls)
{
    Q_UNUSED( device ); Q_UNUSED( urls );
//since Phonon doesn't know anything about CD listings, there's actually no reason for this functionality to be here
//kept to keep things compiling, probably should be in its own class.
    return false;
}

bool
EngineController::isStream()
{
    DEBUG_BLOCK

    if( m_media ) {
        debug() << "stream = true";
        return m_media->currentSource().stream() != 0;
    }

    debug() << "stream = false";
    return false;
}

int
EngineController::trackPosition() const
{
//NOTE: there was a bunch of last.fm logic removed from here
//pretty sure it's irrelevant, if not, look back to mid-March 2008
    return static_cast<int>( m_media->currentTime() / 1000 );
}

//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void
EngineController::slotTick( qint64 position )
{
    trackPositionChangedNotify( static_cast<long>( position ), false ); //it expects milliseconds
}

void
EngineController::slotAboutToFinish()
{
#if 0
    DEBUG_BLOCK

    if( m_multi.isNull() )
    {
        trackEnded( m_media->currentTime(), m_media->totalTime(), i18n( "Previous track finished" ) );
        slotTrackEnded(); // Phonon does not alert us that a track has finished if there is another source in the queue.
        m_currentTrack = The::playlistModel()->nextTrack();
        if( m_currentTrack )
            m_media->enqueue( m_currentTrack->playableUrl() );
    }
    else
    {
        m_multi->fetchNext();
    }
#endif
}

void
EngineController::slotTrackEnded()
{
    DEBUG_BLOCK

    emit trackFinished();

    if( m_currentTrack )
        m_currentTrack->finishedPlaying( 1.0 );

    if( m_multi )
        m_multi->fetchNext();
    else
    {
        m_currentTrack = The::playlistModel()->nextTrack();

        if( m_currentTrack ) {
            if( m_currentTrack->playableUrl().isEmpty() )
            {
                m_media->stop();
                return;
            }
            m_media->setCurrentSource( m_currentTrack->playableUrl() );
            m_media->play();
            emit trackChanged( m_currentTrack );
        }
    }
}

void
EngineController::slotNewTrackPlaying( const Phonon::MediaSource &source )
{
    Q_UNUSED( source );

    newTrackPlaying();
}

void
EngineController::slotStateChanged( Phonon::State newState, Phonon::State oldState ) //SLOT
{
    // Sanity checks
    if( newState == oldState || newState == Phonon::BufferingState )
        return;

    if( newState == Phonon::ErrorState ) {  // If media is borked, skip to next track
        warning() << "Phonon failed to play this URL. Error: " << m_media->errorString();
        QTimer::singleShot( 0, this, SLOT( slotTrackEnded() ) );  // QTimer because we don't want to cause recursion
    }

    stateChangedNotify( newState, oldState );
}

void
EngineController::slotPlayableUrlFetched( const KUrl &url )
{
    if( url.isEmpty() )
    {
        play( The::playlistModel()->nextTrack() );
    }
    else
    {
        playUrl( url, 0 );
    }
}

void
EngineController::slotTrackLengthChanged( qint64 milliseconds )
{
    if( milliseconds != 0 ) //don't notify for 0 seconds, it's probably just a stream
        trackLengthChangedNotify( static_cast<long>( milliseconds ) / 1000 );
}

void
EngineController::slotMetaDataChanged()
{
    DEBUG_BLOCK

    QHash<qint64, QString> meta;

    meta.insert( Meta::valUrl, m_media->currentSource().url().toString() );

    QStringList artist = m_media->metaData( "ARTIST" );
    debug() << "Artist     : " << artist;
    if( !artist.isEmpty() )
        meta.insert( Meta::valArtist, artist.first() );

    QStringList album = m_media->metaData( "ALBUM" );
    debug() << "Album      : " << album;
    if( !album.isEmpty() )
        meta.insert( Meta::valAlbum, album.first() );

    QStringList title = m_media->metaData( "TITLE" );
    debug() << "Title      : " << title;
    if( !title.isEmpty() )
        meta.insert( Meta::valTitle, title.first() );

    QStringList genre = m_media->metaData( "GENRE" );
    debug() << "Genre      : " << genre;
    if( !genre.isEmpty() )
        meta.insert( Meta::valGenre, genre.first() );

    QStringList tracknum = m_media->metaData( "TRACKNUMBER" );
    debug() << "Tracknumber: " << tracknum;
    if( !tracknum.isEmpty() )
        meta.insert( Meta::valTrackNr, tracknum.first() );

    QStringList length = m_media->metaData( "LENGTH" );
    debug() << "Length     : " << length;
    if( !length.isEmpty() )
        meta.insert( Meta::valLength, length.first() );

    bool trackChanged = false;
    if( m_lastTrack != m_currentTrack )
    {
        trackChanged = true;
        m_lastTrack = m_currentTrack;
    }
    debug() << "Track changed: " << trackChanged;
    newMetaDataNotify( meta, trackChanged );
}

void
EngineController::slotStopFadeout() //SLOT
{
    DEBUG_BLOCK

    // Make sure the timer won't call this method again 
    m_fadeoutTimer->stop();

    if ( m_fader ) {
        m_fader->deleteLater();
        m_media->stop();
    }
}

#include "EngineController.moc"

