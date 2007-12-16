/***************************************************************************
 * copyright            : (C) 2006 Ian Monroe <ian@monroe.nu> 
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy 
 * defined in Section 14 of version 3 of the license.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/
#include "amarok.h"
#include "amarokconfig.h"
#include "daapclient.h"
#include "daapreader/authentication/hasher.h"
#include "debug.h"
#include "AmarokProcess.h"
#include "proxy.h"

#include <kapplication.h>

using namespace Daap;

//input url: daap://host:port/databaseId/music.ext
/*
        bundle->setUrl( Amarok::QStringx("http://%1:3689/databases/%2/items/%3.%4?%5").args(
            QStringList() << m_host
                        << m_databaseId
                        << QString::number( (*it).asMap()["miid"].asList()[0].asInt() )
                        << (*it).asMap()["asfm"].asList()[0].asString()
                        << m_loginString ) );

*/
Proxy::Proxy(KUrl stream, DaapClient* client, const char* name)
    : QObject(client, name)
    , m_proxy( new AmarokProcIO() )
{
    DEBUG_BLOCK
    //find the request id and increment it
    const QString hostKey = stream.host() + ':' + QString::number(stream.port());
    const int revisionId = client->incRevision( hostKey );
    const int sessionId = client->getSession( hostKey );
    //compose URL
    KUrl realStream = realStreamUrl( stream, sessionId );

    //get hash
    char hash[33] = {0};
    GenerateHash( 3
        , reinterpret_cast<const unsigned char*>((realStream.path() + realStream.query()).ascii())
        , 2
        , reinterpret_cast<unsigned char*>(hash)
        , revisionId );

    // Find free port
    MyServerSocket* socket = new MyServerSocket();
    const int port = socket->port();
    debug() << "Proxy server using port: " << port;
    delete socket;
    m_proxyUrl = KUrl( QString("http://localhost:%1/daap.mp3").arg( port ) );
    //start proxy
    m_proxy->setOutputChannelMode( AmarokProcIO::MergedChannels );
    *m_proxy << "amarok_proxy.rb";
    *m_proxy << "--daap";
    *m_proxy << QString::number( port );
    *m_proxy << realStream.url();
    *m_proxy << AmarokConfig::soundSystem();
    *m_proxy << hash;
    *m_proxy << QString::number( revisionId );
    *m_proxy << Amarok::proxyForUrl( realStream.url() );

    m_proxy->start();
    if( m_proxy->error() == AmarokProcIO::FailedToStart ) {
        error() << "Failed to start amarok_proxy.rb";
        return;
    }

    QString line;
    while( true ) {
        kapp->processEvents();
        m_proxy->readln( line );
        if( line == "AMAROK_PROXY: startup" ) break;
    }
    debug() << "started amarok_proxy.rb --daap " << QString::number( port ) << ' ' << realStream.url() << ' ' << AmarokConfig::soundSystem() << ' ' << hash << ' ' << revisionId;
    connect( m_proxy, SIGNAL( finished( int ) ), this, SLOT( playbackStopped() ) );
    connect( m_proxy, SIGNAL( readReady( AmarokProcIO* ) ), this, SLOT( readProxy() ) );
}

Proxy::~Proxy()
{
    delete m_proxy;
}

void
Proxy::playbackStopped()
{
    deleteLater();
}

void
Proxy::readProxy()
{
    QString line;

    while( m_proxy->readln( line ) != -1 )
    {
        debug() << line;
    }
}

KUrl Proxy::realStreamUrl( KUrl fakeStream, int sessionId )
{
    KUrl realStream;
    realStream.setProtocol( "http" );
    realStream.setHost(fakeStream.host());
    realStream.setPort(fakeStream.port());
    realStream.setPath( "/databases" + fakeStream.directory() + "/items/" + fakeStream.fileName() );
    realStream.setQuery( QString("?session-id=") + QString::number(sessionId) );
    return realStream;
}

#include "proxy.moc"
