/*  Copyright (C) 2005-2007 Jeff Mitchell <kde-dev@emailgoeshere.com>
    (c) 2004 Christian Muehlhaeuser <chris@chris.de>
    (c) 2005-2006 Martin Aumueller <aumuell@reserv.at>
    (c) 2005 Seb Ruiz <me@sebruiz.net>
    (c) 2006 T.R.Shashwath <trshash84@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "amarok.h"
#include "mediabrowser.h"
#include "MediaItem.h"

#include "kiconloader.h"

#include <QPainter>

QPixmap *MediaItem::s_pixUnknown = 0;
QPixmap *MediaItem::s_pixArtist = 0;
QPixmap *MediaItem::s_pixAlbum = 0;
QPixmap *MediaItem::s_pixFile = 0;
QPixmap *MediaItem::s_pixTrack = 0;
QPixmap *MediaItem::s_pixPodcast = 0;
QPixmap *MediaItem::s_pixPlaylist = 0;
QPixmap *MediaItem::s_pixInvisible = 0;
QPixmap *MediaItem::s_pixStale = 0;
QPixmap *MediaItem::s_pixOrphaned = 0;
QPixmap *MediaItem::s_pixDirectory = 0;
QPixmap *MediaItem::s_pixRootItem = 0;
QPixmap *MediaItem::s_pixTransferFailed = 0;
QPixmap *MediaItem::s_pixTransferBegin = 0;
QPixmap *MediaItem::s_pixTransferEnd = 0;

MediaItem::MediaItem( Q3ListView* parent )
: K3ListViewItem( parent )
{
    init();
}

MediaItem::MediaItem( Q3ListViewItem* parent )
: K3ListViewItem( parent )
{
    init();
}

MediaItem::MediaItem( Q3ListView* parent, Q3ListViewItem* after )
: K3ListViewItem( parent, after )
{
    init();
}

MediaItem::MediaItem( Q3ListViewItem* parent, Q3ListViewItem* after )
: K3ListViewItem( parent, after )
{
    init();
}

MediaItem::~MediaItem()
{
    setBundle( 0 );
}

void
MediaItem::init()
{
    // preload pixmaps used in browser
    KIconLoader iconLoader;
    MediaItem::s_pixUnknown = new QPixmap(iconLoader.loadIcon( Amarok::icon( "unknown" ), K3Icon::Toolbar, K3Icon::SizeSmall ));
    MediaItem::s_pixTrack = new QPixmap(iconLoader.loadIcon( Amarok::icon( "playlist" ), K3Icon::Toolbar, K3Icon::SizeSmall ));
    MediaItem::s_pixFile = new QPixmap(iconLoader.loadIcon( Amarok::icon( "sound" ), K3Icon::Toolbar, K3Icon::SizeSmall ) );
    MediaItem::s_pixPodcast = new QPixmap(iconLoader.loadIcon( Amarok::icon( "podcast" ), K3Icon::Toolbar, K3Icon::SizeSmall ) );
    MediaItem::s_pixPlaylist = new QPixmap(iconLoader.loadIcon( Amarok::icon( "playlist" ), K3Icon::Toolbar, K3Icon::SizeSmall ) );
    MediaItem::s_pixRootItem = new QPixmap(iconLoader.loadIcon( Amarok::icon( "files2" ), K3Icon::Toolbar, K3Icon::SizeSmall ) );
    // history
    // favorites
    // collection
    // folder
    // folder_red
    // player_playlist_2
    // cancel
    // sound
    MediaItem::s_pixArtist = new QPixmap(iconLoader.loadIcon( Amarok::icon( "personal" ), K3Icon::Toolbar, K3Icon::SizeSmall ) );
    MediaItem::s_pixAlbum = new QPixmap(iconLoader.loadIcon( Amarok::icon( "cdrom_unmount" ), K3Icon::Toolbar, K3Icon::SizeSmall ) );
    MediaItem::s_pixInvisible = new QPixmap(iconLoader.loadIcon( Amarok::icon( "cancel" ), K3Icon::Toolbar, K3Icon::SizeSmall ) );
    MediaItem::s_pixStale = new QPixmap(iconLoader.loadIcon( Amarok::icon( "cancel" ), K3Icon::Toolbar, K3Icon::SizeSmall ) );
    MediaItem::s_pixOrphaned = new QPixmap(iconLoader.loadIcon( Amarok::icon( "cancel" ), K3Icon::Toolbar, K3Icon::SizeSmall ) );
    MediaItem::s_pixDirectory = new QPixmap(iconLoader.loadIcon( Amarok::icon( "folder" ), K3Icon::Toolbar, K3Icon::SizeSmall ) );
    MediaItem::s_pixTransferBegin = new QPixmap(iconLoader.loadIcon( Amarok::icon( "play" ), K3Icon::Toolbar, K3Icon::SizeSmall ) );
    MediaItem::s_pixTransferEnd = new QPixmap(iconLoader.loadIcon( Amarok::icon( "process-stop" ), K3Icon::Toolbar, K3Icon::SizeSmall ) );
    MediaItem::s_pixTransferFailed = new QPixmap(iconLoader.loadIcon( Amarok::icon( "cancel" ), K3Icon::Toolbar, K3Icon::SizeSmall ) );
    
    m_bundle=0;
    m_order=0;
    m_type=UNKNOWN;
    m_playlistName.clear();
    m_device=0;
    m_flags=0;
    setExpandable( false );
    setDragEnabled( true );
    setDropEnabled( true );
}

void
MediaItem::setBundle( MetaBundle *bundle )
{
    MediaBrowser::instance()->m_itemMapMutex.lock();
    if( m_bundle )
    {
    QString itemUrl = url().url();
        MediaBrowser::ItemMap::iterator it = MediaBrowser::instance()->m_itemMap.find( itemUrl );
        if( it != MediaBrowser::instance()->m_itemMap.end() && *it == this )
            MediaBrowser::instance()->m_itemMap.remove( itemUrl );
    }
    delete m_bundle;
    m_bundle = bundle;

    if( m_bundle )
    {
    QString itemUrl = url().url();
        MediaBrowser::ItemMap::iterator it = MediaBrowser::instance()->m_itemMap.find( itemUrl );
        if( it == MediaBrowser::instance()->m_itemMap.end() )
            MediaBrowser::instance()->m_itemMap[itemUrl] = this;
    }
    MediaBrowser::instance()->m_itemMapMutex.unlock();
    createToolTip();
}

void MediaItem::paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align )
{
    switch( type() )
    {
    case INVISIBLE:
    case PODCASTSROOT:
    case PLAYLISTSROOT:
    case ORPHANEDROOT:
    case STALEROOT:
        {
            QFont font( p->font() );
            font.setBold( true );
            p->setFont( font );
        }
    default:
        break;
    }

    K3ListViewItem::paintCell( p, cg, column, width, align );
}

const MetaBundle *
MediaItem::bundle() const
{
    return m_bundle;
}

KUrl
MediaItem::url() const
{
    if( bundle() )
        return bundle()->url();
    else
        return KUrl();
}

bool
MediaItem::isFileBacked() const
{
    switch( type() )
    {
    case ARTIST:
    case ALBUM:
    case PODCASTSROOT:
    case PODCASTCHANNEL:
    case PLAYLISTSROOT:
    case PLAYLIST:
    case PLAYLISTITEM:
    case INVISIBLEROOT:
    case STALEROOT:
    case STALE:
    case ORPHANEDROOT:
        return false;

    case UNKNOWN:
    case TRACK:
    case ORPHANED:
    case INVISIBLE:
    case PODCASTITEM:
    case DIRECTORY:
        return true;
    }

    return false;
}

long
MediaItem::size() const
{
    if( !isFileBacked() )
        return 0;

    if( bundle() )
        return bundle()->filesize();

    return 0;
}

void
MediaItem::setType( Type type )
{
    m_type=type;

    setDragEnabled(true);
    setDropEnabled(false);

    switch(m_type)
    {
        case UNKNOWN:
            setPixmap(0, *s_pixUnknown);
            break;
        case INVISIBLE:
        case TRACK:
            setPixmap(0, *s_pixFile);
            break;
        case PLAYLISTITEM:
            setPixmap(0, *s_pixTrack);
            setDropEnabled(true);
            break;
        case ARTIST:
            setPixmap(0, *s_pixArtist);
            break;
        case ALBUM:
            setPixmap(0, *s_pixAlbum);
            break;
        case PODCASTSROOT:
            setPixmap(0, *s_pixRootItem);
            break;
        case PODCASTITEM:
        case PODCASTCHANNEL:
            setPixmap(0, *s_pixPodcast);
            break;
        case PLAYLIST:
            setPixmap(0, *s_pixPlaylist);
            setDropEnabled(true);
            break;
        case PLAYLISTSROOT:
            setPixmap(0, *s_pixRootItem);
            setDropEnabled( true );
            break;
        case INVISIBLEROOT:
            setPixmap(0, *s_pixInvisible);
            break;
        case STALEROOT:
        case STALE:
            setPixmap(0, *s_pixStale);
            break;
        case ORPHANEDROOT:
        case ORPHANED:
            setPixmap(0, *s_pixOrphaned);
            break;
        case DIRECTORY:
            setExpandable( true );
            setDropEnabled( true );
            setPixmap(0, *s_pixDirectory);
            break;
    }
    createToolTip();
}

void
MediaItem::setFailed( bool failed )
{
    if( failed )
    {
        m_flags &= ~MediaItem::Transferring;
        m_flags |= MediaItem::Failed;
        setPixmap(0, *MediaItem::s_pixTransferFailed);
    }
    else
    {
        m_flags &= ~MediaItem::Failed;
        if( m_type == PODCASTITEM )
            setPixmap(0, *s_pixPodcast);
        else if( m_type == PLAYLIST )
            setPixmap(0, *s_pixPlaylist);
        else
            setPixmap(0, QPixmap() );
    }
}

MediaItem *
MediaItem::lastChild() const
{
    Q3ListViewItem *last = 0;
    for( Q3ListViewItem *it = firstChild();
            it;
            it = it->nextSibling() )
    {
        last = it;
    }

    return dynamic_cast<MediaItem *>(last);
}

bool
MediaItem::isLeafItem() const
{
    switch(type())
    {
        case UNKNOWN:
            return false;

        case INVISIBLE:
        case TRACK:
        case PODCASTITEM:
        case PLAYLISTITEM:
        case STALE:
        case ORPHANED:
            return true;

        case ARTIST:
        case ALBUM:
        case PODCASTSROOT:
        case PODCASTCHANNEL:
        case PLAYLISTSROOT:
        case PLAYLIST:
        case INVISIBLEROOT:
        case STALEROOT:
        case ORPHANEDROOT:
        case DIRECTORY:
            return false;
    }

    return false;
}

MediaItem *
MediaItem::findItem( const QString &key, const MediaItem *after ) const
{
    MediaItem *it = 0;
    if( after )
        it = dynamic_cast<MediaItem *>( after->nextSibling() );
    else
        it = dynamic_cast<MediaItem *>( firstChild() );

    for( ; it; it = dynamic_cast<MediaItem *>(it->nextSibling()))
    {
        if(key == it->text(0))
            return it;
        if(key.isEmpty() && it->text(0).isEmpty())
            return it;
    }
    return 0;
}

int
MediaItem::compare( Q3ListViewItem *i, int col, bool ascending ) const
{
    MediaItem *item = dynamic_cast<MediaItem *>(i);
    if(item && col==0 && item->m_order != m_order)
        return m_order-item->m_order;
    else if( item && item->type() == MediaItem::ARTIST )
    {
        QString key1 = key( col, ascending );
        if( key1.startsWith( "the ", Qt::CaseInsensitive ) )
            key1 = key1.mid( 4 );
        QString key2 = i->key( col, ascending );
        if( key2.startsWith( "the ", Qt::CaseInsensitive ) )
            key2 = key2.mid( 4 );

       return key1.localeAwareCompare( key2 );
    }

    return K3ListViewItem::compare(i, col, ascending);
}

void
MediaItem::createToolTip()
{
    QString text;
    switch( type() )
    {
        case MediaItem::TRACK:
            {
                const MetaBundle *b = bundle();
                if( b )
                {
                    if( b->track() )
                        text = QString( "%1 - %2 (%3)" )
                            .arg( QString::number(b->track()), b->title(), b->prettyLength() );
                    if( !b->genre().isEmpty() )
                    {
                        if( !text.isEmpty() )
                            text += "<br>";
                        text += QString( "<i>Genre: %1</i>" )
                            .arg( b->genre() );
                    }
                }
            }
            break;
        case MediaItem::PLAYLISTSROOT:
            text = i18n( "Drag items here to create new playlist" );
            break;
        case MediaItem::PLAYLIST:
            text = i18n( "Drag items here to append to this playlist" );
            break;
        case MediaItem::PLAYLISTITEM:
            text = i18n( "Drag items here to insert before this item" );
            break;
        case MediaItem::INVISIBLEROOT:
        case MediaItem::INVISIBLE:
            text = i18n( "Not visible on media device" );
            break;
        case MediaItem::STALEROOT:
        case MediaItem::STALE:
            text = i18n( "In device database, but file is missing" );
            break;
        case MediaItem::ORPHANEDROOT:
        case MediaItem::ORPHANED:
            text = i18n( "File on device, but not in device database" );
            break;
        default:
            break;
    }

    //if( !text.isEmpty() ) setToolTip( text );
}


