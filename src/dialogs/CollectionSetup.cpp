/***************************************************************************
   begin                : Tue Feb 4 2003
   copyright            : (C) 2003 Scott Wheeler <wheeler@kde.org>
                        : (C) 2004 Max Howell <max.howell@methylblue.com>
                        : (C) 2004 Mark Kretschmann <markey@web.de>
                        : (C) 2008 Seb Ruiz <ruiz@kde.org>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "CollectionSetup.h"

#include "amarokconfig.h"
#include "mountpointmanager.h"

#include <KFileItem>
#include <KLocale>
#include <KVBox>

#include <QDir>
#include <QFile>
#include <QLabel>
#include <QToolTip>

#include "Debug.h"

CollectionSetup* CollectionSetup::s_instance;


CollectionSetup::CollectionSetup( QWidget *parent )
        : KVBox( parent )
{
    DEBUG_BLOCK

    setObjectName( "CollectionSetup" );
    s_instance = this;

    (new QLabel( i18n(
        "These folders will be scanned for "
        "media to make up your collection:"), this ))->setAlignment( Qt::AlignJustify );

    m_view  = new CollectionView( this );

    m_recursive = new QCheckBox( i18n("&Scan folders recursively"), this );
    m_monitor   = new QCheckBox( i18n("&Watch folders for changes"), this );

    m_recursive->setToolTip( i18n( "If selected, Amarok will read all subfolders." ) );
    m_monitor->setToolTip(   i18n( "If selected, folders will automatically get rescanned when the content is modified, e.g. when a new file was added." ) );

    m_recursive->setChecked( AmarokConfig::scanRecursively() );
    m_monitor->setChecked( AmarokConfig::monitorChanges() );

    m_view->setHeaderHidden( true );
    m_view->setRootIsDecorated( true );
    m_view->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

    // set the model _after_ constructing the checkboxes
    m_model = new CollectionFolder::Model();
    m_view->setModel( m_model );
    m_view->setRootIndex( m_model->setRootPath( QDir::rootPath() ) );
    
    // Read config values
    //we have to detect if this is the actual first run and not get the collectionFolders in that case
    //there won't be any anyway and accessing them creates a Sqlite database, even if the user wants to
    //use another database
    //bug 131719 131724
    //if( !Amarok::config().readEntry( "First Run", true ) )
    QStringList dirs = MountPointManager::instance()->collectionFolders();
    m_model->setDirectories( dirs );

    setSpacing( 6 );
}


void
CollectionSetup::writeConfig()
{
    //If we are in recursive mode then we don't need to store the names of the
    //subdirectories of the selected directories
    if ( recursive() )
    {
        for ( QStringList::ConstIterator it=m_dirs.constBegin(), end = m_dirs.constEnd(); it!=end; ++it )
        {
            QStringList::Iterator jt=m_dirs.begin();
            QStringList::ConstIterator dirsEnd = m_dirs.constEnd();
            while ( jt!=dirsEnd )
            {
                if ( it==jt )
                {
                    ++jt;
                    continue;
                }
                //Note: all directories except "/" lack a trailing '/'.
                //If (*jt) is a subdirectory of (*it) it is redundant.
                //As all directories are subdirectories of "/", if "/" is selected, we
                //can delete everything else.
                if ( ( *jt ).startsWith( *it + '/' ) || *it=="/" )
                    jt = m_dirs.erase( jt );
                else
                    ++jt;
            }
        }
    }

    MountPointManager::instance()->setCollectionFolders( m_dirs );
    AmarokConfig::setScanRecursively( recursive() );
    AmarokConfig::setMonitorChanges( monitor() );
}


//////////////////////////////////////////////////////////////////////////////////////////
// CLASS Model
//////////////////////////////////////////////////////////////////////////////////////////

namespace CollectionFolder {

    Model::Model()
        : QFileSystemModel()
    {
        setFilter( QDir::AllDirs | QDir::NoDotAndDotDot );
    }

    Qt::ItemFlags
    Model::flags( const QModelIndex &index ) const
    {
        Qt::ItemFlags flags = QFileSystemModel::flags( index );
        const QString path = filePath( index );
        if( ( recursive() && ancestorChecked( path ) ) || isForbiddenPath( path ) )
            flags ^= Qt::ItemIsEnabled; //disabled!
       
        flags |= Qt::ItemIsUserCheckable;

        return flags;
    }

    QVariant
    Model::data( const QModelIndex& index, int role ) const
    {
        if( index.isValid() && index.column() == 0 && role == Qt::CheckStateRole )
        {
            const QString path = filePath( index );
            if( recursive() && ancestorChecked( path ) )
                return Qt::Checked; // always set children of recursively checked parents to checked
            if( isForbiddenPath( path ) )
                return Qt::Unchecked; // forbidden paths can never be checked
            return m_checked.contains( path ) ? Qt::Checked : Qt::Unchecked;
        }
        return QFileSystemModel::data( index, role );
    }

    bool
    Model::setData( const QModelIndex& index, const QVariant& value, int role )
    {
        if( index.isValid() && index.column() == 0 && role == Qt::CheckStateRole )
        {
            QString path = filePath( index );
            // store checked paths, remove unchecked paths
            if( value.toInt() == Qt::Checked )
                m_checked.insert( path );
            else
                m_checked.remove( path );
            return true;
        }
        return QFileSystemModel::setData( index, value, role );
    }

    void
    Model::setDirectories( QStringList &dirs )
    {
        m_checked.clear();
        foreach( QString dir, dirs )
        {
            m_checked.insert( dir );
        }
    }

    inline bool
    Model::isForbiddenPath( const QString &path ) const
    {
        // we need the trailing slash otherwise we could forbid "/dev-music" for example
        QString _path = path.endsWith( "/" ) ? path : path + "/";
        return _path.startsWith( "/proc/" ) || _path.startsWith( "/dev/" ) || _path.startsWith( "/sys/" );
    }

    bool
    Model::ancestorChecked( const QString &path ) const
    {
        foreach( QString element, m_checked )
        {
            if( path.startsWith( element ) && element != path )
                return true;
        }
        return false;
    }

} //namespace Collection

