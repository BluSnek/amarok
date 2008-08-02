/***************************************************************************
* copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>         *
* copyright            : (C) 2008 Mark Kretschmann <kretschmann@kde.org>   *
* copyright            : (C) 2008 William Viana Soares <vianasw@gmail.com> *
****************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "Amarok.h"
#include "ColumnContainment.h"
#include "ContextScene.h"
#include "Debug.h"
#include "MainWindow.h"
#include "Svg.h"
#include "SvgTinter.h"
#include "WidgetBackgroundPainter.h"

#include "plasma/theme.h"

#include <KAuthorized>
#include <KMenu>
#include <KTemporaryFile>
#include <KSvgRenderer>
#include <KGlobalSettings>
#include <KIcon>

#include <threadweaver/ThreadWeaver.h>
#include <threadweaver/Job.h>

#include <QAction>
#include <QApplication>
#include <QDesktopWidget>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QThread>
#include <QTimeLine>

#include <limits.h>


namespace Context
{

void SvgRenderJob::run()
{
    KSvgRenderer renderer( m_file );
    m_image = QImage(  QApplication::desktop()->screenGeometry( QApplication::desktop()->primaryScreen() ).size(), QImage::Format_ARGB32 );
    QPainter painter ( &m_image );
    renderer.render( &painter );
}

ColumnContainment::ColumnContainment( QObject *parent, const QVariantList &args )
    : Context::Containment( parent, args )
    , m_actions( 0 )
    , m_minColumnWidth( 370 )
    , m_maxColumnWidth( 500 )
    , m_defaultRowHeight( 150 )
    , m_paintTitle( 0 )
    , m_manageCurrentTrack( 0 )
    , m_appletsFromConfigCount( 0 )
    , m_toolBox( 0 )
{
    DEBUG_BLOCK

    setContainmentType( CustomContainment );
    
    m_grid = new QGraphicsGridLayout();
    setLayout( m_grid );
    m_grid->setSpacing( 3 );
    m_grid->setContentsMargins( 0, 0, 0, 0 );
    setContentsMargins( 20, 60, 0, 0 );

    setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored));
    for( int i = 0; i < MAX_ROWS; i++ )
        for( int j = 0; j < MAX_COLUMNS; j++ )
            m_gridFreePositions[i][j] = true;
        
    loadInitialConfig();
    
    m_header = new Context::Svg( this );
    m_header->setImagePath( "widgets/amarok-containment-header" );
    m_header->setContainsMultipleImages( false );

    m_header->resize();

    m_title = new QGraphicsSimpleTextItem( this );

    m_title->hide();
    QFont labelFont;
//     labelFont.setBold( true );
    labelFont.setPointSize( labelFont.pointSize() + 5 );
    m_title->setFont( labelFont );

    DEBUG_LINE_INFO

    //HACK alert!

    //m_job = new SvgRenderJob( file.fileName() );
    //the background is not really important for the use of Amarok,
    //and running this in a thread makes startup quite a bit faster
    //consider only starting the thread if QThread::idealThreadCount > 1
    //if the background *has* to be there in time most of the time
    //connect( m_job, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( jobDone() ) );
    //ThreadWeaver::Weaver::instance()->enqueue( m_job );

    DEBUG_LINE_INFO
    //m_background = new Svg( m_tintedSvg.fileName(), this );
    /*m_logo = new Svg( "widgets/amarok-logo", this );
    m_width = 300; // TODO hardcoding for now, do we want this configurable?
    m_aspectRatio = (qreal)m_logo->size().height() / (qreal)m_logo->size().width();
    m_logo->resize( (int)m_width, (int)( m_width * m_aspectRatio ) );*/
    DEBUG_LINE_INFO

    connect( this, SIGNAL( appletAdded( Plasma::Applet*, const QPointF & ) ), this, SLOT( addApplet( Plasma::Applet*, const QPointF & ) ) );

    DEBUG_LINE_INFO

    m_appletBrowserAction = new QAction(KIcon("list-add-amarok"), i18n("Add Applet..."), this);
    connect( m_appletBrowserAction, SIGNAL( triggered(bool) ), this, SLOT( showAddWidgetsInterface() ) );
    // set up default context menu actions
    m_actions = new QList<QAction*>();
    m_actions->append( m_appletBrowserAction );

    DEBUG_LINE_INFO

    debug() << "Creating ColumnContainment, max size:" << maximumSize();

    connect( this, SIGNAL( appletRemoved( Plasma::Applet* ) ), this, SLOT( appletRemoved( Plasma::Applet* ) ) );
    //connect ( Plasma::Theme::self(), SIGNAL( changed() ), this, SLOT( paletteChange() ) );
    connect( KGlobalSettings::self(), SIGNAL(kdisplayPaletteChanged() ), this, SLOT( paletteChange() ) );
    
}

void
ColumnContainment::setTitle( QString title )
{    
    m_title->setText( title );
}

void
ColumnContainment::showTitle()
{
//     m_header->resize( geometry().width(), 40 );
    m_paintTitle = true;
    int offSetX = ( geometry().width() - 100 ) / 2;
    int offSetY = contentsRect().topLeft().y() - 40;
    m_title->setPos( offSetX , offSetY );
    m_title->show();
}

void
ColumnContainment::hideTitle()
{
    m_title->hide();
    m_paintTitle = false;
}

void ColumnContainment::saveToConfig( KConfigGroup& conf )
{
    QStringList plugins;
    for( int i = 0; i < m_grid->count(); i++ )
    {
        Applet *applet = 0;
        QGraphicsLayoutItem *item = m_grid->itemAt( i );
        applet = dynamic_cast<Plasma::Applet*>( item );
        debug() << "trying to save an applet";
        if( applet != 0 && applet->pluginName() != "currenttrack" ) //don't save the current track applet
        {
            debug() << "saving applet" << applet->name();
            plugins << applet->pluginName();
        }
        conf.writeEntry( "plugins", plugins );
    }
}

void
ColumnContainment::loadConfig( const KConfigGroup& conf )
{
    DEBUG_BLOCK
    if( m_manageCurrentTrack )
    {
        Plasma::Containment::addApplet( "currenttrack" );
    }
    QStringList plugins = conf.readEntry( "plugins", QStringList() );
    m_appletsFromConfigCount = 1 + plugins.size();
    foreach( const QString& plugin, plugins )
    {
        debug() << "Adding applet: " << plugin;
        if( m_currentColumns == 0 || m_currentRows == 0 )
            m_pendingApplets.enqueue( plugin );
        else
            Plasma::Containment::addApplet( plugin );        
    }
}

void ColumnContainment::loadInitialConfig()
{
    DEBUG_BLOCK

    QSize cvSize = Amarok::config( "ContextView" ).readEntry( "ContextView size", QSize() );

    if( cvSize.isValid() )
    {
        m_currentColumns = cvSize.width() / m_minColumnWidth;
        m_currentRows = cvSize.height() / m_defaultRowHeight;
        m_width = cvSize.width();
    }
    else
    {
        //FIXME: get these values based on the current application size
        m_currentColumns = 1;
        m_currentRows = 4;
        m_width = 500.0;
    }
}

QSizeF ColumnContainment::sizeHint( Qt::SizeHint which, const QSizeF &constraint ) const
{
    Q_UNUSED( which )
    Q_UNUSED( constraint )

    return geometry().size();
}

QRectF ColumnContainment::boundingRect() const
{
    return geometry();
}

// call this when the view changes size: e.g. layout needs to be recalculated
void ColumnContainment::updateSize( QRectF rect )
{
    DEBUG_BLOCK

    // HACK HACK HACK i don't know where maximumSize is being set, but SOMETHING is setting it,
    // and is preventing the containment from expanding when it should.
    // so, we manually keep the size high.
    prepareGeometryChange();
    
    setMaximumSize( QSizeF( INT_MAX, INT_MAX ) );
    
    m_grid->setGeometry( rect );
    setGeometry( rect );
    
    m_currentRows = ( int )( rect.height() ) / m_defaultRowHeight;
    
    const int columns = ( int )( rect.width() ) / m_minColumnWidth;
    debug() << "rect.width(): " << rect.width();
    debug() << "m_minColumnWidth(): " << m_minColumnWidth;
    debug() << "columns: " << columns;
    

    if( columns != m_currentColumns )
    {
        const int rowCount = m_grid->rowCount();
        const bool hide = columns  < m_currentColumns;
        const int columnsToUpdate = qAbs( m_currentColumns - columns );
        const int col = hide ? m_currentColumns - columnsToUpdate : m_currentColumns;
            
        for( int j = col; j < col + columnsToUpdate; j++ )
        {
            for( int i = 0; i < rowCount; )
            {
                debug() << "Column: " << j << " Row: " << i;
                Plasma::Applet* applet = static_cast< Plasma::Applet* >( m_grid->itemAt( i, j ) );

                if( !applet ) //probably there are no applets left in the column
                {
                    debug() << "no applet";
                    break;
                }
                
                if( hide )
                    applet->hide();
                else
                    applet->show();

                QList<int> pos = m_appletsPositions[applet];
                const int rowSpan = pos[2];
                i += rowSpan;
            }
        }
    }

    m_currentColumns = columns;

    if( !m_pendingApplets.isEmpty() &&  m_currentColumns > 0 && m_currentRows > 0 )
    {        
        while( !m_pendingApplets.isEmpty() )
        {
            Plasma::Containment::addApplet( m_pendingApplets.dequeue() );
        }
    }
 
    m_grid->updateGeometry();
    m_maxColumnWidth = rect.width();
    
    if( m_currentColumns > 0 )
        recalculate();
    if( m_toolBox )
    {
        m_toolBox->setPos( geometry().width() / 2 - m_toolBox->size(), geometry().height() - m_toolBox->size() * 3 );
    }
    //debug() << "ColumnContainment updating size to:" << geometry() << "sceneRect is:" << scene()->sceneRect() << "max size is:" << maximumSize();
}

void ColumnContainment::paintInterface(QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect& rect )
{
    Q_UNUSED( option );
    painter->save();

    /*int height = rect.height(); //?
    int width = rect.width();

    int offsetX = The::mainWindow()->contextRectGlobal().x();
    int offsetY = The::mainWindow()->contextRectGlobal().topLeft().y();

    //debug() << "offset: " << offsetX << " x " << offsetY;

    painter->drawPixmap( 0, 0, WidgetBackgroundPainter::instance()->getBackground( "Context", offsetX, offsetY, 0, 0, width, height ) );
*/

    QRectF bounds = The::svgHandler()->getRenderer()->boundsOnElement ( "amarok_logo" );
    double aspectRatio = bounds.width() / bounds.height();
    
    int logoWidth = 300;
    int logoHeight = ( int )( ( double ) logoWidth / aspectRatio );

    painter->drawPixmap(rect.width() - ( logoWidth + 20 ) , rect.height() - ( logoHeight + 20 ) , The::svgHandler()->renderSvg( "amarok_logo", logoWidth, logoHeight, "amarok_logo" ) );
    if( m_paintTitle )
    {
        int width = rect.width() * 0.95;
        int offsetX = ( rect.width() - width ) / 2;
        QRectF headerRect( rect.topLeft().x() + offsetX,
                           rect.topLeft().y() - 55,                           
                           width,
                           55 );
        m_header->paint( painter, headerRect );
    }    
    painter->restore();
    
    /*QSize size = m_logo->size();
    QSize pos = rect.size() - size;
    qreal newHeight  = m_aspectRatio * m_width;
    m_logo->resize( QSize( (int)m_width, (int)newHeight ) );
    painter->save();
    m_logo->paint( painter, QRectF( pos.width() - 10.0, pos.height() - 5.0, size.width(), size.height() ) );
    painter->restore();*/
}


bool
ColumnContainment::insertInGrid( Plasma::Applet* applet )
{
    DEBUG_BLOCK
    int width;

    // this is done beucause of the CV small startup width. We use the width read from config file, may be not safe
    // in some cases.
    if( m_appletsFromConfigCount )
    {
        DEBUG_LINE_INFO
        width = m_currentColumns ? m_width / m_currentColumns : m_width;
        m_appletsFromConfigCount--;
    }
    else
        width = m_currentColumns ? m_maxColumnWidth / m_currentColumns : m_maxColumnWidth;
    int height = applet->effectiveSizeHint( Qt::PreferredSize, QSizeF( width, -1 ) ).height();
//     int rowSpan = qMin( qMax( ( int )( height )/ m_defaultRowHeight , 1 ), 2 );
    int rowSpan = height / m_defaultRowHeight;
    
    int colSpan = 1;

    if( rowSpan == 0 || height % m_defaultRowHeight >  m_defaultRowHeight / 2.0  )
        rowSpan += 1;
    
    rowSpan = qMin( rowSpan, m_currentRows );
    
    debug() << "current columns: " << m_currentColumns;
    debug() << "current rows: " << m_currentRows;
    debug() << "applet: " << applet->pluginName();
    debug() << "width: " << width;
    debug() << "height: " << height;
    debug() << "rowspan: " << rowSpan;

    int col = 0;
    int row = 0;
    bool positionFound = false;
    int j = 0;

    //traverse the grid matrix to find rowSpan consecutive positions in the same column

    while( !positionFound && j < m_currentColumns   )
    {
        int consec = 0;
        int i = 0;
        while( i < m_currentRows && consec < rowSpan)
        {
            if( m_gridFreePositions[i][j] )
                consec++;
            else
                consec = 0;
            i++;
        }
        if( consec == rowSpan )
        {
            positionFound = true;
            row = i - rowSpan; //get the first of the consecutive rows
            col = j;
        }
        j++;
    }

    if( positionFound )
    {

        QRectF rect = geometry();
        debug() << "applet height: " << height;
        debug() << "applet inserted at: " << row << col;
        debug() << "applet rowSpan :" << rowSpan;

        QList<int> pos;
        pos << row << col << rowSpan;
        m_appletsPositions[applet] = pos;
        m_appletsIndexes[applet] = m_grid->count();
//         m_grid->setColumnMaximumWidth( col, m_maxColumnWidth/m_currentColumns );
        m_grid->setColumnMinimumWidth( col, m_minColumnWidth );        
        
        for( int i = 0; i < rowSpan; i++ )
        {
            m_gridFreePositions[row + i][col] = false;
            m_grid->setRowMaximumHeight( row + i, m_defaultRowHeight );
            m_grid->setRowPreferredHeight( row + i, height );
        }
        if( m_grid->columnCount() > 0 ) 
            applet->resize( m_maxColumnWidth / m_grid->columnCount(), rowSpan * m_defaultRowHeight );
        else
            applet->resize( m_maxColumnWidth, rowSpan * m_defaultRowHeight );
        m_grid->addItem( applet, row, col, rowSpan, colSpan );

        updateConstraints( Plasma::SizeConstraint );
        recalculate();
        m_grid->updateGeometry();
    }
    return positionFound;
}

Plasma::Applet* ColumnContainment::addApplet( Plasma::Applet* applet, const QPointF & )
{
    DEBUG_BLOCK
//     debug() << "m_columns:" << m_columns;
//     m_columns->addItem( applet );

    
    if( !insertInGrid( applet ) )
    {
        debug() << "Send applet to the next free containment";

        int height = applet->effectiveSizeHint( Qt::PreferredSize,
                                            QSizeF( m_maxColumnWidth, -1 ) ).height();
        int rowSpan = height / m_defaultRowHeight;
        if( rowSpan == 0 || height % m_defaultRowHeight >  m_defaultRowHeight / 2.0  )
            rowSpan += 1;
        debug() << "emit appletRejected at containment";
        emit appletRejected( applet->pluginName(), rowSpan );
        applet->destroy();
        return 0;
    }
//     recalculate();
    
    return applet;
}

bool
ColumnContainment::hasPlaceForApplet( int rowSpan )
{
    bool positionFound = false;
    int j = 0;
    //traverse the grid matrix to find rowSpan consecutive positions in the same column

    while( !positionFound && j < m_currentColumns   )
    {
        int consec = 0;
        int i = 0;
        while( i < m_currentRows && consec < rowSpan)
        {

            if( m_gridFreePositions[i][j] )
                consec++;
            else
                consec = 0;
            i++;
        }
        if( consec == rowSpan )
        {
            positionFound = true;
        }
        j++;
    }
    return positionFound;
}


void ColumnContainment::recalculate()
{
    DEBUG_BLOCK
    debug() << "got child item that wants a recalculation";

    QRectF rect = geometry();
    
    qreal left, top, right, bottom;
    qreal marginTop;
    qreal height;
    qreal width; 
    
    int gridRows = m_grid->rowCount();
    int gridCols = m_grid->columnCount();
    getContentsMargins( &left, &marginTop, &right, &bottom );
    width = m_grid->columnCount() ? m_maxColumnWidth / m_grid->columnCount() : m_maxColumnWidth;

    for( int col = 0; col < gridCols; col++ )
    {                
        
        left += (qreal)( col * width );
        int row = 0;
        top = marginTop;

        while( row < gridRows )
        {
            Plasma::Applet *applet = dynamic_cast< Plasma::Applet *>( m_grid->itemAt( row, col ) );
            if( applet )
            {                               
                height = applet->effectiveSizeHint( Qt::PreferredSize,
                                                        QSizeF( width, -1 ) ).height();
                                                    
                QList<int> pos = m_appletsPositions[applet];
                int rowSpan = pos[2];
                
                height = rowSpan * m_defaultRowHeight;

                //add 25 pixels offSet to the right only if its the last column
                int offSet = col == gridCols - 1 ? 25 : 3;
                
                applet->resize( width - offSet, height );
                applet->setPos( left, top + 6 );
                row += rowSpan;
                top += height; 
            }
            else
            {
                row++; // ignore it
            }
        }

    }
//     m_columns->invalidate();
//    m_columns->relayout();
}

QList<QAction*> ColumnContainment::contextualActions()
{
    return *m_actions;
}


void ColumnContainment::mousePressEvent( QGraphicsSceneMouseEvent * event )
{
    DEBUG_BLOCK

    // Discoverability: We're also showing the context menu on left-click, when clicking empty space
    if( event->button() == Qt::LeftButton )
    {
//         bool insideApplet = false;
//         foreach( Applet* applet, applets() )
//             insideApplet |= applet->geometry().contains( event->pos() );
// 
//         if( !insideApplet )
//             m_appletBrowserAction->trigger();

        debug() << "Focus requested by containment";
        emit focusRequested( this );
    }
}


void
ColumnContainment::showAddWidgetsInterface()
{
    emit Plasma::Containment::showAddWidgetsInterface( QPointF() );
}

/*
bool ColumnContainment::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
    Applet *applet = qgraphicsitem_cast<Applet*>(watched);
    //QEvent::GraphicsSceneHoverEnter

    // Otherwise we're watching something we shouldn't be...
    Q_ASSERT(applet!=0);

    return false;
}*/

void ColumnContainment::jobDone()
{
    m_masterImage = m_job->m_image;
    m_job->deleteLater();
    m_job = 0;
    //we should request a redraw of  applet now, but I have now idea how to do that
}


void ColumnContainment::paletteChange()
{
    The::svgHandler()->reTint();
    update( boundingRect() );
}

void
ColumnContainment::appletRemoved( Plasma::Applet* applet )
{
    DEBUG_BLOCK
    if( m_appletsPositions.contains( applet ) )
    {
        QList<int> pos = m_appletsPositions[applet];
        int row = pos[0];
        int col = pos[1];
        int rowSpan = pos[2];
        for( int i = 0; i < rowSpan; i++ )
            m_gridFreePositions[row + i][col] = true;
        m_appletsPositions.remove( applet );
        m_appletsIndexes.remove( applet );

        //keep the indexes updated
        int idx = m_appletsIndexes[applet];
        foreach( Plasma::Applet *a, m_appletsIndexes.keys() )
            if( m_appletsIndexes[a] > idx )
                m_appletsIndexes[a]--;
        
        if( m_grid->count() > 0 )
            rearrangeApplets( row + rowSpan, col );
        
    }
//         m_columns->removeItem( item );
    
}

void
ColumnContainment::rearrangeApplets( int startRow, int startColumn )
{
    DEBUG_BLOCK
    int i = startRow;
    
    int lastColumn = m_grid->columnCount();
    debug() << "start row: " << startRow;
    debug() << "row count: " << m_grid->rowCount();

    for( int j = startColumn; j < lastColumn; j++ )
    {
        int lastRow = m_grid->rowCount();

        while( i < lastRow )
        {
            Plasma::Applet *applet = static_cast< Plasma::Applet* >( m_grid->itemAt( i, j ) );
            if( !applet )
            {
                debug() << "bad cast";
                break;
            }
            
            QList<int> pos = m_appletsPositions[applet];
            int rowSpan = pos[2];
            int idx = m_appletsIndexes[applet];
            
            debug() << "remove applet with index: " << idx;
            m_grid->removeAt( idx );
            
            for( int k = 0; k < rowSpan; k++ )
                m_gridFreePositions[i + k][j] = true;

            foreach( Plasma::Applet* a, m_appletsIndexes.keys() )
                if( m_appletsIndexes[a] > idx )
                    m_appletsIndexes[a]--;
                
            insertInGrid( applet );
            i += rowSpan;
        }
        i = 0;
    }
    
}

void
ColumnContainment::addToolBox()
{
    DEBUG_BLOCK
    if( m_toolBox )
        return;
    m_toolBox = new AmarokToolBox( this );
    debug() << geometry().width();
    debug() << geometry().height();
    m_toolBox->setPos( geometry().width() / 2 - m_toolBox->size(), geometry().height() - m_toolBox->size() *4 );
    m_toolBox->show();
    m_toolBox->addAction( action( "add widgets") );
    m_toolBox->addAction( action( "zoom in" ) );
    m_toolBox->addAction( action( "zoom out" ) );
    
}


void
ColumnContainment::correctToolBoxPos( Plasma::ZoomDirection direction )
{
    if( direction == Plasma::ZoomOut )
    {
        m_toolBox->setPos( geometry().width() / 2 - m_toolBox->size()*3,
                           geometry().height() - m_toolBox->size() * 7 );
    }
    else
    {
        m_toolBox->setPos( geometry().width() / 2 - m_toolBox->size(),
                           geometry().height() - m_toolBox->size() * 3 );
    }
}

ColumnContainment::~ColumnContainment()
{
    DEBUG_BLOCK

    clearApplets();
    m_appletsPositions.clear();
    Amarok::config( "ContextView" ).writeEntry( "ContextView size", size() );
}

void
ColumnContainment::addCurrentTrack()
{
    DEBUG_BLOCK
    m_manageCurrentTrack = true;
}

} // Context namespace



#include "ColumnContainment.moc"
