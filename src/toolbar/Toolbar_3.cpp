
#include "Toolbar_3.h"

#include "ActionClasses.h"
#include "Amarok.h"
#include "EngineController.h"

#include "browsers/collectionbrowser/CollectionWidget.h"

#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistController.h"

#include "widgets/AnimatedLabelStack.h"
#include "widgets/PlayPauseButton.h"
#include "widgets/ProgressWidget.h"
#include "widgets/VolumeDial.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>

#include <QtDebug>

// NOTICE shall be 10, but there're the time labels :-(
static const int progressStretch = 16;

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
    m_progress = new ProgressWidget( 0 );
    m_progressLayout->addStretch( 3 );
    m_progressLayout->addWidget( m_progress  );
    m_progressLayout->setStretchFactor( m_progress , progressStretch );
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
    if ( currentState == Phonon::PlayingState )
        m_playPause->setPlaying( true );
    else if ( currentState == Phonon::StoppedState || currentState == Phonon::PausedState )
        m_playPause->setPlaying( false );
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
Toolbar_3::engineTrackChanged( Meta::TrackPtr track )
{
    m_current->setData( metadata( track ) );
    m_current->setCursor( track ? Qt::PointingHandCursor : Qt::ArrowCursor );
    QTimer::singleShot( 0, this, SLOT( updatePrevAndNext() ) );
}

void
Toolbar_3::resizeEvent( QResizeEvent *ev )
{
    if ( ev->size().width() > 0 )
    {
        const int limit = 640;
        if ( ev->size().width() > limit )
            m_progressLayout->setStretchFactor( m_progress, progressStretch );
        int s = limit/ev->size().width();
        s *= s*s*progressStretch;
        m_progressLayout->setStretchFactor( m_progress, qMax( progressStretch, s ) );
    }
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

