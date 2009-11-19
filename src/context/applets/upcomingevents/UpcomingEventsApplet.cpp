/****************************************************************************************
 * Copyright (c) 2009 Joffrey Clavel <jclavel@clabert.info>                             *
 * Copyright (c) 2009 Oleksandr Khayrullin <saniokh@gmail.com>                          *
 * Copyright (c) 2009 Ludovic Deveaux <deveaux.ludovic31@gmail.com>                     *
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

#include "UpcomingEventsApplet.h"

#include "Amarok.h"
#include "App.h"
#include "Debug.h"
#include "context/Svg.h"
#include "context/ContextView.h"
#include "PaletteHandler.h"

#include <Plasma/Theme>
#include <plasma/widgets/iconwidget.h>

#include <KConfigDialog>
#include <KStandardDirs>
#include "context/widgets/TextScrollingWidget.h"
#include "context/widgets/DropPixmapItem.h"

#include <QDesktopServices>
#include <QGraphicsSimpleTextItem>
#include <QLabel>
#include <QHBoxLayout>



UpcomingEventsApplet::UpcomingEventsApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_aspectRatio( 0 )
    , m_headerAspectRatio( 0.0 )
    , m_headerLabel( 0 )
    , m_settingsIcon( 0 )    
{
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );
}

void
UpcomingEventsApplet::init()
{
    m_headerLabel = new QGraphicsSimpleTextItem( this );
    m_eventParticipants = new TextScrollingWidget( this );
    m_bigImage = new DropPixmapItem( this );
    m_eventName = new QGraphicsSimpleTextItem( this );
    m_eventDate = new QGraphicsSimpleTextItem( this );
    m_url = new QGraphicsTextItem( this );

    // ask for all the CV height
    resize( 500, -1 );

    m_headerLabel->setText( i18n( "Upcoming Events" ) );

    QAction* settingsAction = new QAction( this );
    settingsAction->setIcon( KIcon( "preferences-system" ) );
    settingsAction->setEnabled( true );
    m_settingsIcon = addAction( settingsAction );
    m_settingsIcon->setToolTip( i18n( "Settings" ) );
    connect( m_settingsIcon, SIGNAL( activated() ), this, SLOT( configure() ) );

    connectSource( "upcomingEvents" );
    connect( dataEngine( "amarok-upcomingEvents" ), SIGNAL( sourceAdded( const QString & ) ), SLOT( connectSource( const QString & ) ) );

    constraintsEvent();

    // Read config and inform the engine.
    KConfigGroup config = Amarok::config("UpcomingEvents Applet");
    m_timeSpan = config.readEntry( "timeSpan", "AllEvents" );
    
}

void
UpcomingEventsApplet::connectSource( const QString &source )
{
    if( source == "upcomingEvents" )
    {
        dataEngine( "amarok-upcomingEvents" )->connectSource( "upcomingEvents", this );
        dataUpdated( source, dataEngine( "amarok-upcomingEvents" )->query( "upcomingEvents" ) );
    }
}

void
UpcomingEventsApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints );

    prepareGeometryChange();
    const float textWidth = m_headerLabel->boundingRect().width();
    const float offsetX =  ( boundingRect().width() - textWidth ) / 2;

    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_headerLabel->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_headerLabel->setFont( labelFont );
    m_headerLabel->setPos( offsetX, standardPadding() + 2 );

    // Icon positionning
    m_settingsIcon->setPos( size().width() - m_settingsIcon->size().width() - standardPadding(), standardPadding() );

    m_bigImage->setPos( standardPadding() + 5, standardPadding() + 30 );
    if (!m_bigImage->pixmap().isNull())
    {
        m_eventName->setPos( m_bigImage->boundingRect().width() + standardPadding() + 30, standardPadding() + 30 );
        m_eventParticipants->setPos( m_bigImage->boundingRect().width() + standardPadding() + 30, standardPadding() + 60 );
        m_eventParticipants->setScrollingText( "Muse - Depeche Mode - The Cure - The Police", QRectF( ( boundingRect().width() - m_eventParticipants->boundingRect().width() ) * 2 / 3, standardPadding() + 30, 200, 100 ) );
        m_eventDate->setPos( m_bigImage->boundingRect().width() + standardPadding() + 30, standardPadding() + 90 );
        m_url->setPos( m_bigImage->boundingRect().width() + standardPadding() + 30, standardPadding() + 120 );
    }
}

bool
UpcomingEventsApplet::hasHeightForWidth() const
{
    return true;
}

qreal
UpcomingEventsApplet::heightForWidth( qreal width ) const
{
    return width * m_aspectRatio;
}

void
UpcomingEventsApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data ) // SLOT
{
    Q_UNUSED( name )
    QString artistName = data[ "artist" ].toString();
    if (artistName.compare( "" ) != 0)
        m_headerLabel->setText( "Upcoming events for " + artistName );
    else
        m_headerLabel->setText( "Upcoming events" );
    m_bigImage->setPixmap( data[ "cover" ].value<QPixmap>() );
    m_eventName->setText( data[ "eventName" ].toString() );
    m_eventDate->setText( data[ "eventDate" ].toString() );
    m_url->setHtml( data[ "eventUrl" ].toString() );

    updateConstraints();
    update();
}

void
UpcomingEventsApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option )
    Q_UNUSED( contentsRect )

    p->setRenderHint( QPainter::Antialiasing );

    addGradientToAppletBackground( p );

    //draw background of wiki text
    p->save();
    QColor bg( App::instance()->palette().highlight().color() );
    bg.setHsvF( bg.hueF(), 0.07, 1, bg.alphaF() );

    p->restore();
}


void
UpcomingEventsApplet::configure()
{
    DEBUG_BLOCK
    showConfigurationInterface();
}


void
UpcomingEventsApplet::createConfigurationInterface( KConfigDialog *parent )
{
    // for use when gui created
    /*KConfigGroup configuration = config();
    QWidget *settings = new QWidget;
    //ui_Settings.setupUi( settings );

    parent->addPage( settings, i18n( "Upcoming Events Settings" ), "preferences-system");
    connect( ui_Settings.comboBox, SIGNAL( currentIndexChanged( QString ) ), this, SLOT( switchToLang( QString ) ) );*/

    KConfigGroup config = Amarok::config("UpcomingEvents Applet");
    QWidget *settings = new QWidget();
    ui_Settings.setupUi( settings );

       // TODO bad, it's done manually ...
    if ( m_timeSpan == "AllEvents" )
        ui_Settings.comboBox->setCurrentIndex( 0 );
    else if ( m_timeSpan == "ThisWeek" )
        ui_Settings.comboBox->setCurrentIndex( 1 );
    else if ( m_timeSpan == "ThisMonth" )
        ui_Settings.comboBox->setCurrentIndex( 2 );
    else if ( m_timeSpan == "ThisYear" )
        ui_Settings.comboBox->setCurrentIndex( 3 );

    if ( m_enabledLinks )
        ui_Settings.checkBox->setCheckState ( Qt::Checked );
    
    parent->addPage( settings, i18n( "Upcoming Events Settings" ), "preferences-system");
     connect( ui_Settings.comboBox, SIGNAL( currentIndexChanged( QString ) ), this, SLOT( changeTimeSpan( QString ) ) );
     connect( ui_Settings.checkBox, SIGNAL( stateChanged( int ) ), this, SLOT( setAddressAsLink( int ) ) );
}

void
UpcomingEventsApplet::changeTimeSpan(QString span)
{
    DEBUG_BLOCK
    // TODO change this b/c it's BAAADDD !!!

    if (span == i18n("Automatic") )
        m_timeSpan = "AllEvents";

    else if (span == i18n("This week") )
        m_timeSpan = "ThisWeek";

    else if (span == i18n("This month") )
        m_timeSpan = "ThisMonth";

    else if (span == i18n("This year") )
        m_timeSpan = "ThisYear";

    else if (span == i18n("All events") )
        m_timeSpan = "AllEvents";

    dataEngine( "amarok-upcomingEvents" )->query( QString( "upcomingEvents:timeSpan:" ) + m_timeSpan );

    KConfigGroup config = Amarok::config("UpcomingEvents Applet");
    config.writeEntry( "timeSpan", m_timeSpan );
    dataEngine( "amarok-upcomingEvents" )->query( QString( "upcomingEvents:timeSpan:" ) + m_timeSpan );
}

void
UpcomingEventsApplet::setAddressAsLink(int state) {
    DEBUG_BLOCK

    m_enabledLinks = (state == Qt::Checked);
    
    dataEngine( "amarok-upcomingEvents" )->query( QString( "upcomingEvents:enabledLinks:" ) + m_enabledLinks );

    KConfigGroup config = Amarok::config("UpcomingEvents Applet");
    config.writeEntry( "enabledLinks", m_enabledLinks );
    dataEngine( "amarok-upcomingEvents" )->query( QString( "upcomingEvents:enabledLinks:" ) + m_enabledLinks );
}

#include "UpcomingEventsApplet.moc"



