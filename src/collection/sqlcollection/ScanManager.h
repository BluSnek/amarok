/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef AMAROK_SQL_SCANMANAGER_H
#define AMAROK_SQL_SCANMANAGER_H

#include <QMutex>
#include <QObject>
#include <QWaitCondition>
#include <QXmlStreamReader>

#include <KProcess>
#include <threadweaver/Job.h>

class SqlCollection;
class XmlParseJob;

class ScanManager : public QObject
{
    Q_OBJECT
    public:
        ScanManager( SqlCollection *parent );

        void startFullScan();
        void startIncrementalScan();

    private slots:
        void slotReadReady();
        void slotFinished( int exitCode, QProcess::ExitStatus exitStatus );

    private:
        QStringList getDirsToScan() const;

    private:
        SqlCollection *m_collection;

        KProcess *m_scanner;
        XmlParseJob *m_parser;

        int m_restartCount;
};

class XmlParseJob : public ThreadWeaver::Job
{
    Q_OBJECT
    public:
        XmlParseJob( ScanManager *parent );
        ~XmlParseJob();

        void run();

        void addNewXmlData( const QString &data );

    private:
        QXmlStreamReader m_reader;
        QString m_nextData;
        QWaitCondition m_wait;
        QMutex m_mutex;
};

#endif
