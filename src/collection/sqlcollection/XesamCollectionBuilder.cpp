/* This file is part of the KDE project
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

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

#include "XesamCollectionBuilder.h"

#include "debug.h"
#include "meta/MetaUtility.h"
#include "mountpointmanager.h"
#include "sqlcollection.h"
#include "XesamDbus.h"

#include <QBuffer>
#include <QDBusConnection>
#include <QDBusReply>
#include <QDir>
#include <QXmlStreamWriter>

#include <kurl.h>

static const QString XESAM_NS = "http://freedesktop.org/standards/xesam/1.0/query";

#define DEBUG_XML true

XesamCollectionBuilder::XesamCollectionBuilder( SqlCollection *collection )
    : QObject( collection )
    , m_collection( collection )
{
    DEBUG_BLOCK
    m_xesam = new OrgFreedesktopXesamSearchInterface( "org.freedesktop.xesam.searcher",
                                                  "/org/freedesktop/xesam/searcher/main",
                                                  QDBusConnection::sessionBus() );
    if( m_xesam->isValid() )
    {
        connect( m_xesam, SIGNAL( HitsAdded( QString , int ) ), SLOT( slotHitsAdded( QString, int ) ) );
        connect( m_xesam, SIGNAL( HitsModified( QString, QList<int> ) ), SLOT( slotHitsModified( QString, QList<int> ) ) );
        connect( m_xesam, SIGNAL( HitsRemoved( QString, QList<int> ) ), SLOT( slotHitsRemoved( QString, QList<int> ) ) );
        QDBusReply<QString> sessionId = m_xesam->NewSession();
        if( !sessionId.isValid() )
        {
            debug() << "Could not acquire Xesam session, aborting. error was: " << sessionId.error();
            return;
            //TODO error handling
        }
        m_session = sessionId.value();
        if( !setupXesam() )
        {
            debug() << "Warning, could not setup xesam correctly";
        }
        QDBusReply<QString> search = m_xesam->NewSearch( m_session, generateXesamQuery() );
        if( search.isValid() )
        {
            m_search = search.value();
            m_xesam->StartSearch( m_search );
        }
        else
        {
            debug() << "Invalid response for NewSearch";
        }
    }
    else
    {
        //TODO display warning about unavailable xesam daemon
    }

}

XesamCollectionBuilder::~XesamCollectionBuilder()
{
    if( m_xesam && m_xesam->isValid() )
        m_xesam->CloseSession( m_session );
}

bool
XesamCollectionBuilder::setupXesam()
{
    bool status = true;
    if( !m_xesam->SetProperty( m_session, "search.live", QDBusVariant( true ) ).value().variant().toBool() )
    {
        debug() << "could not select xesam live search mode";
        status = false;
    }
    QStringList fields;
    fields << Meta::Field::URL << Meta::Field::TITLE << Meta::Field::ALBUM << Meta::Field::ARTIST << Meta::Field::GENRE;
    fields << Meta::Field::COMPOSER << Meta::Field::YEAR << Meta::Field::COMMENT << Meta::Field::CODEC;
    fields << Meta::Field::BITRATE << Meta::Field::BPM << Meta::Field::TRACKNUMBER << Meta::Field::DISCNUMBER;
    fields << Meta::Field::FILESIZE << Meta::Field::LENGTH << Meta::Field::SAMPLERATE;
    m_xesam->SetProperty( m_session, "hit.fields", QDBusVariant( fields ) );
    QStringList fieldsExtended;
    m_xesam->SetProperty( m_session, "hit.fields.extended", QDBusVariant( fieldsExtended ) );
    m_xesam->SetProperty( m_session, "sort.primary", QDBusVariant( Meta::Field::URL ) );
    m_xesam->SetProperty( m_session, "search.blocking", QDBusVariant( false ) );
    return status;
}

void
XesamCollectionBuilder::slotHitsAdded( const QString &search, int count )
{
    DEBUG_BLOCK
    if( m_search != search )
        return;
    debug() << "New Xesam hits: " << count;
    QDBusReply<VariantListVector> reply = m_xesam->GetHits( m_search, count );
    if( reply.isValid() )
    {
        VariantListVector result = reply.value();
        if( result.isEmpty() )
            return;
        KUrl firstUrl( result[0][0].toString() );
        QString dir = firstUrl.directory();
        QList<QList<QVariant> > dirData;
        //rows are sorted by directory/uri
        foreach( QList<QVariant> row, result )
        {
            KUrl url( row[0].toString() );
            if( url.directory() == dir )
            {
                dirData.append( row );
            }
            else
            {
                processDirectory( dirData );
                dirData.clear();
                dir = url.directory();
            }
        }
    }
}

void
XesamCollectionBuilder::slotHitsModified( const QString &search, const QList<int> &hit_ids )
{
    DEBUG_BLOCK
    if( m_search != search )
        return;
}

void
XesamCollectionBuilder::slotHitsRemoved( const QString &search, const QList<int> &hit_ids )
{
    DEBUG_BLOCK
    if( m_search != search )
        return;
}

void
XesamCollectionBuilder::searchDone( const QString &search )
{
    DEBUG_BLOCK
    if( m_search != search )
        return;
}

QString
XesamCollectionBuilder::generateXesamQuery() const
{
    QStringList collectionFolders = MountPointManager::instance()->collectionFolders();
    QString result;
    QXmlStreamWriter writer( &result );
    writer.setAutoFormatting( DEBUG_XML );
    writer.writeStartElement( XESAM_NS, "request" );
    writer.writeStartElement( XESAM_NS, "query" );
    writer.writeAttribute( XESAM_NS, "content", "xesam:Audio" );
    if( collectionFolders.size() <= 1 )
    {
        QString folder = collectionFolders.isEmpty() ? QDir::homePath() : collectionFolders[0];
        writer.writeStartElement( XESAM_NS, "startsWith" );
        writer.writeTextElement( XESAM_NS, "field", "dc:uri" );
        writer.writeTextElement( XESAM_NS, "string", folder );
        writer.writeEndElement();
    }
    else
    {
        writer.writeStartElement( XESAM_NS, "or" );
        foreach( QString folder, collectionFolders )
        {
            writer.writeStartElement( XESAM_NS, "startsWith" );
            writer.writeTextElement( XESAM_NS, "field", "dc:uri" );
            writer.writeTextElement( XESAM_NS, "string", folder );
            writer.writeEndElement();
        }
        writer.writeEndElement();
    }
    writer.writeEndDocument();
    if( DEBUG_XML )
        debug() << result;
    return result;
}

void
XesamCollectionBuilder::processDirectory( const QList<QList<QVariant> > &data )
{
    //URL TITLE ALBUM ARTIST GENRE COMPOSER YEAR COMMENT CODEC BITRATE BPM TRACKNUMBER DISCNUMBER FILESIZE LENGTH SAMPLERATE
    QSet<QString> artists;
    QString album;
    bool multipleAlbums = false;
    if( !data.isEmpty() )
        album = data[0][2].toString();
    foreach(QList<QVariant> row, data )
    {
        artists.insert( row[3].toString() );
        if( row[2].toString() != album )
            multipleAlbums = true;
    }
}

void
XesamCollectionBuilder::addTrack( const QList<QVariant> &trackData, int albumArtistId )
{
    int album = albumId( trackData[2].toString(), albumArtistId );
    int artist = artistId( trackData[3].toString() );
    int genre = genreId( trackData[4].toString() );
    int composer = composerId( trackData[5].toString() );
    int year = yearId( trackData[6].toString() );
    int url = urlId( trackData[0].toString() );

    QString insert = "INSERT INTO tracks(url,artist,album,genre,composer,year,title,comment,"
                     "tracknumber,discnumber,bitrate,length,samplerate,filesize,filetype,bpm"
                     "createdate,modifydate) VALUES ( %1,%2,%3,%4,%5,%6,'%7','%8'"; //goes up to comment
    insert = insert.arg( url ).arg( artist ).arg( album ).arg( genre ).arg( composer ).arg( year );
    insert = insert.arg( m_collection->escape( trackData[1].toString() ), m_collection->escape( trackData[7].toString() ) );

    QString insert2 = ",%1,%2,%3,%4,%5,%6,%7,%8,%9,%10);";
    insert2 = insert2.arg( trackData[11].toInt() ).arg( trackData[12].toInt() ).arg( trackData[9].toInt() );
    insert2 = insert2.arg( trackData[14].toInt() ).arg( trackData[15].toInt() ).arg( trackData[13].toInt() );
    insert2 = insert2.arg( "0", "0", "0" ); //bpm, createdate, modifydate not implemented yet
    insert += insert2;

    m_collection->insert( insert, "tracks" );
}

int
XesamCollectionBuilder::artistId( const QString &artist )
{
    if( m_artists.contains( artist ) )
        return m_artists.value( artist );
    QString query = QString( "SELECT id FROM artists WHERE name = '%1';" ).arg( m_collection->escape( artist ) );
    QStringList res = m_collection->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO albums( name ) VALUES ('%1');" ).arg( m_collection->escape( artist ) );
        int id = m_collection->insert( insert, "albums" );
        m_artists.insert( artist, id );
        return id;
    }
    else
    {
        int id = res[0].toInt();
        m_artists.insert( artist, id );
        return id;
    }
}

int
XesamCollectionBuilder::genreId( const QString &genre )
{
    if( m_genre.contains( genre ) )
        return m_genre.value( genre );
    QString query = QString( "SELECT id FROM genres WHERE name = '%1';" ).arg( m_collection->escape( genre ) );
    QStringList res = m_collection->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO genres( name ) VALUES ('%1');" ).arg( m_collection->escape( genre ) );
        int id = m_collection->insert( insert, "genre" );
        m_genre.insert( genre, id );
        return id;
    }
    else
    {
        int id = res[0].toInt();
        m_genre.insert( genre, id );
        return id;
    }
}

int
XesamCollectionBuilder::composerId( const QString &composer )
{
    if( m_composer.contains( composer ) )
        return m_composer.value( composer );
    QString query = QString( "SELECT id FROM composers WHERE name = '%1';" ).arg( m_collection->escape( composer ) );
    QStringList res = m_collection->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO composers( name ) VALUES ('%1');" ).arg( m_collection->escape( composer ) );
        int id = m_collection->insert( insert, "composers" );
        m_composer.insert( composer, id );
        return id;
    }
    else
    {
        int id = res[0].toInt();
        m_composer.insert( composer, id );
        return id;
    }
}

int
XesamCollectionBuilder::yearId( const QString &year )
{
    if( m_year.contains( year ) )
        return m_year.value( year );
    QString query = QString( "SELECT id FROM years WHERE name = '%1';" ).arg( m_collection->escape( year ) );
    QStringList res = m_collection->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO years( name ) VALUES ('%1');" ).arg( m_collection->escape( year ) );
        int id = m_collection->insert( insert, "years" );
        m_year.insert( year, id );
        return id;
    }
    else
    {
        int id = res[0].toInt();
        m_year.insert( year, id );
        return id;
    }
}

int 
XesamCollectionBuilder::albumId( const QString &album, int artistId )
{
    QPair<QString, int> key( album, artistId );
    if( m_albums.contains( key ) )
        return m_albums.value( key );

    QString query = QString( "SELECT id FROM albums WHERE artist = %1 AND name = '%2';" )
                        .arg( QString::number( artistId ), m_collection->escape( album ) );
    QStringList res = m_collection->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO albums(artist, name) VALUES( %1, '%2' );" )
                    .arg( QString::number( artistId ), m_collection->escape( album ) );
        int id = m_collection->insert( insert, "albums" );
        m_albums.insert( key, id );
        return id;
    }
    else
    {
        int id = res[0].toInt();
        m_albums.insert( key, id );
        return id;
    }
}

int
XesamCollectionBuilder::urlId( const QString &url )
{
    int deviceId = MountPointManager::instance()->getIdForUrl( url );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceId, url );
    //don't bother caching the data, we only call this method for each url once
    QString query = QString( "SELECT id FROM urls WHERE deviceid = %1 AND rpath = '%2';" )
                        .arg( QString::number( deviceId ), m_collection->escape( rpath ) );
    QStringList res = m_collection->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO urls(deviceid, rpath) VALUES ( %1, '%2' );" )
                            .arg( QString::number( deviceId ), m_collection->escape( rpath ) );
        return m_collection->insert( insert, "urls" );
    }
    else
    {
        return res[0].toInt();
    }
}

#include "XesamCollectionBuilder.moc"
