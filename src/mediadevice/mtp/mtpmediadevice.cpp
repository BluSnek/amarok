/***************************************************************************
 * copyright            : (C) 2006 Andy Kelk <andy@mopoke.co.uk>           *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

 /**
  *  Based on njb mediadevice with some code hints from the libmtp
  *  example tools
  */

 /**
  *  MTP media device
  *  @author Andy Kelk <andy@mopoke.co.uk>
  *  @see http://libmtp.sourceforge.net/
  */

#define DEBUG_PREFIX "MtpMediaDevice"

#include "config-amarok.h"
#include "mtpmediadevice.h"
//Added by qt3to4:
#include <QLabel>

AMAROK_EXPORT_PLUGIN( MtpMediaDevice )

// Amarok
#include <debug.h>
#include <statusbar/ContextStatusBar.h>
#include <statusbar/popupMessage.h>

// KDE
#include <kapplication.h>
#include <kiconloader.h>
#include <k3popupmenu.h>
#include <kmessagebox.h>
#include <ktempdir.h>

// Qt
#include <QDir>
#include <q3listview.h>
#include <QToolTip>
#include <QLineEdit>
#include <QRegExp>
#include <QBuffer>
#include <qmap.h>

/**
 * MtpMediaDevice Class
 */

MtpMediaDevice::MtpMediaDevice() : MediaDevice()
{
    m_name = i18n("MTP Media Device");
    m_device = 0;
    m_folders = 0;
    m_playlistItem = 0;
    setDisconnected();
    m_hasMountPoint = false;
    m_syncStats = false;
    m_transcode = false;
    m_transcodeAlways = false;
    m_transcodeRemove = false;
    m_configure = false;
    m_customButton = true;
    m_transfer = true;

    mtpFileTypes[LIBMTP_FILETYPE_WAV] = "wav";
    mtpFileTypes[LIBMTP_FILETYPE_MP3] = "mp3";
    mtpFileTypes[LIBMTP_FILETYPE_WMA] = "wma";
    mtpFileTypes[LIBMTP_FILETYPE_OGG] = "ogg";
    mtpFileTypes[LIBMTP_FILETYPE_AUDIBLE] = "aa"; // audible
    mtpFileTypes[LIBMTP_FILETYPE_MP4] = "mp4";
    mtpFileTypes[LIBMTP_FILETYPE_UNDEF_AUDIO] = "undef-audio";
    mtpFileTypes[LIBMTP_FILETYPE_WMV] = "wmv";
    mtpFileTypes[LIBMTP_FILETYPE_AVI] = "avi";
    mtpFileTypes[LIBMTP_FILETYPE_MPEG] = "mpg";
    mtpFileTypes[LIBMTP_FILETYPE_ASF] = "asf";
    mtpFileTypes[LIBMTP_FILETYPE_QT] = "mov";
    mtpFileTypes[LIBMTP_FILETYPE_UNDEF_VIDEO] = "undef-video";
    mtpFileTypes[LIBMTP_FILETYPE_JPEG] = "jpg";
    mtpFileTypes[LIBMTP_FILETYPE_JFIF] = "jpg";
    mtpFileTypes[LIBMTP_FILETYPE_TIFF] = "tiff";
    mtpFileTypes[LIBMTP_FILETYPE_BMP] = "bmp";
    mtpFileTypes[LIBMTP_FILETYPE_GIF] = "gif";
    mtpFileTypes[LIBMTP_FILETYPE_PICT] = "pict";
    mtpFileTypes[LIBMTP_FILETYPE_PNG] = "png";
    mtpFileTypes[LIBMTP_FILETYPE_VCALENDAR1] = "vcs"; // vcal1
    mtpFileTypes[LIBMTP_FILETYPE_VCALENDAR2] = "vcs"; // vcal2
    mtpFileTypes[LIBMTP_FILETYPE_VCARD2] = "vcf"; // vcard2
    mtpFileTypes[LIBMTP_FILETYPE_VCARD3] = "vcf"; // vcard3
    mtpFileTypes[LIBMTP_FILETYPE_WINDOWSIMAGEFORMAT] = "wim"; // windows image format
    mtpFileTypes[LIBMTP_FILETYPE_WINEXEC] = "exe";
    mtpFileTypes[LIBMTP_FILETYPE_TEXT] = "txt";
    mtpFileTypes[LIBMTP_FILETYPE_HTML] = "html";
    mtpFileTypes[LIBMTP_FILETYPE_UNKNOWN] = "unknown";

    m_newTracks = new QList<MediaItem*>;
}

void
MtpMediaDevice::init( MediaBrowser *parent )
{
    MediaDevice::init( parent );
}

bool
MtpMediaDevice::isConnected()
{
    return !( m_device == 0 );
}

/**
 * File types that we support
 */
QStringList
MtpMediaDevice::supportedFiletypes()
{
    return m_supportedFiles;
}


int
MtpMediaDevice::progressCallback( uint64_t const sent, uint64_t const total, void const * const data )
{
    Q_UNUSED( sent );
    Q_UNUSED( total );

    kapp->processEvents();

    MtpMediaDevice *dev = (MtpMediaDevice*)(data);

    if( dev->isCanceled() )
    {
        debug() << "Canceling transfer operation";
        dev->setCanceled( true );
        return 1;
    }

    return 0;
}

/**
 * Copy a track to the device
 */
MediaItem
*MtpMediaDevice::copyTrackToDevice( const Meta::TrackPtr track )
{
    DEBUG_BLOCK

    QString genericError = i18n( "Could not send track" );

    LIBMTP_track_t *trackmeta = LIBMTP_new_track_t();
    trackmeta->item_id = 0;
    debug() << "filetype : " << bundle.fileType();
    if( track->type() == "mp3" )
    {
        trackmeta->filetype = LIBMTP_FILETYPE_MP3;
    }
    else if( track->type() == "ogg" )
    {
        trackmeta->filetype = LIBMTP_FILETYPE_OGG;
    }
    else if( track->type() == "wma" )
    {
        trackmeta->filetype = LIBMTP_FILETYPE_WMA;
    }
    else if( track->type() == "mp4" )
    {
        trackmeta->filetype = LIBMTP_FILETYPE_MP4;
    }
    else
    {
        // Couldn't recognise an Amarok filetype.
        // fallback to checking the extension (e.g. .wma, .ogg, etc)
        debug() << "No filetype found by Amarok filetype";

        const QString extension = track->type().toLower();

        int libmtp_type = m_supportedFiles.indexOf( extension );
        if( libmtp_type >= 0 )
        {
            int keyIndex = mtpFileTypes.values().indexOf( extension );
            libmtp_type = mtpFileTypes.keys()[keyIndex];
            trackmeta->filetype = (LIBMTP_filetype_t) libmtp_type;
            debug() << "set filetype to " << libmtp_type << " based on extension of ." << extension;
        }
        else
        {
            debug() << "We do not support the extension ." << extension;
            Amarok::ContextStatusBar::instance()->shortLongMessage(
                genericError,
                i18n( "Cannot determine a valid file type" ),
                KDE::StatusBar::Error
            );
            return 0;
        }
    }

    if( track->prettyName().isEmpty() )
    {
        trackmeta->title = qstrdup( i18n( "Unknown title" ).toUtf8() );
    }
    else
    {
        trackmeta->title = qstrdup( track->prettyName() );
    }

    if( !track->album() )
    {
        trackmeta->album = qstrdup( i18n( "Unknown album" ).toUtf8() );
    }
    else
    {
        trackmeta->album = qstrdup( track->album()->prettyName() );
    }

    if( !track->artist() )
    {
        trackmeta->artist = qstrdup( i18n( "Unknown artist" ).toUtf8() );
    }
    else
    {
        trackmeta->artist = qstrdup( track->artist()->prettyName() );
    }

    if( !track->genre() )
    {
        trackmeta->genre = qstrdup( i18n( "Unknown genre" ).toUtf8() );
    }
    else
    {
        trackmeta->genre = qstrdup( track->genre()->prettyName() );
    }

    if( track->year() > 0 )
    {
        QString date;
        QTextOStream( &date ) << track->year() << "0101T0000.0";
        trackmeta->date = qstrdup( date.toUtf8() );
    }
    else
    {
        trackmeta->date = qstrdup( "00010101T0000.0" );
    }

    if( track->trackNumber() > 0 )
    {
        trackmeta->tracknumber = track->trackNumber();
    }
    if( track->length() > 0 )
    {
        // Multiply by 1000 since this is in milliseconds
        trackmeta->duration = track->length();
    }
    if( !track->playableUrl().fileName().isEmpty() )
    {
        trackmeta->filename = qstrdup( track->playableUrl().fileName() );
    }
    trackmeta->filesize = bundle.filesize();

    // try and create the requested folder structure
    uint32_t parent_id = 0;
    if( !m_folderStructure.isEmpty() )
    {
        parent_id = checkFolderStructure( bundle );
        if( parent_id == 0 )
        {
            debug() << "Could not create new parent (" << m_folderStructure << ")";
            Amarok::ContextStatusBar::instance()->shortLongMessage(
                genericError,
                i18n( "Cannot create parent folder. Check your structure." ),
                KDE::StatusBar::Error
            );
            return 0;
        }
    }
    else
    {
        parent_id = getDefaultParentId();
    }
    debug() << "Parent id : " << parent_id;

    m_critical_mutex.lock();
    debug() << "Sending track... " << track->prettyUrl();
    int ret = LIBMTP_Send_Track_From_File(
        m_device, track->prettyUrl(), trackmeta,
        progressCallback, this, parent_id
    );
    m_critical_mutex.unlock();

    if( ret < 0 )
    {
        debug() << "Could not write file " << ret;
        Amarok::ContextStatusBar::instance()->shortLongMessage(
            genericError,
            i18n( "File write failed" ),
            KDE::StatusBar::Error
        );
        return 0;
    }

    Meta::TrackPtr temp;
    MtpTrack *taggedTrack = new MtpTrack( trackmeta );
    taggedTrack->setTrack( temp );
    taggedTrack->setFolderId( parent_id );

    LIBMTP_destroy_track_t( trackmeta );

    kapp->processEvents();

    // add track to view and to new tracks list
    MediaItem *newItem = addTrackToView( taggedTrack );
    m_newTracks->append( newItem );
    return newItem;
}

/**
 * Get the cover image for a track, convert it to a format supported on the
 * device and set it as the cover art.
 */
void
MtpMediaDevice::sendAlbumArt( QList<MediaItem*> *items )
{
    //PORT 2.0 (CollectionDB)
    /*QString image;
    image = CollectionDB::instance()->albumImage(items->first()->bundle()->artist(), items->first()->bundle()->album(), false, 100);
    if( ! image.endsWith( "@nocover.png" ) )
    {
        debug() << "image " << image << " found for " << items->first()->bundle()->album();
        QByteArray *imagedata = getSupportedImage( image );
        if( imagedata == 0 )
        {
            debug() << "Cannot generate a supported image format";
            return;
        }
        if( imagedata->size() )
        {
            m_critical_mutex.lock();
            LIBMTP_album_t *album_object = getOrCreateAlbum( items );
            if( album_object )
            {
                LIBMTP_filesampledata_t *imagefile = LIBMTP_new_filesampledata_t();
                imagefile->data = (char *) imagedata->data();
                imagefile->size = imagedata->size();
                imagefile->filetype = LIBMTP_FILETYPE_JPEG;
                int ret = LIBMTP_Send_Representative_Sample( m_device, album_object->album_id, imagefile );
                if( ret != 0 )
                {
                    debug() << "image send failed : " << ret;
                }
            }
            m_critical_mutex.unlock();
        }
    }*/
}

uint32_t
MtpMediaDevice::getDefaultParentId( void )
{
    // Decide which folder to send it to:
    // If the device gave us a parent_folder setting, we use it
    uint32_t parent_id = 0;
    if( m_default_parent_folder )
    {
        parent_id = m_default_parent_folder;
    }
    // Otherwise look for a folder called "Music"
    else if( m_folders != 0 )
    {
        parent_id = folderNameToID( "Music", m_folders );
        if( !parent_id )
        {
            debug() << "Parent folder could not be found. Going to use top level.";
        }
    }
    // Give up and don't set a parent folder, let the device deal with it
    else
    {
        debug() << "No folders found. Going to use top level.";
    }
    return parent_id;
}

/**
 * Takes path to an existing cover image and converts it to a format
 * supported on the device
 */
QByteArray
*MtpMediaDevice::getSupportedImage( QString path )
{
    if( m_format == 0 )
        return 0;

    debug() << "Will convert image to " << m_format;

    // open image
    const QImage original( path );

    // save as new image
    QImage newformat( original );
    QByteArray *newimage = new QByteArray();
    QBuffer buffer( newimage );
    buffer.open( QIODevice::WriteOnly );
    if( newformat.save( &buffer, m_format.toAscii() ) )
    {
        buffer.close();
        return newimage;
    }
    return 0;
}

/**
 * Update cover art for a number of tracks
 */
void
MtpMediaDevice::updateAlbumArt( QList<MediaItem*> *items )
{
    DEBUG_BLOCK

    if( m_format == 0 )  // no supported image types. Don't even bother.
        return;

    setCanceled( false );

    kapp->processEvents();
    QMap< QString, QList<MediaItem*> > albumList;

    for( int pos = 0; pos < items->size() && !(m_canceled); ++pos )
    {
        MtpMediaItem *it = dynamic_cast<MtpMediaItem*>( items->at( pos ) );
        if( !it )
            break;
        // build album list
        if( it->type() == MediaItem::TRACK )
        {
            albumList[ it->bundle()->album() ].append( it );
        }
        if( it->type() == MediaItem::ALBUM )
        {
            debug() << "look, we get albums too!";
        }
    }
    int i = 0;
    setProgress( i, albumList.count() );
    kapp->processEvents();
    QMap< QString, QList<MediaItem*> >::Iterator it;
    for( it = albumList.begin(); it != albumList.end(); ++it )
    {
        sendAlbumArt( &it.value() );
        setProgress( ++i );
        if( i % 20 == 0 )
            kapp->processEvents();
    }
    hideProgress();
}

/**
 * Retrieve existing or create new album object.
 */
LIBMTP_album_t
*MtpMediaDevice::getOrCreateAlbum( QList<MediaItem*> *items )//uint32_t track_id, const MetaBundle &bundle )
{
    LIBMTP_album_t *album_object = 0;
    uint32_t albumid = 0;
    int ret;
    QMap<uint32_t,MtpAlbum*>::Iterator it;
    for( it = m_idToAlbum.begin(); it != m_idToAlbum.end(); ++it )
    {
        if( it.value()->album() == items->first()->bundle()->album() )
        {
            albumid = it.value()->id();
            break;
        }
    }
    if( albumid )
    {
        debug() << "reusing existing album " << albumid;
        album_object = LIBMTP_Get_Album( m_device, albumid );
        if( album_object == 0 )
        {
            debug() << "retrieving album failed.";
            return 0;
        }
        uint32_t i;
        uint32_t trackCount = album_object->no_tracks;
        for( int pos = 0; pos < items->size(); ++pos )
        {
            MtpMediaItem *it = dynamic_cast<MtpMediaItem*>( items->at( pos ) );
            if( !it )
                break;
            bool exists = false;
            for( i = 0; i < album_object->no_tracks; i++ )
            {
                if( album_object->tracks[i] == it->track()->id() )
                {
                    exists = true;
                    break;
                }
            }
            if( ! exists )
            {
                debug() << "adding track " << it->track()->id() << " to existing album " << albumid;
                album_object->no_tracks++;
                album_object->tracks = (uint32_t *)realloc( album_object->tracks, album_object->no_tracks * sizeof( uint32_t ) );
                album_object->tracks[ ( album_object->no_tracks - 1 ) ] = it->track()->id();
            }
        }
        if( trackCount != album_object->no_tracks ) // album needs an update
        {
            ret = LIBMTP_Update_Album( m_device, album_object );
            if( ret != 0 )
                debug() << "updating album failed : " << ret;
        }
    }
    else
    {
        debug() << "creating new album ";
        album_object = LIBMTP_new_album_t();
        album_object->name = qstrdup( items->first()->bundle()->album().string().toUtf8() );
        album_object->tracks = (uint32_t *) malloc(items->count() * sizeof(uint32_t));
        int i = 0;
        for( int pos = 0; pos < items->size(); ++pos )
        {
            MtpMediaItem *it = dynamic_cast<MtpMediaItem*>( items->at( pos ) );
            if( !it )
                break;
            album_object->tracks[i++] = it->track()->id();
        }
        album_object->no_tracks = items->count();
        ret = LIBMTP_Create_New_Album( m_device, album_object, 0 );
        if( ret != 0 )
        {
            debug() << "creating album failed : " << ret;
            return 0;
        }
        m_idToAlbum[ album_object->album_id ] = new MtpAlbum( album_object );
    }
    return album_object;
}

/**
 * Check (and optionally create) the folder structure to put a
 * track into. Return the (possibly new) parent folder ID
 */
uint32_t
MtpMediaDevice::checkFolderStructure( const Meta::TrackPtr track, bool create )
{
    QString artistName;
    Meta::ArtistPtr artist = track->artist();
    if( !artist || artist->prettyName().isEmpty() )
        artistName = i18n( "Unknown Artist" );
    else
        artistName = artist->prettyName();
    //FIXME: Port
//     if( bundle.compilation() == MetaBundle::CompilationYes )
//         artist = i18n( "Various Artists" );
    QString albumName;
    Meta::AlbumPtr album = track->album();
    if( !album || album->prettyName().isEmpty() )
        albumName = i18n( "Unknown Album" );
    else
        albumName = album->prettyName();
    QString genreName;
    Meta::GenrePtr genre = track->genre();
    if( !genre || genre->prettyName().isEmpty() )
        genreName = i18n( "Unknown Genre" );
    else
        genreName = genre->prettyName();
    m_critical_mutex.lock();
    uint32_t parent_id = getDefaultParentId();
    QStringList folders = m_folderStructure.split( "/" ); // use slash as a dir separator
    QString completePath;
    for( QStringList::Iterator it = folders.begin(); it != folders.end(); ++it )
    {
        if( (*it).isEmpty() )
            continue;
        // substitute %a , %b , %g
        (*it).replace( QRegExp( "%a" ), artistName )
            .replace( QRegExp( "%b" ), albumName )
            .replace( QRegExp( "%g" ), genreName );
        // check if it exists
        uint32_t check_folder = subfolderNameToID( (*it).toUtf8(), m_folders, parent_id );
        // create if not exists (if requested)
        if( check_folder == 0 )
        {
            if( create )
            {
                check_folder = createFolder( (*it).toUtf8() , parent_id );
                if( check_folder == 0 )
                {
                    m_critical_mutex.unlock();
                    return 0;
                }
            }
            else
            {
                m_critical_mutex.unlock();
                return 0;
            }
        }
        completePath += (*it).toUtf8() + '/';
        // set new parent
        parent_id = check_folder;
    }
    m_critical_mutex.unlock();
    debug() << "Folder path : " << completePath;
    // return parent
    return parent_id;
}

/**
 * Create a new mtp folder
 */
uint32_t
MtpMediaDevice::createFolder( const char *name, uint32_t parent_id )
{
    debug() << "Creating new folder '" << name << "' as a child of "<< parent_id;
    char *name_copy = qstrdup( name );
    uint32_t new_folder_id = LIBMTP_Create_Folder( m_device, name_copy, parent_id );
    delete(name_copy);
    debug() << "New folder ID: " << new_folder_id;
    if( new_folder_id == 0 )
    {
        debug() << "Attempt to create folder '" << name << "' failed.";
        return 0;
    }
    updateFolders();

    return new_folder_id;
}

/**
 * Recursively search the folder list for a matching one under the specified
 * parent ID and return the child's ID
 */
uint32_t
MtpMediaDevice::subfolderNameToID( const char *name, LIBMTP_folder_t *folderlist, uint32_t parent_id )
{
    uint32_t i;

    if( folderlist == 0 )
        return 0;

    if( !strcasecmp( name, folderlist->name ) && folderlist->parent_id == parent_id )
        return folderlist->folder_id;

    if( ( i = ( subfolderNameToID( name, folderlist->child, parent_id ) ) ) )
        return i;
    if( ( i = ( subfolderNameToID( name, folderlist->sibling, parent_id ) ) ) )
        return i;

    return 0;
}

/**
 * Recursively search the folder list for a matching one
 * and return its ID
 */
uint32_t
MtpMediaDevice::folderNameToID( char *name, LIBMTP_folder_t *folderlist )
{
    uint32_t i;

    if( folderlist == 0 )
        return 0;

    if( !strcasecmp( name, folderlist->name ) )
        return folderlist->folder_id;

    if( ( i = ( folderNameToID( name, folderlist->child ) ) ) )
        return i;
    if( ( i = ( folderNameToID( name, folderlist->sibling ) ) ) )
        return i;

    return 0;
}

/**
 * Get a list of selected items, download them to a temporary location and
 * organize.
 */
int
MtpMediaDevice::downloadSelectedItemsToCollection()
{
    QList<MediaItem*> items;
    m_view->getSelectedLeaves( 0, &items );

    KTempDir tempdir;
    tempdir.setAutoRemove( true );
    KUrl::List urls;
    QString genericError = i18n( "Could not copy track from device." );

    int total,progress;
    total = items.count();
    progress = 0;

    if( total == 0 )
        return 0;

    setProgress( progress, total );
    for( int pos = 0; pos < items.size() && !(m_canceled); ++pos )
    {
        MtpMediaItem *it = dynamic_cast<MtpMediaItem*>( items.at( pos ) );
        if( !it )
            break;
        if( it->type() == MediaItem::TRACK )
        {
            QString filename = tempdir.name() + it->bundle()->filename();
            int ret = LIBMTP_Get_Track_To_File(
                    m_device, it->track()->id(), filename.toUtf8(),
                    progressCallback, this
                  );
            if( ret != 0 )
            {
                debug() << "Get Track failed: " << ret;
                Amarok::ContextStatusBar::instance()->shortLongMessage(
                    genericError,
                    i18n( "Could not copy track from device." ),
                    KDE::StatusBar::Error
                );
            }
            else
            {
                urls << filename;
                progress++;
                setProgress( progress );
            }
        }
        else
        {
            total--;
            setProgress( progress, total );
        }
    }
    hideProgress();
//PORT 2.0    CollectionView::instance()->organizeFiles( urls, i18n( "Move Files To Collection" ), false );
    return 0;
}

/**
 * Write any pending changes to the device, such as database changes
 */
void
MtpMediaDevice::synchronizeDevice()
{
    updateAlbumArt( m_newTracks );
    m_newTracks->clear();
    return;
}

/**
 * Find an existing track
 */
MediaItem
*MtpMediaDevice::trackExists( const Meta::TrackPtr &track )
{
    MediaItem *artist = dynamic_cast<MediaItem *>( m_view->findItem( track->artist()->prettyName(), 0 ) );
    if( artist )
    {
        MediaItem *album = dynamic_cast<MediaItem *>( artist->findItem( track->album()->prettyName() ) );
        if( album )
        {
            MediaItem *track = dynamic_cast<MediaItem *>( album->findItem( track->prettyName() ) );
            if( track )
                return track;
        }
    }
    uint32_t folderId = checkFolderStructure( track, false );
    MediaItem *file = m_fileNameToItem[ QString( "%1/%2" ).arg( folderId ).arg( bundle.filename() ) ];
    if( file != 0 )
	return file;
    return 0;
}

/**
 * Create a new playlist
 */
MtpMediaItem
*MtpMediaDevice::newPlaylist( const QString &name, MediaItem *parent, QList<MediaItem*> items )
{
    DEBUG_BLOCK
    MtpMediaItem *item = new MtpMediaItem( parent, this );
    item->setType( MediaItem::PLAYLIST );
    item->setText( 0, name );
    item->setPlaylist( new MtpPlaylist() );

    addToPlaylist( item, 0, items );

    if( ! isTransferring() )
        m_view->rename( item, 0 );

    return item;
}

/**
 * Add an item to a playlist
 */
void
MtpMediaDevice::addToPlaylist( MediaItem *mlist, MediaItem *after, QList<MediaItem*> items )
{
    DEBUG_BLOCK
    MtpMediaItem *list = dynamic_cast<MtpMediaItem *>( mlist );
    if( !list )
        return;

    int order;
    MtpMediaItem *it;
    if( after )
    {
        order = after->m_order + 1;
        it = dynamic_cast<MtpMediaItem*>(after->nextSibling());
    }
    else
    {
        order = 0;
        it = dynamic_cast<MtpMediaItem*>( list->firstChild() );
    }

    for(  ; it; it = dynamic_cast<MtpMediaItem *>( it->nextSibling() ) )
    {
        it->m_order += items.count();
    }

    for( int pos = 0; pos < items.size(); ++pos )
    {
        MtpMediaItem *it = dynamic_cast<MtpMediaItem *>( items.at( pos ) );
        if( !it )
            break;
        if( !it->track() )
            continue;

        MtpMediaItem *add;
        if( it->parent() == list )
        {
            add = it;
            if( after )
            {
                it->moveItem(after);
            }
            else
            {
                list->takeItem(it);
                list->insertItem(it);
            }
        }
        else
        {
            if( after )
            {
                add = new MtpMediaItem( list, after );
            }
            else
            {
                add = new MtpMediaItem( list, this );
            }
        }
        after = add;

        add->setType( MediaItem::PLAYLISTITEM );
        add->setTrack( it->track() );
        add->setBundle( new MetaBundle( *(it->bundle()) ) );
        add->m_device = this;
        add->setText( 0, it->bundle()->artist() + " - " + it->bundle()->title() );
        add->m_order = order;
        order++;
    }

    // make numbering consecutive
    int i = 0;
    for( MtpMediaItem *it = dynamic_cast<MtpMediaItem *>( list->firstChild() );
            it;
            it = dynamic_cast<MtpMediaItem *>( it->nextSibling() ) )
    {
        it->m_order = i;
        i++;
    }

    playlistFromItem( list );
}

/**
 * When a playlist has been renamed, we must save it
 */
void
MtpMediaDevice::playlistRenamed( Q3ListViewItem *item, const QString &, int ) // SLOT
{
    DEBUG_BLOCK
    MtpMediaItem *playlist = static_cast<MtpMediaItem*>( item );
    if( playlist->type() == MediaItem::PLAYLIST )
        playlistFromItem( playlist );
}

/**
 * Save a playlist
 */
void
MtpMediaDevice::playlistFromItem( MtpMediaItem *item )
{
    if( item->childCount() == 0 )
        return;
    m_critical_mutex.lock();
    LIBMTP_playlist_t *metadata = LIBMTP_new_playlist_t();
    metadata->name = qstrdup( item->text( 0 ).toUtf8() );
    const int trackCount = item->childCount();
    if (trackCount > 0) {
        uint32_t *tracks = ( uint32_t* )malloc( sizeof( uint32_t ) * trackCount );
        uint32_t i = 0;
        for( MtpMediaItem *it = dynamic_cast<MtpMediaItem *>(item->firstChild());
                it;
                it = dynamic_cast<MtpMediaItem *>(it->nextSibling()) )
        {
            tracks[i] = it->track()->id();
            i++;
        }
        metadata->tracks = tracks;
        metadata->no_tracks = i;
    } else {
        debug() << "no tracks available for playlist " << metadata->name
           ;
        metadata->no_tracks = 0;
    }
    QString genericError = i18n( "Could not save playlist." );

    uint32_t *tracks = ( uint32_t* )malloc( sizeof( uint32_t ) * item->childCount() );
    uint32_t i = 0;
    for( MtpMediaItem *it = dynamic_cast<MtpMediaItem *>(item->firstChild());
            it;
            it = dynamic_cast<MtpMediaItem *>(it->nextSibling()) )
    {
        tracks[i] = it->track()->id();
        i++;
    }
    metadata->tracks = tracks;
    metadata->no_tracks = i;

    if( item->playlist()->id() == 0 )
    {
        debug() << "creating new playlist : " << metadata->name;
        int ret = LIBMTP_Create_New_Playlist( m_device, metadata, 0 );
        if( ret == 0 )
        {
            item->playlist()->setId( metadata->playlist_id );
            debug() << "playlist saved : " << metadata->playlist_id;
        }
        else
        {
            Amarok::ContextStatusBar::instance()->shortLongMessage(
                genericError,
                i18n( "Could not create new playlist on device." ),
                KDE::StatusBar::Error
            );
        }
    }
    else
    {
        metadata->playlist_id = item->playlist()->id();
        debug() << "updating playlist : " << metadata->name;
        int ret = LIBMTP_Update_Playlist( m_device, metadata );
        if( ret != 0 )
        {
            Amarok::ContextStatusBar::instance()->shortLongMessage(
                genericError,
                i18n( "Could not update playlist on device." ),
                KDE::StatusBar::Error
            );
        }
    }
    m_critical_mutex.unlock();
}

/**
 * Recursively remove MediaItem from the device and media view
 */
int
MtpMediaDevice::deleteItemFromDevice(MediaItem* item, int flags )
{
    //PORT 2.0
    //If nothing is left in a folder, delete the folder
    int result = 0;
    if( isCanceled() || !item )
    {
        return -1;
    }
    MediaItem *next = 0;

    switch( item->type() )
    {
        case MediaItem::PLAYLIST:
        case MediaItem::TRACK:
            if( isCanceled() )
                break;
            if( item )
            {
                int res = deleteObject( dynamic_cast<MtpMediaItem *> ( item ) );
                if( res >=0 && result >= 0 )
                    result += res;
                else
                    result = -1;
            }
            break;
        case MediaItem::PLAYLISTITEM:
            if( isCanceled() )
                break;
            if( item )
            {
                MtpMediaItem *parent = dynamic_cast<MtpMediaItem *> ( item->parent() );
                if( parent && parent->type() == MediaItem::PLAYLIST ) {
                    delete( item );
                    playlistFromItem( parent );
                }
            }
            break;
        case MediaItem::ALBUM:
        case MediaItem::ARTIST:
            // Recurse through the lists
            next = 0;

            if( isCanceled() )
                break;

            for( MediaItem *it = dynamic_cast<MediaItem *>( item->firstChild() ); it ; it = next )
            {
                next = dynamic_cast<MediaItem *>( it->nextSibling() );
                int res = deleteItemFromDevice( it, flags );
                if( res >= 0 && result >= 0 )
                    result += res;
                else
                    result = -1;

            }
            if( item )
                delete dynamic_cast<MediaItem *>( item );
            break;
        default:
            result = 0;
    }
    return result;
}

/**
 * Actually delete a track or playlist
 */
int
MtpMediaDevice::deleteObject( MtpMediaItem *deleteItem )
{
    DEBUG_BLOCK
    //PORT 2.0
    //If nothing is left in a folder, delete the folder
    u_int32_t object_id;
    if( deleteItem->type() == MediaItem::PLAYLIST )
        object_id = deleteItem->playlist()->id();
    else
        object_id = deleteItem->track()->id();

    QString genericError = i18n( "Could not delete item" );

    debug() << "delete this id : " << object_id;

    m_critical_mutex.lock();
    int status = LIBMTP_Delete_Object( m_device, object_id );
    m_critical_mutex.unlock();

    if( status != 0 )
    {
        debug() << "delete object failed";
        Amarok::ContextStatusBar::instance()->shortLongMessage(
            genericError,
            i18n( "Delete failed" ),
            KDE::StatusBar::Error
        );
        return -1;
    }
    debug() << "object deleted";

    // clear cached filename
    if( deleteItem->type() == MediaItem::TRACK )
        m_fileNameToItem.remove( QString( "%1/%2" ).arg( deleteItem->track()->folderId() ).arg( deleteItem->bundle()->filename() ) );
    // remove from the media view
    delete deleteItem;
    kapp->processEvents();

    return 1;
}

/**
 * Update local cache of mtp folders
 */
void
MtpMediaDevice::updateFolders( void )
{
    LIBMTP_destroy_folder_t( m_folders );
    m_folders = 0;
    m_folders = LIBMTP_Get_Folder_List( m_device );
}

/**
 * Set cancellation of an operation
 */
void
MtpMediaDevice::cancelTransfer()
{
    m_canceled = true;
}

/**
 * Connect to device, and populate m_view with MediaItems
 */
bool
MtpMediaDevice::openDevice( bool silent )
{
    DEBUG_BLOCK

    Q_UNUSED( silent );

    if( m_device != 0 )
        return true;


    QString genericError = i18n( "Could not connect to MTP Device" );

    m_critical_mutex.lock();
    LIBMTP_Init();
    m_device = LIBMTP_Get_First_Device();
    m_critical_mutex.unlock();
    if( m_device == 0 ) {
        debug() << "No devices.";
        Amarok::ContextStatusBar::instance()->shortLongMessage(
            genericError,
            i18n( "MTP device could not be opened" ),
            KDE::StatusBar::Error
        );
        setDisconnected();
        return false;
    }

    connect(
        m_view, SIGNAL( itemRenamed( Q3ListViewItem*, const QString&, int ) ),
        this,   SLOT( playlistRenamed( Q3ListViewItem*, const QString&, int ) )
    );

    QString modelname = QString( LIBMTP_Get_Modelname( m_device ) );
    QString ownername = QString( LIBMTP_Get_Friendlyname( m_device ) );
    m_name = modelname;
    if(! ownername.isEmpty() )
        m_name += " (" + ownername + ')';

    m_default_parent_folder = m_device->default_music_folder;
    debug() << "setting default parent : " << m_default_parent_folder;

    MtpMediaDevice::readMtpMusic();

    m_critical_mutex.lock();
    m_folders = LIBMTP_Get_Folder_List( m_device );
    uint16_t *filetypes;
    uint16_t filetypes_len;
    int ret = LIBMTP_Get_Supported_Filetypes( m_device, &filetypes, &filetypes_len );
    if( ret == 0 )
    {
        uint16_t i;
        for( i = 0; i < filetypes_len; i++ )
            m_supportedFiles << mtpFileTypes[ filetypes[ i ] ];
    }
    // find supported image types (for album art).
    if( m_supportedFiles.indexOf( "jpg" ) )
        m_format = "JPEG";
    else if( m_supportedFiles.indexOf( "png" ) )
        m_format = "PNG";
    else if( m_supportedFiles.indexOf( "gif" ) )
        m_format = "GIF";
    free( filetypes );
    m_critical_mutex.unlock();

    return true;
}

/**
 * Start the view (add default folders such as for playlists)
 */
void
MtpMediaDevice::initView()
{
    if( ! isConnected() )
        return;
    m_playlistItem = new MtpMediaItem( m_view, this );
    m_playlistItem->setText( 0, i18n("Playlists") );
    m_playlistItem->setType( MediaItem::PLAYLISTSROOT );
    m_playlistItem->m_order = -1;
}

/**
 * Wrap up any loose ends and close the device
 */
bool
MtpMediaDevice::closeDevice()
{
    DEBUG_BLOCK

    // clear folder structure
    if( m_folders != 0 )
    {
        m_critical_mutex.lock();
        LIBMTP_destroy_folder_t( m_folders );
        m_critical_mutex.unlock();
        m_folders = 0;
        debug() << "Folders destroyed";
    }

    // release device
    if( m_device != 0 )
    {
        m_critical_mutex.lock();
        LIBMTP_Release_Device( m_device );
        m_critical_mutex.unlock();
        setDisconnected();
        debug() << "Device released";
    }

    // clear the cached mappings
    m_idToAlbum.clear();
    m_idToTrack.clear();
    m_fileNameToItem.clear();

    // clean up the view
    clearItems();

    return true;
}

/**
 * Get the capacity and freespace available on the device, in KB
 */
bool
MtpMediaDevice::getCapacity( KIO::filesize_t *total, KIO::filesize_t *available )
{
    if( !isConnected() )
        return false;

    // TODO : Follow the links so we sum up all the device's storage.
    *total = m_device->storage->MaxCapacity;
    *available = m_device->storage->FreeSpaceInBytes;
    return true;
}

/**
 * Get custom information about the device via MTP
 */
void
MtpMediaDevice::customClicked()
{
    DEBUG_BLOCK
    QString Information;
    if( isConnected() )
    {
        QString batteryLevel;
        QString serialNumber;
        QString supportedFiles;

        uint8_t maxbattlevel;
        uint8_t currbattlevel;


        m_critical_mutex.lock();
        LIBMTP_Get_Batterylevel( m_device, &maxbattlevel, &currbattlevel );
        char *serialnum = LIBMTP_Get_Serialnumber( m_device );
        m_critical_mutex.unlock();

        batteryLevel = i18n("Battery level: ")
            + QString::number( (int) ( (float) currbattlevel / (float) maxbattlevel * 100.0 ) )
            + '%';
        serialNumber = i18n("Serial number: ") + QString::fromUtf8( serialnum );
        if( m_supportedFiles.at( m_supportedFiles.size()-1 ).isEmpty() )
            m_supportedFiles.removeLast();
        supportedFiles = i18n("Supported file types: ")
            + m_supportedFiles.join( ", " );

        Information = ( i18n( "Player Information for " )
                        + m_name + ":\n" + batteryLevel
                        + '\n' + serialNumber + '\n'
                        + supportedFiles );
        free( serialnum );
    }
    else
    {
        Information = i18n( "Player not connected" );
    }

    KMessageBox::information( 0, Information, i18n( "Device information" ) );
}

/**
 * Current device
 */
LIBMTP_mtpdevice_t
*MtpMediaDevice::current_device()
{
    return m_device;
}

/**
 * We use a 0 device to show a disconnected device.
 * This sets the device to that.
 */
void
MtpMediaDevice::setDisconnected()
{
    m_device = 0;
}

/**
 * Handle clicking of the right mouse button
 */
void
MtpMediaDevice::rmbPressed( Q3ListViewItem *qitem, const QPoint &point, int )
{

    enum Actions {RENAME, DOWNLOAD, DELETE, MAKE_PLAYLIST, UPDATE_ALBUM_ART};

    MtpMediaItem *item = static_cast<MtpMediaItem *>( qitem );
    if( item )
    {
        K3PopupMenu menu( m_view );
        switch( item->type() )
        {
        case MediaItem::ARTIST:
        case MediaItem::ALBUM:
        case MediaItem::TRACK:
            menu.insertItem( KIcon( "collection-amarok" ), i18n("&Copy Files to Collection..."), DOWNLOAD );
            menu.insertItem( KIcon( "view-media-playlist-amarok" ), i18n( "Make Media Device Playlist" ), MAKE_PLAYLIST );
            menu.insertItem( KIcon( "covermanager-amarok" ), i18n( "Refresh Cover Images" ), UPDATE_ALBUM_ART );
            break;
        case MediaItem::PLAYLIST:
            menu.insertItem( KIcon( "edit-rename-amarok" ), i18n( "Rename" ), RENAME );
            break;
        default:
            break;
        }

        menu.insertItem( KIcon( "edit-delete-amarok" ), i18n( "Delete from device" ), DELETE );

        int id =  menu.exec( point );
        switch( id )
        {
        case MAKE_PLAYLIST:
            {
                QList<MediaItem*> items;
                m_view->getSelectedLeaves( 0, &items );
                QString name = i18n( "New Playlist" );
                newPlaylist( name, m_playlistItem, items );
            }
            break;
        case DELETE:
            MediaDevice::deleteFromDevice();
            break;
        case RENAME:
            if( item->type() == MediaItem::PLAYLIST && ! isTransferring() )
            {
                m_view->rename( item, 0 );
            }
            break;
        case DOWNLOAD:
            downloadSelectedItemsToCollection();
            break;
        case UPDATE_ALBUM_ART:
            {
                QList<MediaItem*> *items = new QList<MediaItem*>;
                m_view->getSelectedLeaves( 0, items );

                if( items->count() > 100 )
                {
                    int button = KMessageBox::warningContinueCancel( m_parent,
                            i18np( "<p>You are updating cover art for 1 track. This may take some time.</p>",
                                "<p>You are updating cover art for %1 tracks. This may take some time.</p>",
                                items->count()
                                ),
                            QString() );

                    if( button != KMessageBox::Continue )
                        return;
                }
                updateAlbumArt( items );
                break;
            }
        }
    }
    return;
}

/**
 * Add gui elements to the device configuration
 */
void
MtpMediaDevice::addConfigElements( QWidget *parent )
{

    m_folderLabel = new QLabel( parent );
    m_folderLabel->setText( i18n( "Folder structure:" ) );

    m_folderStructureBox = new QLineEdit( parent );
    m_folderStructureBox->setText( m_folderStructure );
    m_folderStructureBox->setToolTip(
        i18n( "Files copied to the device will be placed in this folder." ) + '\n'
        + i18n( "/ is used as folder separator." ) + '\n'
        + i18n( "%a will be replaced with the artist name, ")
        + i18n( "%b with the album name," ) + '\n'
        + i18n( "%g with the genre.") + '\n'
        + i18n( "An empty path means the files will be placed unsorted in the default music folder." ) );
}

/**
 * Remove gui elements from the device configuration
 */
void
MtpMediaDevice::removeConfigElements( QWidget *parent)
{
    Q_UNUSED(parent)

    delete m_folderStructureBox;
    m_folderStructureBox = 0;

    delete m_folderLabel;
    m_folderLabel = 0;
}

/**
 * Save changed config after dialog commit
 */
void
MtpMediaDevice::applyConfig()
{
    m_folderStructure = m_folderStructureBox->text();
    setConfigString( "FolderStructure", m_folderStructure );
}

/**
 * Load config from the amarokrc file
 */
void
MtpMediaDevice::loadConfig()
{
    m_folderStructure = configString( "FolderStructure","%a - %b" );
}

/**
 * Add a track to the current list view
 */
MtpMediaItem
*MtpMediaDevice::addTrackToView( MtpTrack *track, MtpMediaItem *item )
{
    QString artistName = track->bundle()->artist();

    MtpMediaItem *artist = dynamic_cast<MtpMediaItem *>( m_view->findItem( artistName, 0 ) );
    if( !artist )
    {
        artist = new MtpMediaItem(m_view);
        artist->m_device = this;
        artist->setText( 0, artistName );
        artist->setType( MediaItem::ARTIST );
    }

    QString albumName = track->bundle()->album();
    MtpMediaItem *album = dynamic_cast<MtpMediaItem *>( artist->findItem( albumName ) );
    if( !album )
    {
        album = new MtpMediaItem( artist );
        album->setText( 0, albumName );
        album->setType( MediaItem::ALBUM );
        album->m_device = this;
    }

    if( item )
        album->insertItem( item );
    else
    {
        item = new MtpMediaItem( album );
        item->m_device = this;
        QString titleName = track->bundle()->title();
        item->setTrack( track );
        item->m_order = track->bundle()->track();
        item->setText( 0, titleName );
        item->setType( MediaItem::TRACK );
        item->setBundle( track->bundle() );
        item->track()->setId( track->id() );
        m_fileNameToItem[ QString( "%1/%2" ).arg( track->folderId() ).arg( track->bundle()->filename() ) ] = item;
        m_idToTrack[ track->id() ] = track;
    }
    return item;
}

/**
 * Get tracks and add them to the listview
 */
int
MtpMediaDevice::readMtpMusic()
{
    DEBUG_BLOCK

    clearItems();

    m_critical_mutex.lock();

    QString genericError = i18n( "Could not get music from MTP Device" );

    int total = 100;
    int progress = 0;
    setProgress( progress, total ); // we don't know how many tracks. fake progress bar.

    kapp->processEvents();

    LIBMTP_track_t *tracks = LIBMTP_Get_Tracklisting_With_Callback( m_device, progressCallback, this );

    debug() << "Got tracks from device";

    if( tracks == 0 )
    {
        debug() << "0 tracks returned. Empty device...";
    }
    else
    {
        LIBMTP_track_t *tmp = tracks;
        total = 0;
        // spin through once to determine size of the list
        while( tracks != 0 )
        {
            tracks = tracks->next;
            total++;
        }
        setProgress( progress, total );
        tracks = tmp;
        // now process the tracks
        while( tracks != 0 )
        {
            MtpTrack *mtp_track = new MtpTrack( tracks );
            mtp_track->readMetaData( tracks );
            addTrackToView( mtp_track );
            tmp = tracks;
            tracks = tracks->next;
            LIBMTP_destroy_track_t( tmp );
            progress++;
            setProgress( progress );
            if( progress % 50 == 0 )
                kapp->processEvents();
        }
    }

    readPlaylists();
    readAlbums();

    setProgress( total );
    hideProgress();

    m_critical_mutex.unlock();

    return 0;
}

/**
 * Populate playlists
 */
void
MtpMediaDevice::readPlaylists()
{
    LIBMTP_playlist_t *playlists = LIBMTP_Get_Playlist_List( m_device );

    if( playlists != 0 )
    {
        LIBMTP_playlist_t *tmp;
        while( playlists != 0 )
        {
            MtpMediaItem *playlist = new MtpMediaItem( m_playlistItem, this );
            playlist->setText( 0, QString::fromUtf8( playlists->name ) );
            playlist->setType( MediaItem::PLAYLIST );
            playlist->setPlaylist( new MtpPlaylist() );
            playlist->playlist()->setId( playlists->playlist_id );
            uint32_t i;
            for( i = 0; i < playlists->no_tracks; i++ )
            {
                MtpTrack *track = m_idToTrack[ playlists->tracks[i] ];
                if( track == 0 ) // skip invalid playlist entries
                    continue;
                MtpMediaItem *item = new MtpMediaItem( playlist );
                item->setText( 0, track->bundle()->artist() + " - " + track->bundle()->title() );
                item->setType( MediaItem::PLAYLISTITEM );
                item->setBundle( track->bundle() );
                item->setTrack( track );
                item->m_order = i;
                item->m_device = this;
            }
            tmp = playlists;
            playlists = playlists->next;
            LIBMTP_destroy_playlist_t( tmp );
            kapp->processEvents();
        }
    }
}


/**
 * Read existing albums
 */
void
MtpMediaDevice::readAlbums()
{
    LIBMTP_album_t *albums = LIBMTP_Get_Album_List( m_device );

    if( albums != 0 )
    {
        LIBMTP_album_t *tmp;
        while( albums != 0 )
        {
            m_idToAlbum[ albums->album_id ] = new MtpAlbum( albums );
            tmp = albums;
            albums = albums->next;
            LIBMTP_destroy_album_t( tmp );
            kapp->processEvents();
        }
    }
}

/**
 * Clear the current listview
 */
void
MtpMediaDevice::clearItems()
{
    m_view->clear();
    initView();
}

/**
 * MtpTrack Class
 */
MtpTrack::MtpTrack( LIBMTP_track_t *track )
{
    m_id = track->item_id;
}

/**
 * Read track properties from the device and set it on the track
 */
void
MtpTrack::readMetaData( LIBMTP_track_t *track )
{
    MetaBundle *bundle = new MetaBundle();

    if( track->genre != 0 )
        bundle->setGenre( AtomicString( QString::fromUtf8( track->genre ) ) );
    if( track->artist != 0 )
        bundle->setArtist( AtomicString( QString::fromUtf8( track->artist ) ) );
    if( track->album != 0 )
        bundle->setAlbum( AtomicString( QString::fromUtf8( track->album ) ) );
    if( track->title != 0 )
        bundle->setTitle( AtomicString( QString::fromUtf8( track->title ) ) );
    if( track->filename != 0 )
        bundle->setPath( AtomicString( QString::fromUtf8( track->filename ) ) );

    // translate codecs to file types
    if( track->filetype == LIBMTP_FILETYPE_MP3 )
        bundle->setFileType( MetaBundle::mp3 );
    else if( track->filetype == LIBMTP_FILETYPE_WMA )
        bundle->setFileType( MetaBundle::wma );
    else if( track->filetype == LIBMTP_FILETYPE_OGG )
        bundle->setFileType( MetaBundle::ogg );
    else
        bundle->setFileType( MetaBundle::other );

    if( track->date != 0 )
        bundle->setYear( QString( QString::fromUtf8( track->date ) ).mid( 0, 4 ).toUInt() );
    if( track->tracknumber > 0 )
        bundle->setTrack( track->tracknumber );
    if( track->duration > 0 )
        bundle->setLength( track->duration / 1000 ); // Divide by 1000 since this is in milliseconds

    this->setFolderId( track->parent_id );

    this->setBundle( *bundle );
}

/**
 * Set this track's metabundle
 */
void
MtpTrack::setBundle( Meta::TrackPtr track )
{
    m_track = track;
}

/**
 * MtpAlbum Class
 */
MtpAlbum::MtpAlbum( LIBMTP_album_t *album )
{
    m_id = album->album_id;
    m_album = QString::fromUtf8( album->name );
}

#include "mtpmediadevice.moc"

