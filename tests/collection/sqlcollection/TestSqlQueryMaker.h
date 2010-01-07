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

#ifndef TESTSQLQUERYMAKER_H
#define TESTSQLQUERYMAKER_H

#include <QtTest/QTest>

#include <KTempDir>

class SqlStorage;
class SqlCollection;

class TestSqlQueryMaker : public QObject
{
    Q_OBJECT
public:
    TestSqlQueryMaker();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testQueryTracks();
    void testQueryAlbums();
    void testQueryGenres();
    void testQueryYears();
    void testQueryComposers();
    void testQueryArtists();
    void testAlbumQueryMode();

    void testDeleteQueryMakerWithRunningQuery();

private:
    SqlCollection *m_collection;
    SqlStorage *m_storage;
    KTempDir *m_tmpDir;
};

QTEST_MAIN( TestSqlQueryMaker )

#endif // TESTSQLQUERYMAKER_H
