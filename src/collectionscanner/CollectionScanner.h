/***************************************************************************
 *   Copyright (C) 2003-2008 Mark Kretschmann <kretschmann@kde.org>        *
 *             (C) 2008 Dan Meltzer <hydrogen@notyetimplemented.com        *
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

#ifndef COLLECTIONSCANNER_H
#define COLLECTIONSCANNER_H

#include "metadata/tfile_helper.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <QHash>
#include <QStringList>

#include <KApplication>
#include <KStandardDirs>

//Taglib includes..
#include <audioproperties.h>

typedef QHash<QString, QString> AttributeHash;

/**
 * @class CollectionScanner
 * @short Scans directories and builds the Collection
 */

class CollectionScanner : public KApplication
{
    Q_OBJECT

public:
    CollectionScanner( const QStringList& folders,
                       bool recursive,
                       bool incremental,
                       bool importPlaylists,
                       bool restart );

    ~CollectionScanner();
    int newInstance() { return 0; }

private slots:
    void doJob();

private:

    enum FileType {
        mp3,
        ogg,
        flac,
        mp4
    };

    inline QString saveLocation() const { return KGlobal::dirs()->saveLocation( "data", QString("amarok/"), true ); }

    void readDir( const QString& dir, QStringList& entries );
    void scanFiles( const QStringList& entries );

    /**
     * Read metadata tags of a given file.
     * @track Track for the file.
     * @return QMap containing tags, or empty QMap on failure.
     */
    AttributeHash readTags( const QString &path, TagLib::AudioProperties::ReadStyle readStyle = TagLib::AudioProperties::Fast );

    /**
     * Helper method for writing XML elements to stdout.
     * @name Name of the element.
     * @attributes Key/value map of attributes.
     */
    void writeElement( const QString& name, const AttributeHash& attributes );

    /**
     * @return the LOWERCASE file extension without the preceding '.', r "" if there is none
     */
    inline QString extension( const QString &fileName )
    {
        return fileName.contains( '.' ) ? fileName.mid( fileName.lastIndexOf( '.' ) + 1 ).toLower() : "";
    }

    /**
     * @return the last directory in @param fileName
     */
    inline QString directory( const QString &fileName )
    {
        return fileName.section( '/', 0, -2 );
    }

    const bool    m_importPlaylists;
    QStringList   m_folders;
    const bool    m_recursively;
    const bool    m_incremental;
    const bool    m_restart;
    const QString m_logfile;
    QStringList   m_scannedFolders;
};


#endif // COLLECTIONSCANNER_H

