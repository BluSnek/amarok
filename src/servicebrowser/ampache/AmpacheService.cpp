/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "AmpacheService.h"

#include "Amarok.h"
#include "AmpacheConfig.h"
#include "collection/CollectionManager.h"
#include "Debug.h"
#include "StatusBar.h"


#include <KMessageBox>
#include <kpassworddialog.h>
#include <KMD5>

#include <QDomDocument>

AMAROK_EXPORT_PLUGIN( AmpacheServiceFactory )

void AmpacheServiceFactory::init()
{
    //read config and create the needed number of services
    AmpacheConfig config;
    AmpacheServerList servers = config.servers();

    for( int i = 0; i < servers.size(); i++ )
    {
        AmpacheServerEntry server = servers.at( i );
        ServiceBase* service = new AmpacheService( "Ampache (" + server.name + ')', server.url, server. username, server.password );
        m_activeServices << service;
        debug() << "Emitting service!!!!!!";
        emit newService( service );
    }
}

QString
AmpacheServiceFactory::name()
{
    return "Ampache";
}

KPluginInfo
AmpacheServiceFactory::info()
{
    KPluginInfo pluginInfo( "amarok_service_ampache.desktop", "services" );
    pluginInfo.setConfig( config() );
    return pluginInfo;
}

KConfigGroup
AmpacheServiceFactory::config()
{
    return Amarok::config( "Service_Ampache" );
}

bool
AmpacheServiceFactory::possiblyContainsTrack(const KUrl & url) const
{
    DEBUG_BLOCK
    debug() << "url: " << url.url();
    AmpacheConfig config;
    foreach( AmpacheServerEntry server, config.servers() )
    {
        if ( url.url().contains( server.url, Qt::CaseInsensitive ) )
        {
            debug() << "plausible...";
            return true;
        }
    }

    debug() << "do not think so...";

    return false;
}


AmpacheService::AmpacheService(const QString & name, const QString &url, const QString &username, const QString &password )
    : ServiceBase( name )
    , m_authenticated( false )
    , m_server ( QString() )
    , m_sessionId ( QString() )
    , m_collection( 0 )
{
    setShortDescription( i18n( "Use Amarok as a seamless frontend to your Ampache server!" ) );
    setIcon( KIcon( "get-hot-new-stuff-amarok" ) );

    //we are using http queries later on, so we require
    KUrl kurl;
    if( url.contains( "//" ) )
    {
        kurl.setUrl( url, KUrl::TolerantMode );
        if( kurl.protocol() != "http" && kurl.protocol() != "https" )
        {
            kurl.setProtocol( "http" );
        }
    }
    else
    {
        kurl.setProtocol( "http" );
        kurl.setAuthority( url );
    }
    m_server = kurl.url();
    m_username = username;
    m_password = password;

    //FIXME: HACK:  Force a reauthentication every hour to prevent the session from timing out.
    // This should really be fixed by parsing a pretty error code from ampache
     // and reauthenticating when it tells us the session is over
    // However, vollmer needs to break out the error code for us to do that.
    // ~hydrogen
    QTimer *t = new QTimer(this);
    connect(t, SIGNAL( timeout() ), SLOT(authenticate() ) );
    t->start( 3600000 );

    authenticate( );
}

AmpacheService::~AmpacheService()
{
    CollectionManager::instance()->removeUnmanagedCollection( m_collection );
    delete m_collection;
}

void
AmpacheService::polish()
{
    m_bottomPanel->hide();

    /*if ( !m_authenticated )
        authenticate( );*/
}

void
AmpacheService::authenticate()
{
    //lets keep this around for now if we want to allow pwople to add a service that prompts for stuff
    if ( m_server.isEmpty() || m_password.isEmpty() )
    {
        KPasswordDialog dlg( 0 , KPasswordDialog::ShowUsernameLine );  //FIXME 0x02 = KPasswordDialog::showUsername according to api, but that does not work
        dlg.setPrompt( i18n( "Enter the server name and a password" ) );
        if( !dlg.exec() )
            return; //the user canceled

        KUrl kurl( dlg.username() );
        if( kurl.protocol() != "http" && kurl.protocol() != "https" )
        {
            kurl.setProtocol( "http" );
        }
        m_server = kurl.url();
        m_password = dlg.password();

    }
    else
    {
        KUrl kurl( m_server );
        if( kurl.protocol() != "http" && kurl.protocol() != "https" )
        {
            kurl.setProtocol( "http" );
        }
        m_server = kurl.url();
    }

    QString timestamp = QString::number( QDateTime::currentDateTime().toTime_t() );

    QString rawHandshake = timestamp + m_password;
    KMD5 context( rawHandshake.toUtf8() );

    QString passPhrase = context.hexDigest().data();

    QString authenticationString = "<server>/server/xml.server.php?action=handshake<username>&auth=<passphrase>&timestamp=<timestamp>";

    authenticationString.replace(QString("<server>"), m_server);
    if ( !m_username.isEmpty() )
        authenticationString.replace(QString("<username>"), "&user=" + m_username);
    else
        authenticationString.replace(QString("<username>"), "");
    authenticationString.replace(QString("<passphrase>"), passPhrase);
    authenticationString.replace(QString("<timestamp>"), timestamp);

    debug() << "Authenticating with string: " << authenticationString;

    m_xmlDownloadJob = KIO::storedGet( authenticationString, KIO::NoReload, KIO::HideProgressInfo );
    /*connect( m_xmlDownloadJob, SIGNAL(result(KJob *)), this, SLOT( authenticationComplete( KJob*) ) );
    The::statusBar() ->newProgressOperation( m_xmlDownloadJob )
    .setDescription( i18n( "Authenticating" ) );*/

    if ( m_xmlDownloadJob->exec() )
        authenticationComplete( m_xmlDownloadJob );
}

void AmpacheService::authenticationComplete(KJob * job)
{
    if( !job->error() == 0 )
    {
        //TODO: error handling here
        return;
    }
    if( job != m_xmlDownloadJob )
        return; //not the right job, so let's ignore it

    QString xmlReply = ((KIO::StoredTransferJob* )job)->data();
    debug() << "Authentication reply: " << xmlReply;

    //so lets figure out what we got here:
    QDomDocument doc( "reply" );

    doc.setContent( m_xmlDownloadJob->data() );
    QDomElement root = doc.firstChildElement("root");

    //is this an error?

    QDomElement error = root.firstChildElement("error");

    if ( !error.isNull() )
    {
        KMessageBox::error ( this, error.text(), i18n( "Authentication Error!" ) );
    }
    else
    {
        //find status code:
        QDomElement element = root.firstChildElement("auth");

        m_sessionId = element.text();

        m_authenticated = true;

        m_collection = new AmpacheServiceCollection( this, m_server, m_sessionId );
        CollectionManager::instance()->addUnmanagedCollection( m_collection );
        QList<int> levels;
        levels << CategoryId::Artist << CategoryId::Album;
        setModel( new SingleCollectionTreeItemModel( m_collection, levels ) );
    }
    m_xmlDownloadJob->deleteLater();
}

#include "AmpacheService.moc"

