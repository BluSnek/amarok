/***************************************************************************
 *   Copyright (C) 2004 Frederik Holljen <fh@ez.no>                        *
 *             (C) 2004,5 Max Howell <max.howell@methylblue.com>           *
 *             (C) 2004-2008 Mark Kretschmann <kretschmann@kde.org>        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_ENGINECONTROLLER_H
#define AMAROK_ENGINECONTROLLER_H

#include "EngineObserver.h"
#include "meta/Meta.h"

#include <QMap>
#include <QObject>
#include <QPointer>

#include <Phonon/Global>
#include <Phonon/Path>
#include <Phonon/MediaSource> //Needed for the slot

class QTimer;

namespace KIO { class Job; }
namespace Meta { class MultiPlayableCapability; }
namespace Phonon { class AudioOutput; class MediaObject; class VolumeFaderEffect; }

/**
 * This class captures Amarok specific behaviour for some common features.
 * Accessing the engine directly is perfectly legal but on your own risk.
 */

class AMAROK_EXPORT EngineController : public QObject, public EngineSubject
{
    Q_OBJECT

    friend class EngineControllerSingleton;

public:
    typedef QMap<QString, bool>  ExtensionCache;

    static bool              canDecode( const KUrl& );
    static ExtensionCache&   extensionCache() { return s_extensionCache; }

    int trackPosition() const;

    Meta::TrackPtr currentTrack() const;
    int trackLength() const;

    /**
     * Used to enqueue a track before it starts to play, for gapless playback.
     */
    void setNextTrack( Meta::TrackPtr );

    void restoreSession();
    void endSession();

    Phonon::State state() const { return phononMediaObject()->state(); }

    //xx000, xx100, xx200, so at most will be 200ms delay before time displays are updated
    static const int MAIN_TIMER = 150;

    /*enum Filetype { MP3 };*/ //assuming MP3 for time being
    /*AMAROK_EXPORT*/ static bool installDistroCodec();

    const Phonon::MediaObject* phononMediaObject() const { return m_media; } //!const so that it's only used by DBus for info
    int volume() const;
    bool loaded() { return phononMediaObject()->state() != Phonon::StoppedState; }
    bool getAudioCDContents(const QString &device, KUrl::List &urls);
    bool isStream();
    enum PlayerStatus
    {
        Playing  = 0,
        Paused   = 1,
        Stopped  = 2,
        Error   = -1
    };

public slots:
    void play();
    void play( const Meta::TrackPtr&, uint offset = 0 );
    void pause();
    void stop( bool forceInstant = false );
    void playPause(); //pauses if playing, plays if paused or stopped

    void seek( int ms );
    void seekRelative( int ms );
    void seekForward( int ms = 10000 );
    void seekBackward( int ms = 10000 );

    int increaseVolume( int ticks = 100/25 );
    int decreaseVolume( int ticks = 100/25 );
    int setVolume( int percent );

    void mute();

signals:
    void trackPlayPause( int ); //Playing: 0, Paused: 1
    void trackFinished();
    void trackChanged( Meta::TrackPtr );
    void trackSeeked( int ); //return relative time in million second
    void volumeChanged( int );

protected:
    void playUrl( const KUrl &url, uint offset );
    void trackDone();

private slots:
    void slotAboutToFinish();
    void slotNewTrackPlaying( const Phonon::MediaSource &source);
    void slotStateChanged( Phonon::State newState, Phonon::State oldState);
    void slotPlayableUrlFetched(const KUrl&);
    void slotTick( qint64 );
    void slotTrackLengthChanged( qint64 );
    void slotMetaDataChanged();
    void slotStopFadeout(); //called after the fade-out has finished

private:
    EngineController();
    ~EngineController();

    static ExtensionCache s_extensionCache;

    Q_DISABLE_COPY( EngineController );

    Phonon::MediaObject *m_media;
    Phonon::AudioOutput *m_audio;
    Phonon::Path        m_path;
    QPointer<Phonon::VolumeFaderEffect> m_fader;

    Meta::TrackPtr  m_currentTrack;
    Meta::TrackPtr  m_lastTrack;
    Meta::TrackPtr  m_nextTrack;
    QPointer<Meta::MultiPlayableCapability> m_multi;
    QTimer* m_fadeoutTimer;
};

namespace The {
    AMAROK_EXPORT EngineController* engineController();
}


#endif
