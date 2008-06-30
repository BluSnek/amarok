/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "LyricsEngine.h"

#include "Amarok.h"
#include "Debug.h"
#include "ContextObserver.h"
#include "ContextView.h"
#include "EngineController.h"
#include "ScriptManager.h"

#include <QDomDocument>

using namespace Context;

LyricsEngine::LyricsEngine( QObject* parent, const QList<QVariant>& /*args*/ )
    : DataEngine( parent )
    , ContextObserver( ContextView::self() )
    , LyricsObserver( LyricsManager::self() )
{
    m_requested = true; // testing
}

QStringList LyricsEngine::sources() const
{
    QStringList sourcesList;
    sourcesList << "lyrics";
    return sourcesList; // we don't have pre-set sources, as there is only
    // one source---lyrics.
}

bool LyricsEngine::sourceRequested( const QString& name )
{
    Q_UNUSED( name )
    m_requested = true; // someone is asking for data, so we turn ourselves on :)
    removeAllData( name );
    setData( name, QVariant());
    update();
    return true;
}

void LyricsEngine::message( const ContextState& state )
{
    if( state == Current && m_requested )
        update();
}

void LyricsEngine::update()
{

    Meta::TrackPtr curtrack = The::engineController()->currentTrack();
    if( !curtrack )
        return;
    QString lyrics = curtrack->cachedLyrics();
    // don't rely on caching for streams
    const bool cached = !lyrics.isEmpty() && !The::engineController()->isStream();

    QString title  = The::engineController()->currentTrack()->name();
    QString artist = The::engineController()->currentTrack()->artist()->name();

    if( title.contains("PREVIEW: buy it at www.magnatune.com", Qt::CaseSensitive) )
        title = title.remove(" (PREVIEW: buy it at www.magnatune.com)");
    if( artist.contains("PREVIEW: buy it at www.magnatune.com", Qt::CaseSensitive) )
        artist = artist.remove(" (PREVIEW: buy it at www.magnatune.com)");

    if ( title.isEmpty() )
    {
        /* If title is empty, try to use pretty title.
           The fact that it often (but not always) has "artist name" together, can be bad,
           but at least the user will hopefully get nice suggestions. */
        QString prettyTitle = The::engineController()->currentTrack()->prettyName();
        int h = prettyTitle.indexOf( '-' );
        if ( h != -1 )
        {
            title = prettyTitle.mid( h+1 ).trimmed();
            if( title.contains("PREVIEW: buy it at www.magnatune.com", Qt::CaseSensitive) )
                title = title.remove(" (PREVIEW: buy it at www.magnatune.com)");
            if ( artist.isEmpty() ) {
                artist = prettyTitle.mid( 0, h ).trimmed();
                if( artist.contains("PREVIEW: buy it at www.magnatune.com", Qt::CaseSensitive) )
                    artist = artist.remove(" (PREVIEW: buy it at www.magnatune.com)");
            }

        }
    }

    if( ( !cached ) && ScriptManager::instance()->lyricsScriptRunning().isEmpty() ) // no lyrics, and no lyrics script!
    {
        removeAllData( "lyrics" );
        setData( "lyrics", "noscriptrunning", "noscriptrunning" );
        return;
    }

    if( cached )
        LyricsManager::self()->lyricsResult( lyrics.toUtf8(), true );
    else
    { // fetch by lyrics script
        removeAllData( "lyrics" );
        setData( "lyrics", "fetching", "fetching" );
        ScriptManager::instance()->notifyFetchLyrics( artist, title );

    }

}

void LyricsEngine::newLyrics( QStringList& lyrics )
{
    removeAllData( "lyrics" );
    setData( "lyrics", "lyrics", lyrics );
}

void LyricsEngine::lyricsMessage( QString& msg )
{
    removeAllData( "lyrics" );
    setData( "lyrics", msg, msg );
}

#include "LyricsEngine.moc"

