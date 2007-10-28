// (c) Pierpaolo Di Panfilo 2004
// (c) 2005 Isaiah Damron <xepo@trifault.net>
// See COPYING file for licensing information

#include "covermanager.h"

#include "amarok.h"
#include "amarokconfig.h"
#include "browserToolBar.h"
#include "debug.h"
#include "querybuilder.h"
#include "config-amarok.h"
#include "pixmapviewer.h"
#include "playlist/PlaylistModel.h"

#include "TheInstances.h"

#include <k3listview.h>
#include <k3multipledrag.h>
#include <k3urldrag.h>
#include <KApplication>
#include <KConfig>
#include <KCursor>
#include <KFileDialog>
#include <KIconLoader>
#include <KIO/NetAccess>
#include <KLineEdit>
#include <KLocale>
#include <KMenu>    //showCoverMenu()
#include <KMessageBox>    //showCoverMenu()
#include <KPushButton>
#include <KSqueezedTextLabel> //status label
#include <KStandardDirs>   //KGlobal::dirs()
#include <KStatusBar>
#include <KToolBar>
#include <KUrl>
#include <KVBox>
#include <KWindowSystem>

#include <QDesktopWidget>  //ctor: desktop size
#include <QFile>
#include <QFontMetrics>    //paintItem()
#include <QImage>
#include <QLabel>
#include <QLayout>
#include <QObject>    //used to delete all cover fetchers
#include <QPainter>    //paintItem()
#include <QPalette>    //paintItem()
#include <QPixmap>
#include <QPoint>
#include <QProgressBar>
#include <q3progressdialog.h>
#include <QRect>
#include <QStringList>
#include <QToolTip>
#include <QTimer>    //search filter timer
//Added by qt3to4:
#include <Q3ValueList>
#include <Q3PtrList>
#include <QDropEvent>
#include <QToolButton>


static QString artistToSelectInInitFunction;
CoverManager *CoverManager::s_instance = 0;

class ArtistItem : public K3ListViewItem
{
    public:
    ArtistItem(Q3ListView *view, Q3ListViewItem *item, const QString &text)
        : K3ListViewItem(view, item, text) {}
    protected:
    int compare( Q3ListViewItem* i, int col, bool ascending ) const
    {
        Q_UNUSED(col);
        Q_UNUSED(ascending);

        QString a = text(0);
        QString b = i->text(0);

        if ( a.startsWith( "the ", Qt::CaseInsensitive ) )
            Amarok::manipulateThe( a, true );
        if ( b.startsWith( "the ", Qt::CaseInsensitive ) )
            Amarok::manipulateThe( b, true );

        return QString::localeAwareCompare( a.toLower(), b.toLower() );
    }
};

CoverManager::CoverManager()
        : QSplitter( 0 )
        , m_timer( new QTimer( this ) )    //search filter timer
        , m_fetchCounter( 0 )
        , m_fetchingCovers( 0 )
        , m_coversFetched( 0 )
        , m_coverErrors( 0 )
{
    DEBUG_BLOCK

    setObjectName( "TheCoverManager" );

    s_instance = this;

    // Sets caption and icon correctly (needed e.g. for GNOME)
    kapp->setTopWidget( this );
    setWindowTitle( KDialog::makeStandardCaption( i18n("Cover Manager") ) );
    setAttribute( Qt::WA_DeleteOnClose );
    setContentsMargins( 4, 4, 4, 4 );

    //artist listview
    m_artistView = new K3ListView( this );
    m_artistView->addColumn(i18n( "Albums By" ));
    m_artistView->setFullWidth( true );
    m_artistView->setSorting( 0 );
    m_artistView->setMinimumWidth( 180 );
    ArtistItem *item = 0;

    //load artists from the collection db
    const QStringList artists = CollectionDB::instance()->artistList( false, false );
    foreach( QString artist, artists )
    {
        item = new ArtistItem( m_artistView, item, artist );
        item->setPixmap( 0, SmallIcon( Amarok::icon( "artist" ) ) );
    }
    m_artistView->sort();

    m_artistView->setSorting( -1 );
    ArtistItem *last = static_cast<ArtistItem *>(m_artistView->lastItem());
    item = new ArtistItem( m_artistView, 0, i18n( "All Albums" ) );
    item->setPixmap( 0, SmallIcon( Amarok::icon( "album" ) ) );

    QueryBuilder qb;
    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.setOptions( QueryBuilder::optOnlyCompilations );
    qb.setLimit( 0, 1 );
    if ( qb.run().count() ) {
        item = new ArtistItem( m_artistView, last, i18n( "Various Artists" ) );
        item->setPixmap( 0, SmallIcon("personal") );
    }

    KVBox *vbox = new KVBox( this );
    KHBox *hbox = new KHBox( vbox );

    vbox->setSpacing( 4 );
    hbox->setSpacing( 4 );

    { //<Search LineEdit>
        KHBox *searchBox = new KHBox( hbox );
        KToolBar* searchToolBar = new Browser::ToolBar( searchBox );
        QToolButton *button = new QToolButton( searchToolBar );
        button->setIcon( KIcon( "edit-clear-locationbar") );
        m_searchEdit = new KLineEdit( searchToolBar );
        m_searchEdit->setClickMessage( i18n( "Enter search terms here" ) );
        m_searchEdit->setFrame( QFrame::Sunken );

        m_searchEdit->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Minimum);
        connect( button, SIGNAL(clicked()), m_searchEdit, SLOT(clear()) );

        button->setToolTip( i18n( "Clear search field" ) );
        m_searchEdit->setToolTip( i18n( "Enter space-separated terms to search in the albums" ) );

        hbox->setStretchFactor( searchBox, 1 );
    } //</Search LineEdit>

    // view menu
    m_viewMenu = new KMenu( this );
    m_selectAllAlbums          = m_viewMenu->addAction( i18n("All Albums"),           this, SLOT( slotShowAllAlbums() ) );
    m_selectAlbumsWithCover    = m_viewMenu->addAction( i18n("Albums With Cover"),    this, SLOT( slotShowAlbumsWithCover() ) );
    m_selectAlbumsWithoutCover = m_viewMenu->addAction( i18n("Albums Without Cover"), this, SLOT( slotShowAlbumsWithoutCover() ) );

    QActionGroup *viewGroup = new QActionGroup( this );
    viewGroup->setExclusive( true );
    viewGroup->addAction( m_selectAllAlbums );
    viewGroup->addAction( m_selectAlbumsWithCover );
    viewGroup->addAction( m_selectAlbumsWithoutCover );
    m_selectAllAlbums->setChecked( true );

    // amazon locale menu
    QString locale = AmarokConfig::amazonLocale();
    m_currentLocale = CoverFetcher::localeStringToID( locale );
    
    QAction *a;
    QActionGroup *localeGroup = new QActionGroup( this );
    localeGroup->setExclusive( true );

    m_amazonLocaleMenu = new KMenu( this );
    
    a = m_amazonLocaleMenu->addAction( i18n("International"),  this, SLOT( slotSetLocaleIntl() ) );
    if( m_currentLocale == CoverFetcher::International ) a->setChecked( true );
    localeGroup->addAction( a );

    a = m_amazonLocaleMenu->addAction( i18n("Canada"),         this, SLOT( slotSetLocaleCa() )   );
    if( m_currentLocale == CoverFetcher::Canada ) a->setChecked( true );
    localeGroup->addAction( a );
    
    a = m_amazonLocaleMenu->addAction( i18n("France"),         this, SLOT( slotSetLocaleFr() )   );
    if( m_currentLocale == CoverFetcher::France ) a->setChecked( true );
    localeGroup->addAction( a );
    
    a = m_amazonLocaleMenu->addAction( i18n("Germany"),        this, SLOT( slotSetLocaleDe() )   );
    if( m_currentLocale == CoverFetcher::Germany ) a->setChecked( true );
    localeGroup->addAction( a );
    
    a = m_amazonLocaleMenu->addAction( i18n("Japan"),          this, SLOT( slotSetLocaleJp() )   );
    if( m_currentLocale == CoverFetcher::Japan ) a->setChecked( true );
    localeGroup->addAction( a );
    
    a = m_amazonLocaleMenu->addAction( i18n("United Kingdom"), this, SLOT( slotSetLocaleUk() )   );
    if( m_currentLocale == CoverFetcher::UK ) a->setChecked( true );
    localeGroup->addAction( a );

    KToolBar* toolBar = new KToolBar( hbox );
    toolBar->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
//TODO port to kde4
//    toolBar->setFrameShape( QFrame::NoFrame );
    {
        QAction* viewMenuAction = new QAction( KIcon( "view_choose" ), i18nc( "@title buttontext for popup-menu", "View" ), this );
        viewMenuAction->setMenu( m_viewMenu );
        toolBar->addAction( viewMenuAction );
    }
    {
        QAction* localeMenuAction = new QAction( KIcon( "babelfish" ),  i18n( "Amazon Locale" ), this );
        localeMenuAction->setMenu( m_amazonLocaleMenu );
        toolBar->addAction( localeMenuAction );
    }

    //fetch missing covers button
    m_fetchButton = new KPushButton( KGuiItem( i18n("Fetch Missing Covers"), Amarok::icon( "download" ) ), hbox );
    connect( m_fetchButton, SIGNAL(clicked()), SLOT(fetchMissingCovers()) );


    //cover view
    m_coverView = new CoverView( vbox );

    //status bar
    KStatusBar *m_statusBar = new KStatusBar( vbox );

    m_statusLabel = new KSqueezedTextLabel( m_statusBar );
    m_statusLabel->setIndent( 3 );
    m_progressBox = new KHBox( m_statusBar );
    
    m_statusBar->addWidget( m_statusLabel, 4 );
    m_statusBar->addPermanentWidget( m_progressBox, 1 );

    KPushButton *stopButton = new KPushButton( KGuiItem(i18n("Abort"), "stop"), m_progressBox );
    connect( stopButton, SIGNAL(clicked()), SLOT(stopFetching()) );
    m_progress = new QProgressBar( m_progressBox );
    m_progress->setTextVisible( true );

    const int h = m_statusLabel->height() + 3;
    m_statusLabel->setFixedHeight( h );
    m_progressBox->setFixedHeight( h );
    m_progressBox->hide();


    // signals and slots connections
    connect( m_artistView, SIGNAL(selectionChanged( Q3ListViewItem* ) ),
                           SLOT(slotArtistSelected( Q3ListViewItem* )) );
    connect( m_coverView,  SIGNAL(contextMenuRequested( Q3IconViewItem*, const QPoint& )),
                           SLOT(showCoverMenu( Q3IconViewItem*, const QPoint& )) );
    connect( m_coverView,  SIGNAL(executed( Q3IconViewItem* )),
                           SLOT(coverItemExecuted( Q3IconViewItem* )) );
    connect( m_timer,      SIGNAL(timeout()),
                           SLOT(slotSetFilter()) );
    connect( m_searchEdit, SIGNAL(textChanged( const QString& )),
                           SLOT(slotSetFilterTimeout()) );

    connect( CollectionDB::instance(), SIGNAL(coverFetched( const QString&, const QString& )),
                                       SLOT(coverFetched( const QString&, const QString& )) );
    connect( CollectionDB::instance(), SIGNAL(coverRemoved( const QString&, const QString& )),
                                       SLOT(coverRemoved( const QString&, const QString& )) );
    connect( CollectionDB::instance(), SIGNAL(coverFetcherError( const QString& )),
                                       SLOT(coverFetcherError()) );

    m_currentView = AllAlbums;

    QSize size = QApplication::desktop()->screenGeometry( this ).size() / 1.5;
    QSize sz = Amarok::config( "Cover Manager" ).readEntry( "Window Size", size );
    resize( sz.width(), sz.height() );

    show();

    QTimer::singleShot( 0, this, SLOT(init()) );
}


CoverManager::~CoverManager()
{
    DEBUG_BLOCK

    Amarok::config( "Cover Manager" ).writeEntry( "Window Size", size() );

    s_instance = 0;
}


void CoverManager::init()
{
    DEBUG_BLOCK

    Q3ListViewItem *item = 0;

    if ( !artistToSelectInInitFunction.isEmpty() )
        for ( item = m_artistView->firstChild(); item; item = item->nextSibling() )
            if ( item->text( 0 ) == artistToSelectInInitFunction )
                break;

    if ( item == 0 )
        item = m_artistView->firstChild();

    m_artistView->setSelected( item, true );
}


CoverViewDialog::CoverViewDialog( const QString& artist, const QString& album, QWidget *parent )
    : QDialog( parent, 0, false, Qt::WType_TopLevel | Qt::WNoAutoErase )
    , m_pixmap( CollectionDB::instance()->albumImage( artist, album, false, 0 ) )
{
    setAttribute( Qt::WA_DeleteOnClose );
#ifdef Q_WS_X11
    KWindowSystem::setType( winId(), NET::Utility );
#endif
    kapp->setTopWidget( this );
    setWindowTitle( KDialog::makeStandardCaption( i18n("%1 - %2", artist, album ) ) );

    m_layout = new QHBoxLayout( this );
    m_pixmapViewer = new PixmapViewer( this, m_pixmap );
    m_layout->addWidget( m_pixmapViewer );

    setFixedSize( m_pixmapViewer->maximalSize() );
}


void CoverManager::viewCover( const QString& artist, const QString& album, QWidget *parent ) //static
{
    //QDialog means "escape" works as expected
    QDialog *dialog = new CoverViewDialog( artist, album, parent );
    dialog->show();
}


QString CoverManager::amazonTld() //static
{
    if( AmarokConfig::amazonLocale() == "us" )
        return "com";
    else if( AmarokConfig::amazonLocale()== "jp" )
        return "co.jp";
    else if( AmarokConfig::amazonLocale() == "uk" )
        return "co.uk";
    else if( AmarokConfig::amazonLocale() == "ca" )
        return "ca";
    else
        return AmarokConfig::amazonLocale();
}


void CoverManager::fetchMissingCovers() //SLOT
{
    DEBUG_BLOCK

    for ( Q3IconViewItem *item = m_coverView->firstItem(); item; item = item->nextItem() ) {
        CoverViewItem *coverItem = static_cast<CoverViewItem*>( item );
        if( !coverItem->hasCover() ) {
            m_fetchCovers += coverItem->artist() + " @@@ " + coverItem->album();
            m_fetchingCovers++;
        }
    }

    if( !m_fetchCounter )    //loop isn't started yet
        fetchCoversLoop();

    updateStatusBar();
    m_fetchButton->setEnabled( false );

}


void CoverManager::fetchCoversLoop() //SLOT
{
    if( (int)m_fetchCounter < m_fetchCovers.count() )
    {
        //get artist and album from keyword
        const QStringList values = m_fetchCovers[m_fetchCounter].split( " @@@ ", QString::KeepEmptyParts );

        if( values.count() > 1 )
           CollectionDB::instance()->fetchCover( this, values[0], values[1], m_fetchCovers.count() != 1); //edit mode when fetching 1 cover

        m_fetchCounter++;

        // Wait 1 second, since amazon caps the number of accesses per client
        QTimer::singleShot( 1000, this, SLOT( fetchCoversLoop() ) );
    }
    else {
        m_fetchCovers.clear();
        m_fetchCounter = 0;
    }

}


void CoverManager::showOnce( const QString &artist )
{
    if ( !s_instance ) {
        artistToSelectInInitFunction = artist;
        new CoverManager(); //shows itself
    }
    else {
        s_instance->activateWindow();
        s_instance->raise();
    }
}

void CoverManager::slotArtistSelected( Q3ListViewItem *item ) //SLOT
{
    if( item->depth() ) //album item
        return;

    QString artist = item->text(0);

    if( artist.endsWith( ", The" ) )
       Amarok::manipulateThe( artist, false );

    m_coverView->clear();
    m_coverItems.clear();

    // reset current view mode state to "AllAlbum" which is the default on artist change in left panel
    m_currentView = AllAlbums;
    m_selectAllAlbums->trigger();
    m_selectAllAlbums->setChecked( true );

    Q3ProgressDialog progress( this, 0, true );
    progress.setLabelText( i18n("Loading Thumbnails...") );
    progress.QDialog::setWindowTitle( i18n("...") );

    //NOTE we MUST show the dialog, otherwise the closeEvents get processed
    // in the processEvents() calls below, GRUMBLE! Qt sux0rs
    progress.show();
    progress.repaint();  //ensures the dialog isn't blank

    //this is an extra processEvent call for the sake of init() and aesthetics
    //it isn't necessary
    kapp->processEvents();

    //this can be a bit slow
    QApplication::setOverrideCursor( Qt::WaitCursor );
    QueryBuilder qb;
    QStringList albums;

    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabAlbum,  QueryBuilder::valName );

    qb.excludeMatch( QueryBuilder::tabAlbum, i18n( "Unknown" ) );
    qb.sortBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    qb.setOptions( QueryBuilder::optNoCompilations );

    if ( item != m_artistView->firstChild() )
        qb.addMatch( QueryBuilder::tabArtist, artist );

    albums = qb.run();

    //also retrieve compilations when we're showing all items (first treenode) or
    //"Various Artists" (last treenode)
    if ( item == m_artistView->firstChild() || item == m_artistView->lastChild() )
    {
        QStringList cl;

        qb.clear();
        qb.addReturnValue( QueryBuilder::tabAlbum,  QueryBuilder::valName );

        qb.excludeMatch( QueryBuilder::tabAlbum, i18n( "Unknown" ) );
        qb.sortBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
        qb.setOptions( QueryBuilder::optRemoveDuplicates );
        qb.setOptions( QueryBuilder::optOnlyCompilations );
        cl = qb.run();

        for( int i = 0; i < cl.count(); i++ ) {
            albums.append( i18n( "Various Artists" ) );
            albums.append( cl[ i ] );
        }
    }

    QApplication::restoreOverrideCursor();

    progress.setTotalSteps( (albums.count()/2) + (albums.count()/10) );

    //insert the covers first because the list view is soooo paint-happy
    //doing it in the second loop looks really bad, unfortunately
    //this is the slowest step in the bit that we can't process events
    uint x = 0;
    oldForeach( albums )
    {
        const QString artist = *it;
        const QString album = *(++it);
        m_coverItems.append( new CoverViewItem( m_coverView, m_coverView->lastItem(), artist, album ) );

        if ( ++x % 50 == 0 ) {
            progress.setProgress( x / 5 ); // we do it less often due to bug in Qt, ask Max
            kapp->processEvents(); // QProgressDialog also calls this, but not always due to Qt bug!

            //only worth testing for after processEvents() is called
            if( progress.wasCancelled() )
               break;
        }
    }

    //now, load the thumbnails
    for( Q3IconViewItem *item = m_coverView->firstItem(); item; item = item->nextItem() ) {
        progress.setProgress( progress.progress() + 1 );
        kapp->processEvents();

        if( progress.wasCancelled() )
           break;

        static_cast<CoverViewItem*>(item)->loadCover();
    }

    updateStatusBar();
}

void CoverManager::showCoverMenu( Q3IconViewItem *item, const QPoint &p ) //SLOT
{
    #define item static_cast<CoverViewItem*>(item)
    if( !item ) return;

    KMenu menu;

    menu.addTitle( i18n( "Cover Image" ) );

    Q3PtrList<CoverViewItem> selected = selectedItems();
    const int nSelected = selected.count();

    QAction* fetchSelectedAction = new QAction( KIcon( Amarok::icon( "download" ) )
        , ( nSelected == 1 ? i18n( "&Fetch From amazon.%1", CoverManager::amazonTld() )
                           : i18n( "&Fetch Selected Covers" ) )
        , &menu );
    connect( fetchSelectedAction, SIGNAL( triggered() ), this, SLOT( fetchSelectedCovers() ) );

    QAction* setCustomAction = new QAction( KIcon( Amarok::icon( "files" ) )
        , i18np( "Set &Custom Cover", "Set &Custom Cover for Selected Albums", nSelected )
        , &menu );
    connect( setCustomAction, SIGNAL( triggered() ), this, SLOT( setCustomSelectedCovers() ) );
    QAction* unsetAction = new QAction( KIcon( Amarok::icon( "remove" ) ), i18np( "&Unset Cover", "&Unset Selected Covers", nSelected ), &menu );
    connect( unsetAction, SIGNAL( triggered() ), this, SLOT ( deleteSelectedCovers() ) );

    QAction* playAlbumAction = new QAction( KIcon( Amarok::icon( "add_playlist" ) )
        , i18np( "&Append to Playlist", "&Append Selected Albums to Playlist", nSelected )
        , &menu );

    connect( playAlbumAction, SIGNAL( triggered() ), this, SLOT( playSelectedAlbums() ) );

    if( nSelected > 1 ) {
        menu.addAction( fetchSelectedAction );
        menu.addAction( setCustomAction );
        menu.addAction( playAlbumAction );
        menu.addAction( unsetAction );
    }
    else {
        QAction* viewAction = new QAction( KIcon( Amarok::icon( "zoom" ) ), i18n( "&Show Fullsize" ), &menu );
        connect( viewAction, SIGNAL( triggered() ), this, SLOT( viewSelectedCover() ) );
        viewAction ->setEnabled( item->hasCover() );
        unsetAction->setEnabled( item->canRemoveCover() );
        menu.addAction( viewAction );
        menu.addAction( fetchSelectedAction );
        menu.addAction( setCustomAction );
        menu.addAction( playAlbumAction );
        menu.addSeparator();

        menu.addAction( unsetAction );
    }

    menu.exec( p );

    #undef item
}

void CoverManager::viewSelectedCover()
{
    CoverViewItem* item = selectedItems().first();
    viewCover( item->artist(), item->album(), this );
}

void CoverManager::coverItemExecuted( Q3IconViewItem *item ) //SLOT
{
    #define item static_cast<CoverViewItem*>(item)

    if( !item ) return;

    item->setSelected( true );
    if ( item->hasCover() )
        viewCover( item->artist(), item->album(), this );
    else
        fetchSelectedCovers();

    #undef item
}


void CoverManager::slotSetFilter() //SLOT
{
    m_filter = m_searchEdit->text();

    m_coverView->selectAll( false);
    Q3IconViewItem *item = m_coverView->firstItem();
    while ( item )
    {
        Q3IconViewItem *tmp = item->nextItem();
        m_coverView->takeItem( item );
        item = tmp;
    }

    m_coverView->setAutoArrange( false );
    for( Q3IconViewItem *item = m_coverItems.first(); item; item = m_coverItems.next() )
    {
        CoverViewItem *coverItem = static_cast<CoverViewItem*>(item);
        if( coverItem->album().contains( m_filter, Qt::CaseInsensitive ) || coverItem->artist().contains( m_filter, Qt::CaseInsensitive ) )
            m_coverView->insertItem( item, m_coverView->lastItem() );
    }
    m_coverView->setAutoArrange( true );

    m_coverView->arrangeItemsInGrid();
    updateStatusBar();
}


void CoverManager::slotSetFilterTimeout() //SLOT
{
    if ( m_timer->isActive() ) m_timer->stop();
    m_timer->setSingleShot( true );
    m_timer->start( 180 );
}

void CoverManager::changeView( int id  ) //SLOT
{
    if( m_currentView == id ) return;

    //clear the iconview without deleting items
    m_coverView->selectAll( false);
    Q3IconViewItem *item = m_coverView->firstItem();
    while ( item ) {
        Q3IconViewItem *tmp = item->nextItem();
        m_coverView->takeItem( item );
        item = tmp;
    }

    m_coverView->setAutoArrange(false );
    for( Q3IconViewItem *item = m_coverItems.first(); item; item = m_coverItems.next() ) {
        bool show = false;
        CoverViewItem *coverItem = static_cast<CoverViewItem*>(item);
        if( !m_filter.isEmpty() ) {
            if( !coverItem->album().contains( m_filter, Qt::CaseInsensitive ) && !coverItem->artist().contains( m_filter, Qt::CaseInsensitive ) )
                continue;
        }

        if( id == AllAlbums )    //show all albums
            show = true;
        else if( id == AlbumsWithCover && coverItem->hasCover() )    //show only albums with cover
            show = true;
        else if( id == AlbumsWithoutCover && !coverItem->hasCover() )   //show only albums without cover
            show = true;

        if( show )    m_coverView->insertItem( item, m_coverView->lastItem() );
    }
    m_coverView->setAutoArrange( true );

    m_coverView->arrangeItemsInGrid();
    m_currentView = id;
}

void CoverManager::changeLocale( int id ) //SLOT
{
    QString locale = CoverFetcher::localeIDToString( id );
    AmarokConfig::setAmazonLocale( locale );
    m_currentLocale = id;
}


void CoverManager::coverFetched( const QString &artist, const QString &album ) //SLOT
{
    loadCover( artist, album );
    m_coversFetched++;
    updateStatusBar();
}


void CoverManager::coverRemoved( const QString &artist, const QString &album ) //SLOT
{
    loadCover( artist, album );
    m_coversFetched--;
    updateStatusBar();
}


void CoverManager::coverFetcherError()
{
    DEBUG_FUNC_INFO

    m_coverErrors++;
    updateStatusBar();
}


void CoverManager::stopFetching()
{
    Debug::Block block( __PRETTY_FUNCTION__ );

    m_fetchCovers.clear();
    m_fetchCounter = 0;

    //delete all cover fetchers
    QList<CoverFetcher *> list = qFindChildren<CoverFetcher*>(this);
    foreach( CoverFetcher *cf, list )
        cf->deleteLater();

    m_fetchingCovers = 0;
    updateStatusBar();
}

// PRIVATE

void CoverManager::loadCover( const QString &artist, const QString &album )
{
    for( Q3IconViewItem *item = m_coverItems.first(); item; item = m_coverItems.next() )
    {
        CoverViewItem *coverItem = static_cast<CoverViewItem*>(item);
        if ( album == coverItem->album() && ( artist == coverItem->artist() || ( artist.isEmpty() && coverItem->artist().isEmpty() ) ) )
        {
            coverItem->loadCover();
            return;
        }
    }
}

void CoverManager::setCustomSelectedCovers()
{
    //function assumes something is selected
    Q3PtrList<CoverViewItem> selected = selectedItems();
    CoverViewItem* first = selected.getFirst();

    QString artist_id; artist_id.setNum( CollectionDB::instance()->artistID( first->artist() ) );
    QString album_id; album_id.setNum( CollectionDB::instance()->albumID( first->album() ) );
    QStringList values = CollectionDB::instance()->albumTracks( artist_id, album_id );

    QString startPath = ":homedir";
    if ( !values.isEmpty() ) {
        KUrl url;
        url.setPath( values.first() );
        startPath = url.directory();
    }
    KUrl file = KFileDialog::getImageOpenUrl( startPath, this, i18n( "Select Cover Image File" ) );
    if ( !file.isEmpty() ) {
        kapp->processEvents();    //it may takes a while so process pending events
        QString tmpFile;
        QImage image = CollectionDB::fetchImage(file, tmpFile);
        for ( CoverViewItem* item = selected.first(); item; item = selected.next() ) {
            CollectionDB::instance()->setAlbumImage( item->artist(), item->album(), image );
            item->loadCover();
        }
        KIO::NetAccess::removeTempFile( tmpFile );
    }
}

void CoverManager::fetchSelectedCovers()
{
    Q3PtrList<CoverViewItem> selected = selectedItems();
    for ( CoverViewItem* item = selected.first(); item; item = selected.next() )
        m_fetchCovers += item->artist() + " @@@ " + item->album();

    m_fetchingCovers += selected.count();

    if( !m_fetchCounter )    //loop isn't started yet
        fetchCoversLoop();

    updateStatusBar();
}


void CoverManager::deleteSelectedCovers()
{
    Q3PtrList<CoverViewItem> selected = selectedItems();

    int button = KMessageBox::warningContinueCancel( this,
                            i18np( "Are you sure you want to remove this cover from the Collection?",
                                  "Are you sure you want to delete these %1 covers from the Collection?",
                                  selected.count() ),
                            QString(),
                            KStandardGuiItem::del() );

    if ( button == KMessageBox::Continue ) {
        for ( CoverViewItem* item = selected.first(); item; item = selected.next() ) {
            kapp->processEvents();
            if ( CollectionDB::instance()->removeAlbumImage( item->artist(), item->album() ) )    //delete selected cover
                  coverRemoved( item->artist(), item->album() );
        }
    }
}


void CoverManager::playSelectedAlbums()
{
    Q3PtrList<CoverViewItem> selected = selectedItems();
    QString artist_id, album_id;
    for ( CoverViewItem* item = selected.first(); item; item = selected.next() )
    {
        artist_id.setNum( CollectionDB::instance()->artistID( item->artist() ) );
        album_id.setNum( CollectionDB::instance()->albumID( item->album() ) );
        The::playlistModel()->insertMedia( CollectionDB::instance()->albumTracks( artist_id, album_id ), Playlist::Append );
    }
}

Q3PtrList<CoverViewItem> CoverManager::selectedItems()
{
    Q3PtrList<CoverViewItem> selectedItems;
    for ( Q3IconViewItem* item = m_coverView->firstItem(); item; item = item->nextItem() )
        if ( item->isSelected() )
              selectedItems.append( static_cast<CoverViewItem*>(item) );

    return selectedItems;
}


void CoverManager::updateStatusBar()
{
    QString text;

    //cover fetching info
    if( m_fetchingCovers ) {
        //update the progress bar
        m_progress->setMaximum( m_fetchingCovers );
        m_progress->setValue( m_coversFetched + m_coverErrors );
        if( m_progressBox->isHidden() )
            m_progressBox->show();

        //update the status text
        if( m_coversFetched + m_coverErrors >= m_progress->value() ) {
            //fetching finished
            text = i18n( "Finished." );
            if( m_coverErrors )
                text += i18np( " Cover not found", " <b>%1</b> covers not found", m_coverErrors );
            //reset counters
            m_fetchingCovers = 0;
            m_coversFetched = 0;
            m_coverErrors = 0;
            QTimer::singleShot( 2000, this, SLOT( updateStatusBar() ) );
        }

        if( m_fetchingCovers == 1 ) {
            QStringList values = m_fetchCovers[0].split( " @@@ ", QString::KeepEmptyParts ); //get artist and album name
            if ( values.count() >= 2 )
            {
                if( values[0].isEmpty() )
                    text = i18n( "Fetching cover for %1..." , values[1] );
                else
                    text = i18n( "Fetching cover for %1 - %2...", values[0], values[1] );
            }
        }
        else if( m_fetchingCovers ) {
            text = i18np( "Fetching 1 cover: ", "Fetching <b>%1</b> covers... : ", m_fetchingCovers );
            if( m_coversFetched )
                text += i18np( "1 fetched", "%1 fetched", m_coversFetched );
            if( m_coverErrors ) {
            if( m_coversFetched ) text += i18n(" - ");
                text += i18np( "1 not found", "%1 not found", m_coverErrors );
            }
            if( m_coversFetched + m_coverErrors == 0 )
                text += i18n( "Connecting..." );
        }
    }
    else {
        m_coversFetched = 0;
        m_coverErrors = 0;

        uint totalCounter = 0, missingCounter = 0;

        if( m_progressBox->isVisible() )
            m_progressBox->hide();

        //album info
        for( Q3IconViewItem *item = m_coverView->firstItem(); item; item = item->nextItem() ) {
            totalCounter++;
            if( !static_cast<CoverViewItem*>( item )->hasCover() )
                missingCounter++;    //counter for albums without cover
        }

        if( !m_filter.isEmpty() )
            text = i18np( "1 result for \"%2\"", "%1 results for \"%2\"", totalCounter, m_filter );
        else if( m_artistView->selectedItem() ) {
            text = i18np( "1 album", "%1 albums", totalCounter );
            if( m_artistView->selectedItem() != m_artistView->firstChild() ) //showing albums by an artist
            {
                QString artist = m_artistView->selectedItem()->text(0);
                if( artist.endsWith( ", The" ) )
                    Amarok::manipulateThe( artist, false );
                text += i18n( " by " ) + artist;
            }
        }

        if( missingCounter )
            text += i18n(" - ( <b>%1</b> without cover )", missingCounter );

        m_fetchButton->setEnabled( missingCounter );
    }

    m_statusLabel->setText( text );
}

void CoverManager::setStatusText( QString text )
{
    m_oldStatusText = m_statusLabel->text();
    m_statusLabel->setText( text );
}

//////////////////////////////////////////////////////////////////////
//    CLASS CoverView
/////////////////////////////////////////////////////////////////////

CoverView::CoverView( QWidget *parent, const char *name, Qt::WFlags f )
    : K3IconView( parent, name, f )
{
    Debug::Block block( __PRETTY_FUNCTION__ );

    setArrangement( Q3IconView::LeftToRight );
    setResizeMode( Q3IconView::Adjust );
    setSelectionMode( Q3IconView::Extended );
    arrangeItemsInGrid();
    setAutoArrange( true );
    setItemsMovable( false );

    // as long as QIconView only shows tooltips when the cursor is over the
    // icon (and not the text), we have to create our own tooltips
    setShowToolTips( false );

    connect( this, SIGNAL( onItem( Q3IconViewItem * ) ), SLOT( setStatusText( Q3IconViewItem * ) ) );
    connect( this, SIGNAL( onViewport() ), CoverManager::instance(), SLOT( updateStatusBar() ) );
}


Q3DragObject *CoverView::dragObject()
{
    CoverViewItem *item = static_cast<CoverViewItem*>( currentItem() );
    if( !item )
       return 0;

    const QString sql = "SELECT tags.url FROM tags, album WHERE album.name %1 AND tags.album = album.id ORDER BY tags.track;";
    const QStringList values = CollectionDB::instance()->query( sql.arg( CollectionDB::likeCondition( item->album() ) ) );

    KUrl::List urls;
    for( QStringList::ConstIterator it = values.begin(), end = values.end(); it != end; ++it )
        urls += *it;

    QString imagePath = CollectionDB::instance()->albumImage( item->artist(), item->album(), false, 1 );
    K3MultipleDrag *drag = new K3MultipleDrag( this );
    drag->setPixmap( item->coverPixmap() );
    drag->addDragObject( new Q3IconDrag( this ) );
    drag->addDragObject( new Q3ImageDrag( QImage( imagePath ) ) );
    drag->addDragObject( new K3URLDrag( urls ) );

    return drag;
}

void CoverView::setStatusText( Q3IconViewItem *item )
{
    #define item static_cast<CoverViewItem *>( item )
    if ( !item )
        return;

    bool sampler = false;
    //compilations have valDummy for artist.  see QueryBuilder::addReturnValue(..) for explanation
    //FIXME: Don't rely on other independent code, use an sql query
    if( item->artist().isEmpty() ) sampler = true;

    QString tipContent = i18n( "%1 - %2", sampler ? i18n("Various Artists") : item->artist() , item->album() );

    CoverManager::instance()->setStatusText( tipContent );

    #undef item
}

//////////////////////////////////////////////////////////////////////
//    CLASS CoverViewItem
/////////////////////////////////////////////////////////////////////

CoverViewItem::CoverViewItem( Q3IconView *parent, Q3IconViewItem *after, const QString &artist, const QString &album )
    : K3IconViewItem( parent, after, album )
    , m_artist( artist )
    , m_album( album )
    , m_coverImagePath( CollectionDB::instance()->albumImage( m_artist, m_album, false, 0, &m_embedded ) )
    , m_coverPixmap( )
{
    setDragEnabled( true );
    setDropEnabled( true );
    calcRect();
}

bool CoverViewItem::hasCover() const
{
    return !m_coverImagePath.endsWith( "nocover.png" ) && QFile::exists( m_coverImagePath );
}

void CoverViewItem::loadCover()
{
    m_coverImagePath = CollectionDB::instance()->albumImage( m_artist, m_album, false, 1, &m_embedded );
    m_coverPixmap = QPixmap( m_coverImagePath );  //create the scaled cover

    repaint();
}


void CoverViewItem::calcRect( const QString& )
{
    int thumbWidth = AmarokConfig::coverPreviewSize();

    QFontMetrics fm = iconView()->fontMetrics();
    QRect itemPixmapRect( 5, 1, thumbWidth, thumbWidth );
    QRect itemRect = rect();
    itemRect.setWidth( thumbWidth + 10 );
    itemRect.setHeight( thumbWidth + fm.lineSpacing() + 2 );
    QRect itemTextRect( 0, thumbWidth+2, itemRect.width(), fm.lineSpacing() );

    setPixmapRect( itemPixmapRect );
    setTextRect( itemTextRect );
    setItemRect( itemRect );
}


void CoverViewItem::paintItem(QPainter* p, const QColorGroup& cg)
{
    Q_UNUSED( cg ); // FIXME is this correct (cg replaced by palette() )?
    QPalette palette = KApplication::palette();
    QRect itemRect = rect();

    p->save();
    p->translate( itemRect.x(), itemRect.y() );

    // draw the border
    p->setPen( palette.mid().color() );
    p->drawRect( 0, 0, itemRect.width(), pixmapRect().height()+2 );

    // draw the cover image
    if( !m_coverPixmap.isNull() )
        p->drawPixmap( pixmapRect().x() + (pixmapRect().width() - m_coverPixmap.width())/2,
            pixmapRect().y() + (pixmapRect().height() - m_coverPixmap.height())/2, m_coverPixmap );

    //justify the album name
    QString str = text();
    QFontMetrics fm = p->fontMetrics();
    str = fm.elidedText( str, Qt::ElideRight, textRect().width() );

    p->setPen( palette.text().color() );
    p->drawText( textRect(), Qt::AlignCenter, str );

    if( isSelected() ) {
       p->setPen( palette.highlight().color() );
       p->drawRect( pixmapRect() );
       p->drawRect( pixmapRect().left()+1, pixmapRect().top()+1, pixmapRect().width()-2, pixmapRect().height()-2);
       p->drawRect( pixmapRect().left()+2, pixmapRect().top()+2, pixmapRect().width()-4, pixmapRect().height()-4);
    }

    p->restore();
}


void CoverViewItem::dropped( QDropEvent *e, const Q3ValueList<Q3IconDragItem> & )
{
    if( Q3ImageDrag::canDecode( e ) ) {
       if( hasCover() ) {
           KGuiItem continueButton = KStandardGuiItem::cont();
           continueButton.setText( i18n("&Overwrite") );
           int button = KMessageBox::warningContinueCancel( iconView(),
                            i18n( "Are you sure you want to overwrite this cover?"),
                            i18n("Overwrite Confirmation"),
                            continueButton );
           if( button == KMessageBox::Cancel )
               return;
       }

       QImage img;
       Q3ImageDrag::decode( e, img );
       CollectionDB::instance()->setAlbumImage( artist(), album(), img );
       m_coverImagePath = CollectionDB::instance()->albumImage( m_artist, m_album, false, 0 );
       loadCover();
    }
}


void CoverViewItem::dragEntered()
{
    setSelected( true );
}


void CoverViewItem::dragLeft()
{
    setSelected( false );
}

#include "covermanager.moc"
