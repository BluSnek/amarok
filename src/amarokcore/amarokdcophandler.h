/***************************************************************************
                          amarokdcophandler.h  -  DCOP Implementation
                             -------------------
    begin                : Sat Oct 11 2003
    copyright            : (C) 2003 by Stanislav Karchebny
    email                : berkus@users.sf.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_DCOP_HANDLER_H
#define AMAROK_DCOP_HANDLER_H

#include <qobject.h>
#include "amarokdcopiface.h"

namespace amaroK
{

class DcopPlayerHandler : public QObject, virtual public AmarokPlayerInterface
{
      Q_OBJECT

   public:
      DcopPlayerHandler();

   public /* DCOP */ slots:
      virtual void play();
      virtual void playPause();
      virtual void stop();
      virtual void next();
      virtual void prev();
      virtual void pause();
      virtual void seek( int s );
      virtual void seekRelative( int s );
      virtual void enableRandomMode( bool enable );
      virtual void enableRepeatPlaylist( bool enable );
      virtual int  trackTotalTime();
      virtual int  trackCurrentTime();
      virtual int  trackPlayCounter();
      virtual void scanCollection();
      virtual QString nowPlaying();
      virtual QString artist();
      virtual QString title();
      virtual QString track();
      virtual QString album();
      virtual QString totalTime();
      virtual QString currentTime();
      virtual QString genre();
      virtual QString year();
      virtual QString comment();
      virtual QString bitrate();
      virtual int sampleRate();
      virtual QString encodedURL();
      virtual QString coverImage();
      virtual int score ();
      virtual bool isPlaying();
      virtual int  status();
      virtual bool repeatTrackStatus();
      virtual bool repeatPlaylistStatus();
      virtual bool randomModeStatus();
      virtual void setVolume( int );
      virtual int  getVolume();
      virtual void volumeUp();
      virtual void volumeDown();
      virtual void mute();
      virtual void setEqualizerEnabled( bool active );
      virtual void configEqualizer();
      virtual bool equalizerEnabled();
      virtual void enableOSD( bool enable );
      virtual void showOSD();
      virtual QString setContextStyle(const QString&);
      virtual void setEqualizer(int preamp, int band60, int band170, int band310, int band600, int band1k, int band3k, int band6k, int band12k, int band14k, int band16k);

      virtual void transferCliArgs( QStringList args );
};


class DcopPlaylistHandler : public QObject, virtual public AmarokPlaylistInterface
{
      Q_OBJECT

   public:
      DcopPlaylistHandler();

   public /* DCOP */ slots:
      virtual void    addMedia(const KURL &);
      virtual void    addMediaList(const KURL::List &);
      virtual void    clearPlaylist();
      virtual void    shufflePlaylist();
      virtual QString saveCurrentPlaylist();
      virtual void    removeCurrentTrack();
      virtual void    playByIndex(int);
      virtual int     getActiveIndex();
      virtual void    setStopAfterCurrent(bool);
      virtual void    togglePlaylist();
      virtual void    playMedia(const KURL &);
      virtual void    shortStatusMessage(const QString&);
};


class DcopCollectionHandler : public QObject, virtual public AmarokCollectionInterface
{
   Q_OBJECT

   public:
       DcopCollectionHandler();

   public /* DCOP */ slots:
      virtual QStringList query(const QString& sql);
};


} // namespace amaroK

#endif
