/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#include "TestSqlQueryMaker.h"

#include "Debug.h"
#include "meta/TagLibUtils.h"

#include "DatabaseUpdater.h"
#include "SqlCollection.h"
#include "SqlQueryMaker.h"
#include "SqlRegistry.h"
#include "mysqlecollection/MySqlEmbeddedStorage.h"

#include "SqlMountPointManagerMock.h"

#include <QSignalSpy>

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestSqlQueryMaker )

//required for Debug.h
QMutex Debug::mutex;

//defined in TagLibUtils.h

namespace TagLib
{
    struct FileRef
    {
        //dummy
    };
}

void
Meta::Field::writeFields(const QString &filename, const QVariantMap &changes )
{
    return;
}

void
Meta::Field::writeFields(TagLib::FileRef fileref, const QVariantMap &changes)
{
    return;
}

TestSqlQueryMaker::TestSqlQueryMaker()
{
    qRegisterMetaType<Meta::DataPtr>();
    qRegisterMetaType<Meta::DataList>();
    qRegisterMetaType<Meta::TrackPtr>();
    qRegisterMetaType<Meta::TrackList>();
    qRegisterMetaType<Meta::AlbumPtr>();
    qRegisterMetaType<Meta::AlbumList>();
    qRegisterMetaType<Meta::ArtistPtr>();
    qRegisterMetaType<Meta::ArtistList>();
    qRegisterMetaType<Meta::GenrePtr>();
    qRegisterMetaType<Meta::GenreList>();
    qRegisterMetaType<Meta::ComposerPtr>();
    qRegisterMetaType<Meta::ComposerList>();
    qRegisterMetaType<Meta::YearPtr>();
    qRegisterMetaType<Meta::YearList>();
}

void
TestSqlQueryMaker::initTestCase()
{
    m_tmpDir = new KTempDir();
    m_storage = new MySqlEmbeddedStorage( m_tmpDir->name() );
    m_collection = new SqlCollection( "testId", "testcollection" );
    m_collection->setSqlStorage( m_storage );
    SqlMountPointManagerMock *mpm = new SqlMountPointManagerMock();
    m_collection->setMountPointManager( mpm );
    SqlRegistry *registry = new SqlRegistry( m_collection );
    registry->setStorage( m_storage );
    m_collection->setRegistry( registry );
    DatabaseUpdater updater;
    updater.setStorage( m_storage );
    updater.setCollection( m_collection );
    updater.update();

    //setup test data
    m_storage->query( "INSERT INTO artists(id, name) VALUES (1, 'artist1');" );
    m_storage->query( "INSERT INTO artists(id, name) VALUES (2, 'artist2');" );
    m_storage->query( "INSERT INTO artists(id, name) VALUES (3, 'artist3');" );

    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES(1,'album1',1);" );
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES(2,'album2',1);" );
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES(3,'album3',2);" );
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES(4,'album4',NULL);" );
    m_storage->query( "INSERT INTO albums(id,name,artist) VALUES(5,'album4',3);" );

    m_storage->query( "INSERT INTO composers(id, name) VALUES (1, 'composer1');" );
    m_storage->query( "INSERT INTO composers(id, name) VALUES (2, 'composer2');" );
    m_storage->query( "INSERT INTO composers(id, name) VALUES (3, 'composer3');" );

    m_storage->query( "INSERT INTO genres(id, name) VALUES (1, 'genre1');" );
    m_storage->query( "INSERT INTO genres(id, name) VALUES (2, 'genre2');" );
    m_storage->query( "INSERT INTO genres(id, name) VALUES (3, 'genre3');" );

    m_storage->query( "INSERT INTO years(id, name) VALUES (1, '1');" );
    m_storage->query( "INSERT INTO years(id, name) VALUES (2, '2');" );
    m_storage->query( "INSERT INTO years(id, name) VALUES (3, '3');" );

    m_storage->query( "INSERT INTO urls(id,deviceid,rpath,uniqueid) VALUES (1, -1, './IDoNotExist.mp3','1');" );
    m_storage->query( "INSERT INTO urls(id,deviceid,rpath,uniqueid) VALUES (2, -1, './IDoNotExistAsWell.mp3','2');" );
    m_storage->query( "INSERT INTO urls(id,deviceid,rpath,uniqueid) VALUES (3, -1, './MeNeither.mp3','3');" );
    m_storage->query( "INSERT INTO urls(id,deviceid,rpath,uniqueid) VALUES (4, -1, './NothingHere.mp3','4');" );
    m_storage->query( "INSERT INTO urls(id,deviceid,rpath,uniqueid) VALUES (5, -1, './GuessWhat.mp3','5');" );
    m_storage->query( "INSERT INTO urls(id,deviceid,rpath,uniqueid) VALUES (6, -1, './LookItsA.flac','6');" );

    m_storage->query( "INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(1,1,'track1','comment1',1,1,1,1,1);" );
    m_storage->query( "INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(2,2,'track2','comment2',1,2,1,1,1);" );
    m_storage->query( "INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(3,3,'track3','comment3',3,4,1,1,1);" );
    m_storage->query( "INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(4,4,'track4','comment4',2,3,3,3,3);" );
    m_storage->query( "INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(5,5,'track5','',3,5,2,2,2);" );
    m_storage->query( "INSERT INTO tracks(id,url,title,comment,artist,album,genre,year,composer) "
                      "VALUES(6,6,'track6','',1,4,2,2,2);" );

}

void
TestSqlQueryMaker::cleanupTestCase()
{
    delete m_collection;
    //m_storage is deleted by SqlCollection
    //m_registry is deleted by SqlCollection
    delete m_tmpDir;

}

void
TestSqlQueryMaker::testQueryAlbums()
{
    SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setAlbumQueryMode( QueryMaker::AllAlbums );
    qm.setQueryType( QueryMaker::Album );
    qm.run();
    QCOMPARE( qm.albums( "testId" ).count(), 5 );
}

void
TestSqlQueryMaker::testQueryArtists()
{
    SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Artist );
    qm.run();
    QCOMPARE( qm.artists( "testId" ).count(), 3 );
}

void
TestSqlQueryMaker::testQueryComposers()
{
    SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Composer );
    qm.run();
    QCOMPARE( qm.composers( "testId" ).count(), 3 );
}

void
TestSqlQueryMaker::testQueryGenres()
{
    SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Genre );
    qm.run();
    QCOMPARE( qm.genres( "testId" ).count(), 3 );
}

void
TestSqlQueryMaker::testQueryYears()
{
    SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Year );
    qm.run();
    QCOMPARE( qm.years( "testId" ).count(), 3 );
}

void
TestSqlQueryMaker::testQueryTracks()
{
    SqlQueryMaker qm( m_collection );
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Track );
    qm.run();
    QCOMPARE( qm.tracks( "testId" ).count(), 6 );
}

void
TestSqlQueryMaker::testAlbumQueryMode()
{
    SqlQueryMaker qm( m_collection );

    qm.setBlocking( true );
    qm.setAlbumQueryMode( QueryMaker::OnlyCompilations );
    qm.setQueryType( QueryMaker::Album );
    qm.run();
    QCOMPARE( qm.albums( "testId" ).count(), 1 );

    qm.reset();
    qm.setBlocking( true );
    qm.setAlbumQueryMode( QueryMaker::OnlyNormalAlbums );
    qm.setQueryType( QueryMaker::Album );
    qm.run();
    QCOMPARE( qm.albums( "testId" ).count(), 4 );

    qm.reset();
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Track );
    qm.setAlbumQueryMode( QueryMaker::OnlyCompilations );
    qm.run();
    QCOMPARE( qm.tracks( "testId" ).count(), 2 );

    qm.reset();
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Track );
    qm.setAlbumQueryMode( QueryMaker::OnlyNormalAlbums );
    qm.run();
    QCOMPARE( qm.tracks( "testId" ).count(), 4 );

    qm.reset();
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Artist );
    qm.setAlbumQueryMode( QueryMaker::OnlyCompilations );
    qm.run();
    QCOMPARE( qm.artists( "testId" ).count() , 2 );

    qm.reset();
    qm.setBlocking( true );
    qm.setQueryType( QueryMaker::Artist );
    qm.setAlbumQueryMode( QueryMaker::OnlyNormalAlbums );
    qm.run();
    QCOMPARE( qm.artists( "testId" ).count(), 3 );

    qm.reset();
    qm.setBlocking( true );
    qm.setAlbumQueryMode( QueryMaker::OnlyCompilations );
    qm.setQueryType( QueryMaker::Genre );
    qm.run();
    QCOMPARE( qm.genres( "testId" ).count(), 2 );

    qm.reset();
    qm.setBlocking( true );
    qm.setAlbumQueryMode( QueryMaker::OnlyNormalAlbums );
    qm.setQueryType( QueryMaker::Genre );
    qm.run();
    QCOMPARE( qm.genres( "testId" ).count(), 3 );

}

void
TestSqlQueryMaker::testDeleteQueryMakerWithRunningQuery()
{    
    int iteration = 0;
    bool queryNotDoneYet = true;

    //wait one second per query in total, that should be enough for it to complete
    do
    {
        SqlQueryMaker *qm = new SqlQueryMaker( m_collection );
        QSignalSpy spy( qm, SIGNAL(queryDone()) );
        qm->setQueryType( QueryMaker::Track );
        qm->addFilter( Meta::valTitle, QString::number( iteration), false, false );
        qm->run();
        //wait 10 msec more per iteration, might have to be tweaked
        if( iteration > 0 )
        {
            QTest::qWait( 10 * iteration );
        }
        delete qm;
        queryNotDoneYet = ( spy.count() == 0 );
        if( iteration > 50 )
        {
            break;
        }
        QTest::qWait( 1000 - 10 * iteration );
        iteration++;
    } while ( queryNotDoneYet );
    qDebug() << "Iterations: " << iteration;
}

void
TestSqlQueryMaker::testAsyncAlbumQuery()
{
    QueryMaker *qm = new SqlQueryMaker( m_collection );
    qm->setQueryType( QueryMaker::Album );
    QSignalSpy doneSpy1( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy1( qm, SIGNAL(newResultReady(QString,Meta::AlbumList)));

    qm->run();

    QTest::qWait( 1000 );
    QCOMPARE( resultSpy1.count(), 1 );
    QList<QVariant> args1 = resultSpy1.takeFirst();
    QVERIFY( args1.value(1).canConvert<Meta::AlbumList>() );
    QCOMPARE( args1.value(1).value<Meta::AlbumList>().count(), 5 );
    QCOMPARE( doneSpy1.count(), 1);
    delete qm;

    qm = new SqlQueryMaker( m_collection );
    qm->setQueryType( QueryMaker::Album );
    QSignalSpy doneSpy2( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy2( qm, SIGNAL(newResultReady(QString,Meta::AlbumList)));
    qm->addFilter( Meta::valAlbum, "foo" ); //should result in no match

    qm->run();

    QTest::qWait( 1000 );
    QCOMPARE( resultSpy2.count(), 1 );
    QList<QVariant> args2 = resultSpy2.takeFirst();
    QVERIFY( args2.value(1).canConvert<Meta::AlbumList>() );
    QCOMPARE( args2.value(1).value<Meta::AlbumList>().count(), 0 );
    QCOMPARE( doneSpy2.count(), 1);
}

void
TestSqlQueryMaker::testAsyncArtistQuery()
{
    QueryMaker *qm = new SqlQueryMaker( m_collection );
    qm->setQueryType( QueryMaker::Artist );
    QSignalSpy doneSpy1( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy1( qm, SIGNAL(newResultReady(QString,Meta::ArtistList)));

    qm->run();

    QTest::qWait( 1000 );
    QCOMPARE( resultSpy1.count(), 1 );
    QList<QVariant> args1 = resultSpy1.takeFirst();
    QVERIFY( args1.value(1).canConvert<Meta::ArtistList>() );
    QCOMPARE( args1.value(1).value<Meta::ArtistList>().count(), 3 );
    QCOMPARE( doneSpy1.count(), 1);
    delete qm;

    qm = new SqlQueryMaker( m_collection );
    qm->setQueryType( QueryMaker::Artist );
    QSignalSpy doneSpy2( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy2( qm, SIGNAL(newResultReady(QString,Meta::ArtistList)));
    qm->addFilter( Meta::valArtist, "foo" ); //should result in no match

    qm->run();

    QTest::qWait( 1000 );
    QCOMPARE( resultSpy2.count(), 1 );
    QList<QVariant> args2 = resultSpy2.takeFirst();
    QVERIFY( args2.value(1).canConvert<Meta::ArtistList>() );
    QCOMPARE( args2.value(1).value<Meta::ArtistList>().count(), 0 );
    QCOMPARE( doneSpy2.count(), 1);
}

void
TestSqlQueryMaker::testAsyncComposerQuery()
{
    QueryMaker *qm = new SqlQueryMaker( m_collection );
    qm->setQueryType( QueryMaker::Composer );
    QSignalSpy doneSpy1( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy1( qm, SIGNAL(newResultReady(QString,Meta::ComposerList)));

    qm->run();

    QTest::qWait( 1000 );
    QCOMPARE( resultSpy1.count(), 1 );
    QList<QVariant> args1 = resultSpy1.takeFirst();
    QVERIFY( args1.value(1).canConvert<Meta::ComposerList>() );
    QCOMPARE( args1.value(1).value<Meta::ComposerList>().count(), 3 );
    QCOMPARE( doneSpy1.count(), 1);

    delete qm;

    qm = new SqlQueryMaker( m_collection );
    qm->setQueryType( QueryMaker::Composer );
    QSignalSpy doneSpy2( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy2( qm, SIGNAL(newResultReady(QString,Meta::ComposerList)));
    qm->addFilter( Meta::valComposer, "foo" ); //should result in no match

    qm->run();

    QTest::qWait( 1000 );
    QCOMPARE( resultSpy2.count(), 1 );
    QList<QVariant> args2 = resultSpy2.takeFirst();
    QVERIFY( args2.value(1).canConvert<Meta::ComposerList>() );
    QCOMPARE( args2.value(1).value<Meta::ComposerList>().count(), 0 );
    QCOMPARE( doneSpy2.count(), 1);
}

void
TestSqlQueryMaker::testAsyncTrackQuery()
{
    QueryMaker *qm = new SqlQueryMaker( m_collection );
    qm->setQueryType( QueryMaker::Track );
    QSignalSpy doneSpy1( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy1( qm, SIGNAL(newResultReady(QString,Meta::TrackList)));

    qm->run();

    QTest::qWait( 1000 );
    QCOMPARE( resultSpy1.count(), 1 );
    QList<QVariant> args1 = resultSpy1.takeFirst();
    QVERIFY( args1.value(1).canConvert<Meta::TrackList>() );
    QCOMPARE( args1.value(1).value<Meta::TrackList>().count(), 6 );
    QCOMPARE( doneSpy1.count(), 1);

    delete qm;

    qm = new SqlQueryMaker( m_collection );
    qm->setQueryType( QueryMaker::Track );
    QSignalSpy doneSpy2( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy2( qm, SIGNAL(newResultReady(QString,Meta::TrackList)));
    qm->addFilter( Meta::valTitle, "foo" ); //should result in no match

    qm->run();

    QTest::qWait( 1000 );
    QCOMPARE( resultSpy2.count(), 1 );
    QList<QVariant> args2 = resultSpy2.takeFirst();
    QVERIFY( args2.value(1).canConvert<Meta::TrackList>() );
    QCOMPARE( args2.value(1).value<Meta::TrackList>().count(), 0 );
    QCOMPARE( doneSpy2.count(), 1);
}

void
TestSqlQueryMaker::testAsyncGenreQuery()
{
    QueryMaker *qm = new SqlQueryMaker( m_collection );
    qm->setQueryType( QueryMaker::Genre );
    QSignalSpy doneSpy1( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy1( qm, SIGNAL(newResultReady(QString,Meta::GenreList)));

    qm->run();

    QTest::qWait( 1000 );
    QCOMPARE( resultSpy1.count(), 1 );
    QList<QVariant> args1 = resultSpy1.takeFirst();
    QVERIFY( args1.value(1).canConvert<Meta::GenreList>() );
    QCOMPARE( args1.value(1).value<Meta::GenreList>().count(), 3 );
    QCOMPARE( doneSpy1.count(), 1);

    delete qm;

    qm = new SqlQueryMaker( m_collection );
    qm->setQueryType( QueryMaker::Genre );
    QSignalSpy doneSpy2( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy2( qm, SIGNAL(newResultReady(QString,Meta::GenreList)));
    qm->addFilter( Meta::valGenre, "foo" ); //should result in no match

    qm->run();

    QTest::qWait( 1000 );
    QCOMPARE( resultSpy2.count(), 1 );
    QList<QVariant> args2 = resultSpy2.takeFirst();
    QVERIFY( args2.value(1).canConvert<Meta::GenreList>() );
    QCOMPARE( args2.value(1).value<Meta::GenreList>().count(), 0 );
    QCOMPARE( doneSpy2.count(), 1);
}

void
TestSqlQueryMaker::testAsyncYearQuery()
{
    QueryMaker *qm = new SqlQueryMaker( m_collection );
    qm->setQueryType( QueryMaker::Year );
    QSignalSpy doneSpy1( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy1( qm, SIGNAL(newResultReady(QString,Meta::YearList)));

    qm->run();

    QTest::qWait( 1000 );
    QCOMPARE( resultSpy1.count(), 1 );
    QList<QVariant> args1 = resultSpy1.takeFirst();
    QVERIFY( args1.value(1).canConvert<Meta::YearList>() );
    QCOMPARE( args1.value(1).value<Meta::YearList>().count(), 3 );
    QCOMPARE( doneSpy1.count(), 1);

    delete qm;

    qm = new SqlQueryMaker( m_collection );
    qm->setQueryType( QueryMaker::Year );
    QSignalSpy doneSpy2( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy2( qm, SIGNAL(newResultReady(QString,Meta::YearList)));
    qm->addFilter( Meta::valYear, "foo" ); //should result in no match

    qm->run();

    QTest::qWait( 1000 );
    QCOMPARE( resultSpy2.count(), 1 );
    QList<QVariant> args2 = resultSpy2.takeFirst();
    QVERIFY( args2.value(1).canConvert<Meta::YearList>() );
    QCOMPARE( args2.value(1).value<Meta::YearList>().count(), 0 );
    QCOMPARE( doneSpy2.count(), 1);
}

void
TestSqlQueryMaker::testAsyncTrackDataQuery()
{
    QueryMaker *qm = new SqlQueryMaker( m_collection );
    qm->setQueryType( QueryMaker::Track );
    qm->setReturnResultAsDataPtrs( true );
    QSignalSpy doneSpy1( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy1( qm, SIGNAL(newResultReady(QString,Meta::DataList)));

    qm->run();

    QTest::qWait( 1000 );
    QCOMPARE( resultSpy1.count(), 1 );
    QList<QVariant> args1 = resultSpy1.takeFirst();
    QVERIFY( args1.value(1).canConvert<Meta::DataList>() );
    QCOMPARE( args1.value(1).value<Meta::DataList>().count(), 6 );
    QCOMPARE( doneSpy1.count(), 1);

    delete qm;

    qm = new SqlQueryMaker( m_collection );
    qm->setQueryType( QueryMaker::Track );
    qm->setReturnResultAsDataPtrs( true );
    QSignalSpy doneSpy2( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy2( qm, SIGNAL(newResultReady(QString,Meta::DataList)));
    qm->addFilter( Meta::valTitle, "foo" ); //should result in no match

    qm->run();

    QTest::qWait( 1000 );
    QCOMPARE( resultSpy2.count(), 1 );
    QList<QVariant> args2 = resultSpy2.takeFirst();
    QVERIFY( args2.value(1).canConvert<Meta::DataList>() );
    QCOMPARE( args2.value(1).value<Meta::DataList>().count(), 0 );
    QCOMPARE( doneSpy2.count(), 1);
}

void
TestSqlQueryMaker::testAsyncCustomQuery()
{
    QueryMaker *qm = new SqlQueryMaker( m_collection );
    qm->setQueryType( QueryMaker::Custom );
    qm->addReturnFunction( QueryMaker::Count, Meta::valTitle );
    QSignalSpy doneSpy1( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy1( qm, SIGNAL(newResultReady(QString,QStringList)));

    qm->run();

    QTest::qWait( 1000 );
    QCOMPARE( resultSpy1.count(), 1 );
    QList<QVariant> args1 = resultSpy1.takeFirst();
    QVERIFY( args1.value(1).canConvert<QStringList>() );
    QCOMPARE( args1.value(1).value<QStringList>().count(), 1 );
    QCOMPARE( args1.value(1).value<QStringList>().first(), QString( "6" ) );
    QCOMPARE( doneSpy1.count(), 1);

    delete qm;

    qm = new SqlQueryMaker( m_collection );
    qm->setQueryType( QueryMaker::Custom );
    qm->addReturnFunction( QueryMaker::Count, Meta::valTitle );
    QSignalSpy doneSpy2( qm, SIGNAL(queryDone()));
    QSignalSpy resultSpy2( qm, SIGNAL(newResultReady(QString,QStringList)));
    qm->addFilter( Meta::valTitle, "foo" ); //should result in no match

    qm->run();

    QTest::qWait( 1000 );
    QCOMPARE( resultSpy2.count(), 1 );
    QList<QVariant> args2 = resultSpy2.takeFirst();
    QVERIFY( args2.value(1).canConvert<QStringList>() );
    QCOMPARE( args2.value(1).value<QStringList>().count(), 1 );
    QCOMPARE( args2.value(1).value<QStringList>().first(), QString( "0" ) );
    QCOMPARE( doneSpy2.count(), 1);
}

#include "TestSqlQueryMaker.moc"
