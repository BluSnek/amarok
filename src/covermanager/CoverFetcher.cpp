/****************************************************************************************
 * Copyright (c) 2004 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2004 Stefan Bogner <bochi@online.ms>                                   *
 * Copyright (c) 2004 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2007 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
 * Copyright (c) 2009 Martin Sandsmark <sandsmark@samfundet.no>                         *
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

#include "CoverFetcher.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "CoverFoundDialog.h"
#include "CoverManager.h"
#include "Debug.h"
#include "statusbar/StatusBar.h"
#include "ui_EditCoverSearchDialog.h"

#include <KIO/Job>
#include <KLocale>
#include <KUrl>

#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QRegExp>


CoverFetcher* CoverFetcher::s_instance = 0;

CoverFetcher*
CoverFetcher::instance()
{
    return s_instance ? s_instance : new CoverFetcher();
}

void CoverFetcher::destroy()
{
    if( s_instance )
    {
        delete s_instance;
        s_instance = 0;
    }
}

CoverFetcher::CoverFetcher()
        : QObject()
        , m_interactive( false )
        , m_processedCovers( 0 )
        , m_numURLS( 0 )
        , m_success( true )
        , m_isFetching( false )
{
    DEBUG_FUNC_INFO
    setObjectName( "CoverFetcher" );

    s_instance = this;
}

CoverFetcher::~CoverFetcher()
{
    DEBUG_FUNC_INFO
}

void
CoverFetcher::manualFetch( Meta::AlbumPtr album )
{
    m_interactive = true;
    m_albumsMutex.lock();
    if( m_albums.contains( album ) )
    {
        m_albums.removeAll( album );
    }
    m_albumsMutex.unlock();

    m_currentCoverName = album->hasAlbumArtist()
                       ? album->albumArtist()->prettyName() + ": " + album->prettyName()
                       : album->prettyName();

    startFetch( album );
}

void
CoverFetcher::queueAlbum( Meta::AlbumPtr album )
{
    if( m_albumPtr == album || m_albums.contains( album ) )
        return;
    m_interactive = false;
    m_albumsMutex.lock();
    m_albums << album;
    m_albumsMutex.unlock();
    m_fetchMutex.lock();
    if( !m_isFetching )
    {
        m_fetchMutex.unlock();

        m_albumsMutex.lock();
        if( !m_albums.isEmpty() )
        {
            Meta::AlbumPtr firstAlbum = m_albums.takeFirst();
            startFetch( album );
        }
        m_albumsMutex.unlock();
    }
    m_fetchMutex.unlock();
}
void
CoverFetcher::queueAlbums( Meta::AlbumList albums )
{
    m_interactive = false;
    m_albumsMutex.lock();
    foreach( Meta::AlbumPtr album, albums )
    {
        if( m_albumPtr == album || m_albums.contains( album ) )
            continue;
        m_albums << album;
    }
    m_albumsMutex.unlock();
    m_fetchMutex.lock();
    if( !m_isFetching )
    {
        m_fetchMutex.unlock();

        m_albumsMutex.lock();
        if( !m_albums.isEmpty() )
        {
            Meta::AlbumPtr album = m_albums.takeFirst();
            startFetch( album );
        }
        m_albumsMutex.unlock();
    }
    m_fetchMutex.unlock();
}

void
CoverFetcher::startFetch( Meta::AlbumPtr album )
{
    m_fetchMutex.lock();
    m_isFetching = true;
    m_fetchMutex.unlock();
    m_albumPtr = album;

    // reset all values
    m_xml.clear();

    QUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/2.0/" );
    url.addQueryItem( "api_key", "402d3ca8e9bc9d3cf9b85e1202944ca5" );
    if( album->hasAlbumArtist() )
    {
        url.addQueryItem( "method", "album.getinfo" );
        url.addQueryItem( "artist", album->albumArtist()->name() );
    }
    else
    {
        url.addQueryItem( "method", "album.search" );
    }
    url.addQueryItem( "album", album->name() );

    m_urlMap.insert( url, album );
    debug() << url;

    KJob* job = KIO::storedGet( url, KIO::NoReload, KIO::HideProgressInfo );
    connect( job, SIGNAL(result( KJob* )), SLOT(finishedXmlFetch( KJob* )) );

    if( m_interactive )
        The::statusBar()->newProgressOperation( job, i18n( "Fetching Cover" ) );
}

void
CoverFetcher::finishedXmlFetch( KJob *job ) //SLOT
{
    // NOTE: job can become 0 when this method is called from attemptAnotherFetch()
    if( job && job->error() )
    {
        finish( Error, i18n( "There was an error communicating with last.fm." ), job );
        return;
    }

    if( job )
    {
        KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
        m_xml = QString::fromUtf8( storedJob->data().data(), storedJob->data().size() );
    }

    QDomDocument doc;
    if( !doc.setContent( m_xml ) )
    {
        finish( Error, i18n( "The XML obtained from Last.fm is invalid." ), job );
        return;
    }

    m_pixmaps.clear();
    m_processedCovers = 0;
    m_numURLS = 0;

    const QUrl jobUrl          = dynamic_cast< KIO::TransferJob* >( job )->url();
    const QString queryMethod  = jobUrl.queryItemValue( "method" );
    Meta::AlbumPtr album       = m_urlMap[ jobUrl ];

    QString albumArtist;
    QDomNodeList results;
    QSet< QString > artistSet;
    if( queryMethod == "album.getinfo" )
    {
        results = doc.documentElement().childNodes();
        albumArtist = normalizeString( album->albumArtist()->name() );
    }
    else if( queryMethod == "album.search" )
    {
        results = doc.documentElement().namedItem( "results" ).namedItem( "albummatches" ).childNodes();

        const Meta::TrackList tracks = album->tracks();
        QStringList artistNames( "Various Artists" );
        foreach( const Meta::TrackPtr &track, tracks )
        {
            artistNames << track->artist()->name();
        }
        artistSet = normalizeStrings( artistNames ).toSet();
    }
    else return;


    for( uint x = 0, len = results.length(); x < len; x++ )
    {
        const QDomNode albumNode = results.item( x );
        const QString artist = normalizeString( albumNode.namedItem( "artist" ).toElement().text() );

        if( queryMethod == "album.getinfo" && artist != albumArtist )
            continue;
        else if( queryMethod == "album.search" && !artistSet.contains( artist ) )
            continue;

        QString coverUrl;
        const QDomNodeList list = albumNode.childNodes();
        for( int i = 0, count = list.count(); i < count; ++i )
        {
            const QDomNode &node = list.item( i );
            if( node.nodeName() == "image" && node.hasAttributes() )
            {
                const QString imageSize = node.attributes().namedItem( "size" ).nodeValue();
                if( imageSize == coverSizeString( ExtraLarge ) && node.isElement() )
                {
                    coverUrl = node.toElement().text();
                }
            }
        }

        if( coverUrl.isEmpty() )
        {
            continue;
        }

        m_numURLS++;

        //FIXME: Asyncronous behaviour without informing the user is bad in this case
        KJob* getJob = KIO::storedGet( KUrl(coverUrl), KIO::NoReload, KIO::HideProgressInfo );
        connect( getJob, SIGNAL( result( KJob* ) ), SLOT( finishedImageFetch( KJob* ) ) );
    }

    m_urlMap.remove( jobUrl );

    if ( m_numURLS == 0 )
        finish( NotFound );
}

void
CoverFetcher::finishedImageFetch( KJob *job ) //SLOT
{
    QPixmap pixmap;
    KIO::StoredTransferJob* storedJob = static_cast<KIO::StoredTransferJob*>( job );
    const QByteArray data = storedJob->data();
    
    if( job->error() || data.isNull() || !pixmap.loadFromData( data ) )
    {
        debug() << "finishedImageFetch(): KIO::error(): " << job->error();
        finish( Error, i18n( "The cover could not be retrieved." ), job );
        return;
    }
    else
    {
        m_pixmaps.append( pixmap );
    }

    m_processedCovers++;

    if( m_processedCovers == m_numURLS )
    {
        if( !m_pixmaps.isEmpty() )
        {
            if( m_interactive )
            {
                //yay! images found :)
                //lets see if the user wants one of it
                m_processedCovers = 9999; //prevents to popup a 2nd window
                showCover();
            }
            else
            {
                m_selPixmap = m_pixmaps.takeFirst();
                //image loaded successfully yay!
                finish();
            }
        }
        else
        {
            finish( NotFound );
        }
    }

    The::statusBar()->endProgressOperation( job ); //just to be safe...
}

void
CoverFetcher::showCover()
{
    CoverFoundDialog dialog( static_cast<QWidget*>( parent() ), m_pixmaps, m_currentCoverName );

    switch( dialog.exec() )
    {
    case KDialog::Accepted:
        m_selPixmap = QPixmap( dialog.image() );
        finish();
        break;
    case KDialog::Rejected: //make sure we do not show any more dialogs
        debug() << "cover rejected";
        break;
    default:
        finish( Error, i18n( "Aborted." ) );
        break;
    }
}

QString
CoverFetcher::coverSizeString( enum CoverSize size ) const
{
    QString str;
    switch( size )
    {
        case Small:  str = "small";      break;
        case Medium: str = "medium";     break;
        case Large:  str = "large";      break;
        default:     str = "extralarge"; break;
    }
    return str;
}

QString
CoverFetcher::normalizeString( const QString &raw )
{
    const QRegExp spaceRegExp  = QRegExp( "\\s" );
    return raw.toLower().remove( spaceRegExp ).normalized( QString::NormalizationForm_KC );
}

QStringList
CoverFetcher::normalizeStrings( const QStringList &rawList )
{
    QStringList cooked;
    foreach( const QString &raw, rawList )
    {
        cooked << normalizeString( raw );
    }
    return cooked;
}

void
CoverFetcher::finish( CoverFetcher::FinishState state, const QString &message, KJob *job )
{
    DEBUG_BLOCK
    switch( state )
    {
        case Success:
        {
            The::statusBar()->shortMessage( i18n( "Retrieved cover successfully" ) );
            m_albumPtr->setImage( m_selPixmap );
            m_success = true;
            break;
        }

        case Error:
        {
            if( job )
                warning() << message << "KIO::error(): " << job->errorText();

            m_errors += message;
            m_success = false;

            debug() << "Album name" << m_albumPtr->name();
            break;
        }

        case NotFound:
        {
            const QString text = i18n( "Unable to find a cover for %1.", m_albumPtr->name() );
            //FIXME: Not visible behind cover manager
            if( m_interactive )
                The::statusBar()->longMessage( text, StatusBar::Sorry );
            else
                The::statusBar()->shortMessage( text );

            m_errors += text;
            m_success = false;
            break;
        }
    }

    emit finishedSingle( static_cast< int >( state ) );

    m_albumsMutex.lock();
    if( !m_interactive /*manual fetch*/ && !m_albums.isEmpty() )
    {
        debug() << "CoverFetcher::finish() next album:" << m_albums[0]->name();
        startFetch( m_albums.takeFirst() );
    }
    m_albumsMutex.unlock();

    m_fetchMutex.lock();
    m_isFetching = false;
    m_fetchMutex.unlock();
}

#include "CoverFetcher.moc"

