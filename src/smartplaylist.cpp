// (c) Pierpaolo Di Panfilo 2004
// See COPYING file for licensing information

#include "amarok.h"
#include "collectiondb.h"
#include "k3bexporter.h"
#include "playlist.h"
#include "smartplaylist.h"
#include "smartplaylisteditor.h"

#include <qdragobject.h>    //qtextdrag
#include <qheader.h>
#include <qpainter.h>
#include <qpoint.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtextstream.h>    //loadCustomPlaylists()

#include <kguiitem.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmultipledrag.h>    //::dragObject()
#include <kpopupmenu.h>
#include <kurldrag.h>         //::dragObject()
#include <ktoolbar.h>


SmartPlaylistBox::SmartPlaylistBox( QWidget *parent, const char *name )
        : QVBox( parent, name )
{
    KToolBar *toolbar = new KToolBar( this );
    toolbar->setFlat( true );
    toolbar->setMovingEnabled( false );
    toolbar->setIconSize( 16 );

    QObject *view = new SmartPlaylistView( this );

    toolbar->setIconText( KToolBar::IconTextRight, false ); //the "create smart-playlist" will have text on right
    toolbar->insertButton( "filenew", 0, true, i18n("Create Smart-Playlist") );
    toolbar->insertLineSeparator();
    toolbar->setIconText( KToolBar::IconOnly, false ); //the "remove" button will have default appearance
    toolbar->insertButton( "edittrash", 1, true, i18n("Remove") );

    connect( (QObject*)toolbar->getButton( 0 ), SIGNAL(clicked( int )), view, SLOT(createCustomPlaylist()) );
    connect( (QObject*)toolbar->getButton( 1 ), SIGNAL(clicked( int )), view, SLOT(removeSelectedPlaylists()) );
}



/////////////////////////////////////////////////////////////////////////////
//    CLASS SmartPlaylistView
////////////////////////////////////////////////////////////////////////////

SmartPlaylistView::SmartPlaylistView( QWidget *parent, const char *name )
        : KListView( parent, name )
        , m_loaded( false )
{
    addColumn( i18n("Smart-Playlists") );
    setSelectionMode( QListView::Extended );
    setSorting( 0 ); //enable sorting (used for custom smart playlists)
    setFullWidth( true );
    setRootIsDecorated( true );
    setShowSortIndicator( true );

    collectionScanDone(); //load playlist files if appropriate

    connect( CollectionDB::instance(), SIGNAL(scanDone( bool )), SLOT(collectionScanDone()) );

    connect( this, SIGNAL( doubleClicked(QListViewItem*) ), SLOT( loadPlaylistSlot(QListViewItem*) ) );
    connect( this, SIGNAL( rightButtonPressed( QListViewItem *, const QPoint &, int ) ),
                     SLOT( showContextMenu( QListViewItem *, const QPoint &, int ) ) );
}


SmartPlaylistView::~SmartPlaylistView()
{
    QFile file( customPlaylistsFile() );

    if( !file.open( IO_WriteOnly ) )
        return;

    //save all custom smart playlists
    QTextStream stream( &file );

    for( QListViewItemIterator it( this ); it.current(); ++it ) {
        SmartPlaylist *item = static_cast<SmartPlaylist*>(*it);
        if( item->isCustom() ) {
            //save only custom playlists
            stream << "Name=" + item->text( 0 ) << "\n";
            stream << item->sqlForUrls << "\n";
        }
    }
}


void SmartPlaylistView::createCustomPlaylist() //SLOT
{
    //open a dialog to create a custom smart playlist

    if( CollectionDB::instance()->isEmpty() )
        return;

    int counter = 1;
    for( QListViewItemIterator it( this ); it.current(); ++it )
        if( (*it)->text( 0 ).startsWith( i18n("Untitled") ) )
            counter++;

    SmartPlaylistEditor dialog( i18n("Untitled %1").arg( counter ), this );
    if( dialog.exec() == QDialog::Accepted ) {
        SmartPlaylist *item = new SmartPlaylist( dialog.name(), QString(), this );
        item->setCustom( true );
        item->sqlForUrls = dialog.query();
    }
}


void SmartPlaylistView::removeSelectedPlaylists()
{
    QPtrList<QListViewItem> selected = selectedItems();
    for( QListViewItem *item = selected.first(); item; item = selected.next() )
        if( static_cast<SmartPlaylist*>(item )->isCustom() )
            delete item;
}


void SmartPlaylistView::loadDefaultPlaylists()
{
    const QStringList genres  = CollectionDB::instance()->query( "SELECT DISTINCT name FROM genre;" );
    const QStringList artists = CollectionDB::instance()->artistList();
    SmartPlaylist *item;
    QueryBuilder qb;

    // album / artist / genre / title / year / comment / track / bitrate / length / samplerate / path

    /********** All Collection **************/
    qb.initSQLDrag();
    qb.sortBy( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.sortBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTitle );

    item = new SmartPlaylist( i18n( "All Collection" ), qb.query(), this );
    item->setPixmap( 0, SmallIcon("kfm") );
    item->setKey( 1 );

    /********** Favorite Tracks **************/
    qb.initSQLDrag();
    qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPercentage, true );
    qb.setLimit( 0, 15 );

    item = new SmartPlaylist( i18n( "Favorite Tracks" ), qb.query(), this );
    item->setKey( 2 );
    foreach( artists ) {
        qb.initSQLDrag();
        qb.addMatch( QueryBuilder::tabArtist, *it );
        qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPercentage, true );
        qb.setLimit( 0, 15 );

        new SmartPlaylist( i18n( "By %1" ).arg( *it ), qb.query(), item );
    }

    /********** Most Played **************/
    qb.initSQLDrag();
    qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPlayCounter, true );
    qb.setLimit( 0, 15 );

    item = new SmartPlaylist( i18n( "Most Played" ), qb.query(), this );
    item->setKey( 3 );
    foreach( artists ) {
        qb.initSQLDrag();
        qb.addMatch( QueryBuilder::tabArtist, *it );
        qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPlayCounter, true );
        qb.setLimit( 0, 15 );

        new SmartPlaylist( i18n( "By %1" ).arg( *it ), qb.query(), item );
    }

    /********** Newest Tracks **************/
    qb.initSQLDrag();
    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valCreateDate, true );
    qb.setLimit( 0, 15 );

    item = new SmartPlaylist( i18n( "Newest Tracks" ), qb.query(), this );
    item->setKey( 4 );
    foreach( artists ) {
        qb.initSQLDrag();
        qb.addMatch( QueryBuilder::tabArtist, *it );
        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valCreateDate, true );
        qb.setLimit( 0, 15 );

        new SmartPlaylist( i18n( "By %1" ).arg( *it ), qb.query(), item );
    }

    /********** Last Played **************/
    qb.initSQLDrag();
    qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valAccessDate, true );
    qb.setLimit( 0, 15 );

    item = new SmartPlaylist( i18n( "Last Played" ), qb.query(), this );
    item->setKey( 5 );

    /********** Never Played **************/
    item = new SmartPlaylist( i18n( "Never Played" ), QString(), this );
    item->setDragEnabled( true ); //because we pass QString() in the ctor
    item->setKey( 6 );
    item->sqlForUrls =
            "SELECT tags.url "
            "FROM tags, artist "
            "WHERE tags.url NOT IN(SELECT url FROM statistics) AND tags.artist = artist.id "
            "ORDER BY artist.name, tags.title;";

    /********** Genres **************/
    item = new SmartPlaylist( i18n( "Genres" ), QString(), this );
    item->setKey( 7 );
    foreach( genres ) {
        qb.initSQLDrag();
        qb.addMatch( QueryBuilder::tabGenre, *it );
        qb.sortBy( QueryBuilder::tabArtist, QueryBuilder::valName );
        qb.sortBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
        qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTitle );

        new SmartPlaylist( *it, qb.query(), item );
    }

    /********** 100 Random Tracks **************/
    qb.initSQLDrag();
    qb.setOptions( QueryBuilder::optRandomize );
    qb.setLimit( 0, 100 );

    item = new SmartPlaylist( i18n( "100 Random Tracks" ), qb.query(), this );
    item->setKey( 8 );
}


void SmartPlaylistView::loadCustomPlaylists()
{
    QFile file( customPlaylistsFile() );

    if( file.open( IO_ReadOnly ) )
    {
        QTextStream stream( &file );
        QString line, name, query;

        while( !( line = stream.readLine() ).isNull() )
        {
            if( line.startsWith( "Name=" ) )
                name = line.mid( 5 );
            else {
                query = line;
                SmartPlaylist *item = new SmartPlaylist( name, QString(), this );
                item->sqlForUrls = query;
                item->setCustom( true );
            }
        }
    }
}


QString SmartPlaylistView::customPlaylistsFile()
{
    //returns the file used to store custom smart playlists
    return amaroK::saveLocation() + "smartplaylists";
}


class DelayedKURLDrag : public KURLDrag
{
    QString m_sql;
    bool    m_done;

    virtual QByteArray encodedData( const char *mimetype ) const
    {
        if( !m_done ) {
            //TODO cheating the const check like this is hardly sensible
            ((DelayedKURLDrag*)this)->setFileNames( CollectionDB::instance()->query( m_sql ) );
            ((DelayedKURLDrag*)this)->m_done = true;
        }

        return KURLDrag::encodedData( mimetype );
    }

public:
    DelayedKURLDrag( const QString &sql, QWidget *dragsource )
            : KURLDrag( KURL::List(), dragsource )
            , m_sql( sql )
            , m_done( false ) {}
};

QDragObject *SmartPlaylistView::dragObject()
{
    //FIXME currentItem() doesn't seem to work -- strange
    SmartPlaylist *item = (SmartPlaylist*)selectedItems().first();

    //TODO handle all selected items

    KMultipleDrag *drag = new KMultipleDrag( this );

    if( !item->isCustom() && !item->sqlForTags.isEmpty() ) {
        QTextDrag *textdrag = new QTextDrag( item->sqlForTags, 0 );
        textdrag->setSubtype( "amarok-sql" );
        drag->addDragObject( textdrag );
    }

    if( !item->sqlForUrls.isEmpty() )
        drag->addDragObject( new DelayedKURLDrag( item->sqlForUrls, 0 ) );

    return drag;
}


void SmartPlaylistView::paintEmptyArea( QPainter *p, const QRect &r )
{
    if( !childCount() ) {
        p->fillRect( r, colorGroup().base() );
        p->drawText( 10, 10, width()-20, height()-20, Qt::WordBreak,
                i18n("You need to build a collection to use \"Smart Playlists\"") );
    } else
        QListView::paintEmptyArea( p, r );
}


void SmartPlaylistView::loadPlaylistSlot( QListViewItem *item ) //SLOT
{
    if( !item )
        return;

    Playlist::instance()->insertMedia( static_cast<SmartPlaylist*>(item)->urls(), Playlist::Clear );
}


void SmartPlaylistView::showContextMenu( QListViewItem *item, const QPoint &p, int ) //SLOT
{
    #define item static_cast<SmartPlaylist*>(item)

    if( !item ) return;

    enum Id { BURN_DATACD, BURN_AUDIOCD, EDIT, REMOVE };

    KPopupMenu menu( this );
    //TODO menu.insertItem( i18n("Edit"), EDIT );
    menu.insertItem( i18n("Burn to CD as Data"), BURN_DATACD );
    menu.setItemEnabled( BURN_DATACD, K3bExporter::isAvailable() );
    menu.insertItem( i18n("Burn to CD as Audio"), BURN_AUDIOCD );
    menu.setItemEnabled( BURN_AUDIOCD, K3bExporter::isAvailable() );
    if( item->isCustom() ) {
        menu.insertSeparator();
        menu.insertItem( i18n("Remove"), REMOVE );
    }

    switch( menu.exec( p ) ) {
        case BURN_DATACD:
            K3bExporter::instance()->exportTracks( item->urls(), K3bExporter::DataCD );
            break;

        case BURN_AUDIOCD:
            K3bExporter::instance()->exportTracks( item->urls(), K3bExporter::AudioCD );
            break;

        case EDIT:
            break;

        case REMOVE:
            removeSelectedPlaylists();
            break;
    };

    #undef item
}


void SmartPlaylistView::collectionScanDone() //SLOT
{
    if( CollectionDB::instance()->isEmpty() ) {
        clear();
        m_loaded = false;
    }
    else if( !m_loaded ) {
        loadDefaultPlaylists();
        loadCustomPlaylists();
        m_loaded = true;
    }
}


/////////////////////////////////////////////////////////////////////////////
//    CLASS SmartPlaylist
////////////////////////////////////////////////////////////////////////////

SmartPlaylist::SmartPlaylist( const QString &name, const QString &query, KListView *parent )
        : KListViewItem( parent, 0, name )
        , sqlForTags( query )
        , m_custom( false )
{
    setPixmap( 0, SmallIcon( "player_playlist_2" ) );
    setDragEnabled( query.isEmpty() ? false : true );
}

SmartPlaylist::SmartPlaylist( const QString &name, const QString &query, SmartPlaylist *parent )
        : KListViewItem( parent, name )
        , sqlForTags( query )
        , m_custom( false )
{
    setPixmap( 0, SmallIcon( "player_playlist_2" ) );
    setDragEnabled( true );
}

QString
SmartPlaylist::key( int column, bool ) const
{
    //we want to show default playlists above the custom playlists
    return (m_custom || parent()) ? text( column ) : QString( "000000%1" ).arg( m_key );
}

KURL::List
SmartPlaylist::urls() const
{
    KURL url;
    KURL::List urls;
    const QStringList paths = CollectionDB::instance()->query( sqlForUrls );

    foreach( paths ) {
        url.setPath( *it );
        urls += url;
    }

    return urls;
}

#include "smartplaylist.moc"
