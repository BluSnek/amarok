/***************************************************************************
 * copyright            : (C) 2006 Ian Monroe <ian@monroe.nu>              *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "amarok.h"
#include "debug.h"
#include "daapserver.h"
#include "collectiondb.h"

#include <kstandarddirs.h>
#include <kuser.h>
#if DNSSD_SUPPORT
    #include <dnssd/publicservice.h>
//Added by qt3to4:
#include <QByteArray>
#endif
DaapServer::DaapServer(QObject* parent, char* name)
  : QObject( parent, name )
  , m_service( 0 )
{
    DEBUG_BLOCK

    m_server = new K3ProcIO();
    m_server->setComm( K3Process::All );
    *m_server << "amarok_daapserver.rb";
    *m_server << KStandardDirs::locate( "data", "amarok/ruby_lib/" );
    *m_server << KStandardDirs::locate( "lib", "ruby_lib/" );
    *m_server << KStandardDirs::locate( "data", "amarok/scripts/ruby_debug/debug.rb" );
    if( !m_server->start( K3ProcIO::NotifyOnExit, true ) ) {
        error() << "Failed to start amarok_daapserver.rb";
        return;
    }

    connect( m_server, SIGNAL( readReady( K3ProcIO* ) ), this, SLOT( readSql() ) );
}

DaapServer::~DaapServer()
{
    #if DNSSD_SUPPORT
        delete m_service;
    #endif
    delete m_server;
}

void
DaapServer::readSql()
{
    static const QByteArray sqlPrefix = "SQL QUERY: ";
    static const QByteArray serverStartPrefix = "SERVER STARTING: ";
    QString line;
    while( m_server->readln( line ) != -1 )
    {
        if( line.startsWith( sqlPrefix ) )
        {
            line.remove( 0, sqlPrefix.length() );
            debug() << "sql run " << line;
            m_server->writeStdin( CollectionDB::instance()->query( line ).join("\n") );
            m_server->writeStdin( "**** END SQL ****" );
        }
        else if( line.startsWith( serverStartPrefix ) )
        {
            line.remove( 0, serverStartPrefix.length() );
            debug() << "Server starting on port " << line << '.';
            #if DNSSD_SUPPORT
                KUser current;
                if( !m_service )
                    m_service = new DNSSD::PublicService( i18n("%1's Amarok Share", current.fullName() ), "_daap._tcp", line.toInt() );
                    debug() << "port number: " << line.toInt();
                m_service->publishAsync();
            #endif
        }
        else
            debug() << "server says " << line;
   }
   //m_server->ackRead();
   //m_server->enableReadSignals(true);
}

#include "daapserver.moc"

