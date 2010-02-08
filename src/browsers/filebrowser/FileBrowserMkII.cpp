/****************************************************************************************
 * Copyright (c) 2010 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2010 Casey Link <unnamedrambler@gmail.com>                              *
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

#include "FileBrowserMkII.h"

#include "BrowserBreadcrumbItem.h"
#include "BrowserCategoryList.h"
#include "Debug.h"
#include "EngineController.h"
#include "FileView.h"
#include "MimeTypeFilterProxyModel.h"
#include "playlist/PlaylistModelStack.h"


#include <KLineEdit>
#include <KDirModel>
#include <KDirLister>
#include <QDir>

FileBrowserMkII::FileBrowserMkII( const char * name, QWidget *parent )
    : BrowserCategory( name, parent )
    , m_directoryLoader( 0 )
{

    DEBUG_BLOCK;
    m_searchWidget = new SearchWidget( this, this, false );
    m_searchWidget->setClickMessage( i18n( "Filter Files" ) );

    m_filterTimer.setSingleShot( true );
    connect( &m_filterTimer, SIGNAL( timeout() ), this, SLOT( slotFilterNow() ) );

    m_kdirModel = new KDirModel( this );
    m_kdirModel->dirLister()->openUrl( KUrl( QDir::homePath() ) );

    m_mimeFilterProxyModel = new MimeTypeFilterProxyModel( EngineController::supportedMimeTypes(), this );
    m_mimeFilterProxyModel->setSourceModel( m_kdirModel );
    m_mimeFilterProxyModel->setSortCaseSensitivity( Qt::CaseInsensitive );
    m_mimeFilterProxyModel->sort( 0 );

    debug() << "home path: " <<  QDir::homePath();

    m_fileView = new FileView( this );
    m_fileView->setModel( m_mimeFilterProxyModel );

    m_fileView->setDragEnabled( true );
    m_fileView->setSelectionMode( QAbstractItemView::ExtendedSelection );

    readConfig();

    connect( m_fileView, SIGNAL( activated( const QModelIndex & ) ), this, SLOT( itemActivated( const QModelIndex & ) ) );
}

FileBrowserMkII::~FileBrowserMkII()
{
    writeConfig();
    connect( m_fileView, SIGNAL( doubleClicked( const QModelIndex & ) ), this, SLOT( itemActivated( const QModelIndex & ) ) );
}

void FileBrowserMkII::polish()
{
    DEBUG_BLOCK
    setupAddItems();
}

void FileBrowserMkII::itemActivated( const QModelIndex &index )
{
    DEBUG_BLOCK
    KFileItem file = index.data( KDirModel::FileItemRole ).value<KFileItem>();
    KUrl filePath = file.url();
    m_currentPath = filePath.path();

    debug() << "activated url: " << filePath.url();
    debug() << "filename: " << filePath.fileName();

    if( file.isDir() ) {
        debug() << "setting root path to: " << filePath.path();
        m_kdirModel->dirLister()->openUrl( filePath );
        m_fileView->setRootIndex( index );

        //get list of current sibling directories for breadcrumb:

        QStringList siblings = siblingsForDir( m_currentPath );
        
        debug() << "setting root path to: " << filePath.path();
        m_kdirModel->dirLister()->openUrl( filePath );

        //add this dir to the breadcrumb
        setupAddItems();
        activate();
      
    }
    else
    {
        if( EngineController::canDecode( filePath ) )
        {
            QList<KUrl> urls;
            urls << filePath;
            The::playlistController()->insertOptioned( urls, Playlist::AppendAndPlay );
        }
    }
}

void FileBrowserMkII::slotSetFilterTimeout()
{
    KLineEdit *lineEdit = dynamic_cast<KLineEdit*>( sender() );
    if( lineEdit )
    {
        m_currentFilter = lineEdit->text();
        m_filterTimer.stop();
        m_filterTimer.start( 500 );
    }
}

void FileBrowserMkII::slotFilterNow()
{
    m_mimeFilterProxyModel->setFilterFixedString( m_currentFilter );

    QStringList filters;
    filters << m_currentFilter;
}

void FileBrowserMkII::readConfig()
{
    DEBUG_BLOCK

    KConfigGroup config = Amarok::config( "File Browser" );

    m_kdirModel->dirLister()->openUrl( KUrl( config.readEntry( "Current Directory" ) ) );

    m_currentPath = KUrl( config.readEntry( "Current Directory" ) ).path();
}

void FileBrowserMkII::writeConfig()
{
    DEBUG_BLOCK
    KConfigGroup config = Amarok::config( "File Browser" );
    config.writeEntry( "Current Directory", m_kdirModel->dirLister()->url().toLocalFile() );
    config.sync();
}


void FileBrowserMkII::addItemActivated( const QString &callbackString )
{
    DEBUG_BLOCK
    
    debug() << "callback: " << callbackString;
    
    m_kdirModel->dirLister()->openUrl( KUrl( callbackString ) );
    m_currentPath = callbackString;
    setupAddItems();
    activate();
}

void FileBrowserMkII::setupAddItems()
{
    DEBUG_BLOCK
    clearAdditionalItems();
    
    QStringList parts = m_currentPath.split( QDir::separator() );
    QString partialPath;
    debug() << "current path" << m_currentPath;


    /*
     * A URL like /home/user/Music/Prince is shown as [Home] > [Music] > [Prince]
     */
    if( m_currentPath.startsWith( QDir::homePath() ) )
    {
        int idx = m_currentPath.indexOf( QDir::homePath() ) + QDir::homePath().size();
        // everything after the homedir e.g., Music/Prince
        QString everything_else = m_currentPath.mid( idx );
        debug() << "everything else" << everything_else;
        // replace parts with everything else
        parts = everything_else.split( QDir::separator() ) ;
        debug() << "parts" << parts;
        partialPath = QDir::homePath();

        // Add the [Home]
        QStringList siblings = siblingsForDir( QDir::homePath() );
        addAdditionalItem( new BrowserBreadcrumbItem( i18n( "Home" ), siblings, QDir::homePath(), this ) );
    }

    foreach( QString part, parts )
    {
        if( !part.isEmpty() )
        {
            partialPath += '/' + part;
            QStringList siblings = siblingsForDir( partialPath );
            addAdditionalItem( new BrowserBreadcrumbItem( part, siblings, partialPath, this ) );
        }
    }
    
}

QStringList FileBrowserMkII::siblingsForDir( const QString &path )
{
    // includes the dir itself
    DEBUG_BLOCK
    debug() << "path: " << path;
    QStringList siblings;

    QDir dir( path );
    QString currentName = dir.dirName();
    if( !dir.isRoot() )
    {
        dir.cdUp();
        foreach( QString childDir, dir.entryList( QDir::Dirs | QDir::NoDotAndDotDot ) )
        {
                siblings << childDir;
        }
    }
    return siblings;
}

void FileBrowserMkII::reActivate()
{
    DEBUG_BLOCK
    
    //go to root:
    m_kdirModel->dirLister()->openUrl( KUrl( QDir::rootPath() ) );
    m_currentPath = QDir::rootPath();
    setupAddItems();
    activate();
}

QString FileBrowserMkII::prettyName() const
{
    DEBUG_BLOCK
    if( parentList()->activeCategory() == this )
        return QDir::rootPath();
    else
        return BrowserCategory::prettyName();
}
