
#include "Toolbar_3.h"

#include "ActionClasses.h"
#include "Amarok.h"
#include "EngineController.h"
#include "GlobalCurrentTrackActions.h"

#include "amarokurls/AmarokUrl.h"
#include "amarokurls/AmarokUrlHandler.h"

#include "browsers/collectionbrowser/CollectionWidget.h"

#include "meta/capabilities/CurrentTrackActionsCapability.h"
#include "meta/MetaUtility.h"
#include "meta/capabilities/TimecodeLoadCapability.h"

#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistController.h"

#include "widgets/AnimatedLabelStack.h"
#include "widgets/PlayPauseButton.h"
#include "widgets/SliderWidget.h"
#include "widgets/VolumeDial.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>

#include <QtDebug>

// NOTICE shall be 10, but there're the time labels :-(
static const int sliderStretch = 16;

Toolbar_3::Toolbar_3( QWidget *parent )
    : QToolBar( i18n( "Toolbar 3G" ), parent )
    , EngineObserver( The::engineController() )
{
    setObjectName( "Toolbar_3G" );
    
    EngineController *engine = The::engineController();
    setIconSize( QSize( 48, 48 ) );
    setContentsMargins( 4, 0, 4, 0 );

    m_playPause = new PlayPauseButton;
    m_playPause->setPlaying( engine->state() == Phonon::PlayingState );
    m_playPause->setFixedSize( 48, 48 );
    addWidget( m_playPause );
    connect ( m_playPause, SIGNAL( toggled(bool) ), this, SLOT( setPlaying(bool) ) );

    QWidget *info = new QWidget(this);
    QHBoxLayout *hl = new QHBoxLayout;
    QVBoxLayout *vl = new QVBoxLayout( info );
    
    m_prev = new AnimatedLabelStack(QStringList(), info);
    m_prev->setAnimated( false );
    m_prev->setOpacity( 128 );
    m_prev->installEventFilter( this );
    m_prev->setAlign( Qt::AlignLeft );
    connect ( m_prev, SIGNAL( clicked(const QString&) ), The::playlistActions(), SLOT( back() ) );

    m_current = new AnimatedLabelStack(QStringList() << "Amarok your Music", info);
    m_current->setBold( true );
    connect ( m_current, SIGNAL( clicked(const QString&) ), this, SLOT( filter(const QString&) ) );

    m_next = new AnimatedLabelStack(QStringList(), info);
    m_next->setAnimated( false );
    m_next->setOpacity( 160 );
    m_next->installEventFilter( this );
    m_next->setAlign( Qt::AlignRight );
    connect ( m_next, SIGNAL( clicked(const QString&) ), The::playlistActions(), SLOT( next() ) );

    hl->addWidget( m_prev );
    hl->addWidget( m_current );
    hl->addWidget( m_next );
    vl->addLayout( hl );

    connect ( m_prev, SIGNAL( pulsing(bool) ), m_current, SLOT( setStill(bool) ) );
    connect ( m_next, SIGNAL( pulsing(bool) ), m_current, SLOT( setStill(bool) ) );

    
    m_progressLayout = new QHBoxLayout;
    m_progressLayout->addStretch( 3 );
    m_progressLayout->addWidget( m_timeLabel = new QLabel( this ) );
    m_timeLabel->setAlignment( Qt::AlignVCenter | Qt::AlignRight );
    m_timeLabel->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );
    m_timeLabel->setMinimumWidth( QFontMetrics( m_timeLabel->font() ).width( "33:33:33" ) );

    m_progressLayout->addWidget( m_slider = new Amarok::TimeSlider( this ) );
    m_progressLayout->setStretchFactor( m_slider , sliderStretch );
    connect( m_slider, SIGNAL( sliderReleased( int ) ), The::engineController(), SLOT( seek( int ) ) );
    connect( m_slider, SIGNAL( valueChanged( int ) ), SLOT( setLabelTime( int ) ) );

    m_progressLayout->addWidget( m_trackActionBar = new QToolBar( this ) );
    m_trackActionBar->setToolButtonStyle( Qt::ToolButtonIconOnly );
    m_trackActionBar->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred );
    m_trackActionBar->setIconSize( QSize( 16,16 ) );

    m_progressLayout->addStretch( 3 );
    vl->addLayout( m_progressLayout );

    
    addWidget( info );

    m_volume = new VolumeDial( this );
    m_volume->setRange( 0, 100);
    m_volume->setValue( engine->volume() );
    m_volume->setMute( engine->isMuted() );
    m_volume->setFixedSize( 48, 48 );
    addWidget( m_volume );
    connect ( m_volume, SIGNAL( valueChanged(int) ), engine, SLOT( setVolume(int) ) );
    connect ( m_volume, SIGNAL( muteToggled(bool) ), engine, SLOT( setMuted(bool) ) );

    connect ( The::playlistController(), SIGNAL( changed()), this, SLOT( updatePrevAndNext() ) );
    connect ( The::amarokUrlHandler(), SIGNAL( timecodesUpdated(const QString*) ),
              this, SLOT( updateBookmarks(const QString*) ) );
    connect ( The::amarokUrlHandler(), SIGNAL( timecodeAdded(const QString&, int) ),
              this, SLOT( addBookmark(const QString&, int) ) );
}

void
Toolbar_3::addBookmark( const QString &name, int milliSeconds )
{
    if ( m_slider )
        m_slider->drawTriangle( name, milliSeconds, false );
}

void
Toolbar_3::engineVolumeChanged( int percent )
{
    m_volume->setValue( percent );
}

void
Toolbar_3::engineMuteStateChanged( bool mute )
{
    m_volume->setMute( mute );
}

void
Toolbar_3::engineStateChanged( Phonon::State currentState, Phonon::State oldState )
{
    if ( currentState == oldState )
        return;

    switch ( currentState )
    {
    case Phonon::PlayingState:
        m_playPause->setPlaying( true );
        break;
    case Phonon::StoppedState:
        m_timeLabel->setText( QString() );
    case Phonon::PausedState:
        m_playPause->setPlaying( false );
        break;
    case Phonon::LoadingState:
        if ( !The::engineController()->currentTrack() ||
             ( m_currentUrlId != The::engineController()->currentTrack()->uidUrl() ) )
        {
            m_timeLabel->setText( QString() );
            m_slider->setEnabled( false );
        }
        break;
    default:
        break;
    }
}

void
Toolbar_3::filter( const QString &string )
{
    qDebug() << "filter by" << string << CollectionWidget::instance();
    if ( CollectionWidget::instance() )
        CollectionWidget::instance()->setFilter( string );
}

#define HAS_TAG(_TAG_) !track->_TAG_()->name().isEmpty()
#define TAG(_TAG_) track->_TAG_()->prettyName()

static QStringList metadata( Meta::TrackPtr track )
{
    QStringList list;
    QRegExp rx("(\\s+-\\s+|\\s*;\\s*|\\s*:\\s*)"); // this will split "all-in-one" filename tags
    if ( track )
    {
        if ( !track->name().isEmpty() )
        {
            QString title = track->prettyName();
//             qDebug() << track->prettyUrl();
            if ( title.length() > 50 ||
                 HAS_TAG(artist) && title.contains( TAG(artist), Qt::CaseInsensitive ) ||
                 HAS_TAG(composer) && title.contains( TAG(artist), Qt::CaseInsensitive ) ||
                 HAS_TAG(album) && title.contains( TAG(album) ), Qt::CaseInsensitive )
            {
                list << title.split( rx, QString::SkipEmptyParts );
            }
            else
            {
                list << title;
            }
        }

        if ( HAS_TAG(artist) )
            list << TAG(artist);
        else if ( HAS_TAG(composer) )
            list << TAG(composer);
        if ( HAS_TAG(album) )
            list << TAG(album);
        if ( HAS_TAG(year) && TAG(year) != "0" ) // "0" years be empty?!
            list << TAG(year);
        if ( HAS_TAG(genre) )
            list << TAG(genre);

        /* other tags
        double score
        int rating
        qint64 length // ms
        int sampleRate
        int bitrate
        int trackNumber
        int discNumber
        uint lastPlayed
        uint firstPlayed
        int playCount
        QString type
        bool inCollection
        */
    }
    return list;
}

#undef HAS_TAG
#undef TAG

void
Toolbar_3::updatePrevAndNext()
{
    Meta::TrackPtr track = The::playlistActions()->prevTrack();
    m_prev->setData( metadata( track ) );
    m_prev->setCursor( track ? Qt::PointingHandCursor : Qt::ArrowCursor );
    
    track = The::playlistActions()->nextTrack();
    m_next->setData( metadata( track ) );
    m_next->setCursor( track ? Qt::PointingHandCursor : Qt::ArrowCursor );
}

void
Toolbar_3::updateBookmarks( const QString *BookmarkName )
{
    DEBUG_BLOCK
    m_slider->clearTriangles();
    if ( Meta::TrackPtr track = The::engineController()->currentTrack() )
    {
        if ( track->hasCapabilityInterface( Meta::Capability::LoadTimecode ) )
        {
            Meta::TimecodeLoadCapability *tcl = track->create<Meta::TimecodeLoadCapability>();
            BookmarkList list = tcl->loadTimecodes();
            debug() << "found " << list.count() << " timecodes on this track";
            foreach( AmarokUrlPtr url, list )
            {
                if ( url->command() == "play" && url->args().keys().contains( "pos" ) )
                {
                    int pos = url->args().value( "pos" ).toInt() * 1000;
                    debug() << "showing timecode: " << url->name() << " at " << pos ;
                    m_slider->drawTriangle( url->name(), pos, ( BookmarkName && BookmarkName == url->name() ) );
                }
            }
            delete tcl;
        }
    }
}

void
Toolbar_3::engineTrackChanged( Meta::TrackPtr track )
{
    if ( !track )
        m_currentUrlId.clear();
    m_timeLabel->setText( QString() );
    setActionsFrom( track );
    m_current->setData( metadata( track ) );
    m_current->setCursor( track ? Qt::PointingHandCursor : Qt::ArrowCursor );
    QTimer::singleShot( 0, this, SLOT( updatePrevAndNext() ) );
}

void
Toolbar_3::engineTrackLengthChanged( qint64 ms )
{
    m_slider->setRange( 0, ms );
    m_slider->setEnabled( ms > 0 );

    // get the urlid of the current track as the engine might stop and start several times
    // when skipping last.fm tracks, so we need to know if we are still on the same track...
    if ( The::engineController()->currentTrack() )
        m_currentUrlId = The::engineController()->currentTrack()->uidUrl();

    updateBookmarks( 0 );
}

void
Toolbar_3::engineTrackPositionChanged( qint64 position, bool /*userSeek*/ )
{
    m_slider->setSliderValue( position );
//     if ( !m_slider->isEnabled() )
//         setLabelTime( position )
}

void
Toolbar_3::resizeEvent( QResizeEvent *ev )
{
    if ( ev->size().width() > 0 )
    {
        const int limit = 640;
        if ( ev->size().width() > limit )
            m_progressLayout->setStretchFactor( m_slider, sliderStretch );
        int s = limit/ev->size().width();
        s *= s*s*sliderStretch;
        m_progressLayout->setStretchFactor( m_slider, qMax( sliderStretch, s ) );
    }
}

void
Toolbar_3::setActionsFrom( Meta::TrackPtr track )
{
    m_trackActionBar->clear();

    foreach( QAction* action, The::globalCurrentTrackActions()->actions() )
        m_trackActionBar->addAction( action );
    
    if( track && track->hasCapabilityInterface( Meta::Capability::CurrentTrackActions ) )
    {
        Meta::CurrentTrackActionsCapability *cac = track->create<Meta::CurrentTrackActionsCapability>();
        if( cac )
        {
            QList<QAction *> currentTrackActions = cac->customActions();
            foreach( QAction *action, currentTrackActions )
                m_trackActionBar->addAction( action );
        }
        delete cac;
    }
}

void Toolbar_3::setLabelTime( int ms )
{
    m_timeLabel->setText( Meta::secToPrettyTime( ms/1000 ) );
}

void
Toolbar_3::setPlaying( bool on )
{
    if ( on )
        The::engineController()->play();
    else
        The::engineController()->pause();
}

bool
Toolbar_3::eventFilter( QObject *o, QEvent *ev )
{
    if ( ev->type() == QEvent::Enter )
    {
        if (o == m_next)
            m_next->setOpacity( 255 );
        else if (o == m_prev)
            m_prev->setOpacity( 255 );
        return false;
    }
    if ( ev->type() == QEvent::Leave )
    {
        if (o == m_next)
            m_next->setOpacity( 160 );
        else if (o == m_prev)
            m_prev->setOpacity( 128 );
        return false;
    }
    return false;
}

