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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#include "jamendodatabasehandler.h"

#include "CollectionManager.h"
#include "debug.h"
#include "SqlStorage.h"

using namespace Meta;

JamendoDatabaseHandler::JamendoDatabaseHandler()
{}


JamendoDatabaseHandler::~JamendoDatabaseHandler()
{}

void
JamendoDatabaseHandler::createDatabase( )
{
    //Get database instance
    CollectionDB *db = CollectionDB::instance();

    QString genreAutoIncrement = "";

    if ( db->getDbConnectionType() == DbConnection::postgresql )
    {
        db->query( QString( "CREATE SEQUENCE jamendo_genre_seq;" ) );

        genreAutoIncrement  = QString( "DEFAULT nextval('jamendo_genre_seq')" );

    }
    else if ( db->getDbConnectionType() == DbConnection::mysql )
    {
        genreAutoIncrement = "AUTO_INCREMENT";
    }

    // create table containing tracks
    QString queryString = "CREATE TABLE jamendo_tracks ("
                          "id INTEGER PRIMARY KEY, "
                          "name " + db->textColumnType() + ',' +
                          "track_number INTEGER,"
                          "length INTEGER,"
                          "preview_url " + db->exactTextColumnType() + ',' +
                          "album_id INTEGER,"
                          "artist_id INTEGER );";

    debug() << "Creating jamendo_tracks: " << queryString;


    QStringList result = db->query( queryString );

    //Create album table
    queryString = "CREATE TABLE jamendo_albums ("
                  "id INTEGER PRIMARY KEY, "
                  "name " + db->textColumnType() + ',' +
                  "description " + db->exactTextColumnType() + ',' +
                  "popularity FLOAT, " +
                  "cover_url " + db->exactTextColumnType() + ',' +
                  "launch_year Integer, "
                  "genre " + db->exactTextColumnType() + ',' +
                  "artist_id INTEGER );";

    debug() << "Creating jamendo_albums: " << queryString;

    result = db->query( queryString );

    //Create artist table
    queryString = "CREATE TABLE jamendo_artists ("
                  "id INTEGER PRIMARY KEY, "
                  "name " + db->textColumnType() + ',' +
                  "description " + db->textColumnType() + ',' +
                  "country " + db->textColumnType() + ',' +
                  "photo_url " + db->textColumnType() + ',' +
                  "jamendo_url " + db->textColumnType() + ',' +
                  "home_url " + db->textColumnType() + ");";




    debug() << "Creating jamendo_artists: " << queryString;

    result = db->query( queryString );

    //create genre table
    queryString = "CREATE TABLE jamendo_genre ("
                  "id INTEGER PRIMARY KEY " + genreAutoIncrement + ',' +
                  "name " + db->textColumnType() + ',' +
                  "album_id INTEGER" + ");";

    debug() << "Creating jamendo_genres: " << queryString;

    result = db->query( queryString );

    //create a few indexes ( its all about the SPEEED baby! )



    queryString = "CREATE INDEX jamendo_album_album_id on jamendo_albums (id);";
    result = db->query( queryString );

    queryString = "CREATE INDEX jamendo_album_artist_id on jamendo_albums (artist_id);";
    result = db->query( queryString );

    queryString = "CREATE INDEX jamendo_artist_artist_id on jamendo_artists (id);";
    result = db->query( queryString );

    queryString = "CREATE INDEX jamendo_genre_album_id on jamendo_genre (album_id);";
    result = db->query( queryString );

    queryString = "CREATE INDEX jamendo_genre_name on jamendo_genre (name);";
    result = db->query( queryString );

}

void
JamendoDatabaseHandler::destroyDatabase( )
{

    debug() << "Destroy Jamendo database ";

    CollectionDB *db = CollectionDB::instance();
    QStringList result = db->query( "DROP TABLE jamendo_tracks;" );
    result = db->query( "DROP TABLE jamendo_albums;" );
    result = db->query( "DROP TABLE jamendo_artists;" );
    result = db->query( "DROP TABLE jamendo_genre;" );


    result = db->query( "DROP INDEX jamendo_album_album_id;");
    result = db->query( "DROP INDEX jamendo_album_artist_id;");
    result = db->query( "DROP INDEX jamendo_artist_artist_id;");
    result = db->query( "DROP INDEX jamendo_genre_album_id;");
    result = db->query( "DROP INDEX jamenod_genre_name;");

    if ( db->getDbConnectionType() == DbConnection::postgresql )
    {
        db->query( QString( "DROP SEQUENCE jamendo_track_seq;" ) );
        db->query( QString( "DROP SEQUENCE jamendo_album_seq;" ) );
        db->query( QString( "DROP SEQUENCE jamendo_artist_seq;" ) );
        //db->query( QString( "DROP SEQUENCE jamendo_tags_seq;" ) );
    }
}

int
JamendoDatabaseHandler::insertTrack( ServiceTrack *track )
{

    JamendoTrack * jTrack = static_cast<JamendoTrack *> ( track );
    QString numberString;

    CollectionDB *db = CollectionDB::instance();
    QString queryString = "INSERT INTO jamendo_tracks ( id, name, track_number, length, "
                          "album_id, artist_id, preview_url ) VALUES ( "
                          + QString::number( jTrack->id() ) + ", '"
                          + db->escapeString( jTrack->name() ) + "', "
                          + QString::number( jTrack->trackNumber() ) + ", "
                          + QString::number( jTrack->length() ) + ", "
                          + QString::number( jTrack->albumId() ) + ", "
                          + QString::number( jTrack->artistId() ) + ", '"
                          + db->escapeString( jTrack->url() ) + "' );";

    // debug() << "Adding Jamendo track " << queryString;
    int trackId = db->insert( queryString, NULL );

    // Process moods:

   /* QStringList moods = track->getMoods();

    foreach( QString mood, moods ) {
        queryString = "INSERT INTO jamendo_moods ( track_id, mood ) VALUES ( "
                      + QString::number( trackId ) + ", '"
                      + db->escapeString( mood ) +  "' );";


        //debug() << "Adding Jamendo mood: " << queryString;
        db->insert( queryString, NULL );
    }
*/
    return trackId;
}

int
JamendoDatabaseHandler::insertAlbum( ServiceAlbum *album )
{

    JamendoAlbum * jAlbum = static_cast<JamendoAlbum *> ( album );

    QString queryString;
    SqlStorage *sqlDb = CollectionManager::instance()->sqlStorage();
    queryString = "INSERT INTO jamendo_albums ( id, name, description, "
                  "popularity, cover_url, launch_year, genre, "
                  "artist_id ) VALUES ( "
                  + QString::number( jAlbum->id() ) + ", '"
                  + sqlDb->escape(  jAlbum->name() ) + "', '"
                  + sqlDb->escape( jAlbum->description() )+ "', "
                  + QString::number( jAlbum->popularity() ) + ", '"
                  + sqlDb->escape( jAlbum->coverUrl() )+ "', "
                  + QString::number( jAlbum->launchYear() ) + ", '"
                  + sqlDb->escape( jAlbum->genre() )+ "', "
                  + QString::number( jAlbum->artistId() ) + " );";

    //debug() << "Adding Jamendo album " << queryString;

    return sqlDb->insert( queryString, QString() );
}


int
JamendoDatabaseHandler::insertArtist( ServiceArtist *artist )
{
    JamendoArtist * jArtist = static_cast<JamendoArtist *> ( artist );
    QString queryString;
    SqlStorage *sqlDb = CollectionManager::instance()->sqlStorage();
    queryString = "INSERT INTO jamendo_artists ( id, name, description, "
                  "country, photo_url, jamendo_url, home_url "
                  ") VALUES ( "
                  + QString::number( jArtist->id() ) + ", '"
                  + sqlDb->escape( jArtist->name() ) + "', '"
                  + sqlDb->escape( jArtist->description() ) + "', '"
                  + sqlDb->escape( jArtist->country() ) + "', '"
                  + sqlDb->escape( jArtist->photoURL() ) + "', '"
                  + sqlDb->escape( jArtist->jamendoURL() ) + "', '"
                  + sqlDb->escape( jArtist->homeURL() ) + "' );";

    //debug() << "Adding Jamendo artist " << queryString;

    return sqlDb->insert( queryString, QString() );
/*
    QString m_country;
    QString m_photoURL;
    QString m_jamendoURL;
    QString m_homeURL;*/
}

int JamendoDatabaseHandler::insertGenre(ServiceGenre * genre)
{
    QString queryString;
    SqlStorage *sqlDb = CollectionManager::instance()->sqlStorage();
    queryString = "INSERT INTO jamendo_genre ( album_id, name "
                  ") VALUES ( "
                  + QString::number ( genre->albumId() ) + ", '"
                  + sqlDb->escape( genre->name() ) + "' );";

    //debug() << "Adding Jamendo genre " << queryString;

    return sqlDb->insert( queryString, 0 );
}



void
JamendoDatabaseHandler::begin( )
{
    CollectionManager *mgr = CollectionManager::instance();
    QString queryString = "BEGIN;";
    mgr->sqlStorage()->query( queryString );
}

void
JamendoDatabaseHandler::commit( )
{
    CollectionManager *mgr = CollectionManager::instance();
    QString queryString = "COMMIT;";
    mgr->sqlStorage()->query( queryString );
}










