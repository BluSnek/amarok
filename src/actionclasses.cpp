// Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright:  See COPYING file that comes with this distribution

#include "actionclasses.h"
#include "amarokconfig.h"
#include "app.h"                //actionCollection() and a SLOT
#include "enginecontroller.h"
#include "playlistwindow.h"     //need amaroK::ToolBar
#include "socketserver.h"       //Vis::Selector::showInstance()

#include <kaction.h>
#include <kapplication.h>
#include <khelpmenu.h>
#include <kiconloader.h>
#include <klocale.h>


using namespace amaroK;

KHelpMenu *Menu::HelpMenu = 0;


static void
safePlug( KActionCollection *ac, const char *name, QWidget *w )
{
    if( ac )
    {
        KAction *a = ac->action( name );
        if( a ) a->plug( w );
    }
}


Menu::Menu( QWidget *parent )
  : KPopupMenu( parent )
{
    KActionCollection *ac = pApp->actionCollection();

    setCheckable( true );

    safePlug( ac, "repeat_track", this );
    safePlug( ac, "repeat_playlist", this );
    safePlug( ac, "random_mode", this );

    insertSeparator();

    insertItem( i18n( "&Visualizations..." ), ID_SHOW_VIS_SELECTOR );

    insertSeparator();

    insertItem( i18n( "Configure &Effects..." ), pApp, SLOT( showEffectWidget() ) );
    insertItem( i18n( "Configure &Decoder..." ), ID_CONF_DECODER );

    insertSeparator();

    safePlug( ac, KStdAction::name(KStdAction::ConfigureToolbars), this );
    safePlug( ac, KStdAction::name(KStdAction::KeyBindings), this );
    safePlug( ac, "options_configure_globals", this ); //we created this one
    safePlug( ac, KStdAction::name(KStdAction::Preferences), this );

    insertSeparator();

    insertItem( SmallIcon("help"), i18n( "&Help" ), helpMenu( parent ) );

    insertSeparator();

    safePlug( ac, KStdAction::name(KStdAction::Quit), this );

    connect( this, SIGNAL( aboutToShow() ), SLOT( slotAboutToShow() ) );
    connect( this, SIGNAL( activated(int) ), SLOT( slotActivated(int) ) );
}

KPopupMenu*
Menu::helpMenu( QWidget *parent ) //STATIC
{
    if( HelpMenu == 0 )
        HelpMenu = new KHelpMenu( parent, KGlobal::instance()->aboutData(), pApp->actionCollection() );

    return HelpMenu->menu();
}

void
Menu::slotAboutToShow()
{
    setItemEnabled( ID_CONF_DECODER, EngineController::engine()->decoderConfigurable() );
}

void
Menu::slotActivated( int index )
{
    switch( index )
    {
    case ID_CONF_DECODER:
        EngineController::engine()->configureDecoder();
        break;
    case ID_SHOW_VIS_SELECTOR:
        Vis::Selector::instance()->show(); //doing it here means we delay creation of the widget
        break;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// MenuAction
//////////////////////////////////////////////////////////////////////////////////////////
#include <ktoolbar.h>
#include <ktoolbarbutton.h>

//there is KActionMenu, but it doesn't work very well, hence we made our own

MenuAction::MenuAction( KActionCollection *ac )
  : KAction( i18n( "amaroK Menu" ), 0, ac, "amarok_menu" )
{}

int
MenuAction::plug( QWidget *w, int index )
{
    KToolBar *bar = dynamic_cast<KToolBar*>(w);

    if( bar && kapp->authorizeKAction( name() ) )
    {
        const int id = KAction::getToolButtonID();

        addContainer( w, id );
        connect( w, SIGNAL( destroyed() ), SLOT( slotDestroyed() ) );

        //TODO create menu on demand
        //TODO create menu above and aligned within window
        //TODO make the arrow point upwards!
        bar->insertButton( QString::null, id, true, i18n( "Menu" ), index );
        bar->alignItemRight( id );

        KToolBarButton* button = bar->getButton( id );
        button->setPopup( new amaroK::Menu( 0 ) ); //do not parent to the toolbar! Causes the about dialog to
                                                   //cause the toolbar to increase it's height when opened. Odd.
        button->setName( "toolbutton_amarok_menu" );

        return containerCount() - 1;
    }
    else return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////
// PlayPauseAction
//////////////////////////////////////////////////////////////////////////////////////////
PlayPauseAction::PlayPauseAction( KActionCollection *ac )
  : KAction( i18n( "Play/Pause" ), 0, ac, "play_pause" )
{
    EngineController* const ec = EngineController::instance();

    engineStateChanged( ec->engine() ? ec->engine()->state() : EngineBase::Empty );

    ec->attach( this );
    connect( this, SIGNAL( activated() ), ec, SLOT( playPause() ) );
}

PlayPauseAction::~PlayPauseAction()
{
    EngineController::instance()->detach( this );
}

void
PlayPauseAction::engineStateChanged( EngineBase::EngineState state )
{
    switch( state )
    {
    case EngineBase::Playing:
        setIcon( "player_pause" );
        break;
    default:
        setIcon( "player_play" );
        break;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// AnalyzerAction
//////////////////////////////////////////////////////////////////////////////////////////
#include "blockanalyzer.h"

AnalyzerAction::AnalyzerAction( KActionCollection *ac )
  : KAction( i18n( "Analyzer" ), 0, ac, "toolbar_analyzer" )
{}

int
AnalyzerAction::plug( QWidget *w, int index )
{
    //NOTE the analyzer will be deleted when the toolbar is deleted or cleared()
    //we are not designed for unplugging() yet so there would be a leak if that happens
    //but it's a rare event and unplugging is complicated.

    KToolBar *bar = dynamic_cast<KToolBar*>(w);

    if( bar && kapp->authorizeKAction( name() ) )
    {
        const int id = KAction::getToolButtonID();

        addContainer( w, id );
        connect( w, SIGNAL( destroyed() ), SLOT( slotDestroyed() ) );

        QWidget *block = new BlockAnalyzer( w );
        block->setName( "ToolBarAnalyzer" );

        bar->insertWidget( id, 0, block, index );
        bar->setItemAutoSized( id, true );

        return containerCount() - 1;
    }
    else return -1;
}


//////////////////////////////////////////////////////////////////////////////////////////
// VolumeAction
//////////////////////////////////////////////////////////////////////////////////////////
#include "qslider.h"

VolumeAction::VolumeAction( KActionCollection *ac )
  : KAction( i18n( "Volume" ), 0, ac, "toolbar_volume" )
{}

int
VolumeAction::plug( QWidget *w, int index )
{
    amaroK::ToolBar *bar = dynamic_cast<amaroK::ToolBar*>( w );

    if( bar && kapp->authorizeKAction( name() ) )
    {
        EngineController::instance()->attach( this );
        
        const int id = KAction::getToolButtonID();
        addContainer( w, id );
        connect( w, SIGNAL( destroyed() ), SLOT( slotDestroyed() ) );

        m_slider = new QSlider( Qt::Vertical, w, "ToolBarVolume" );
        //FIXME is there a way to get some sensible height?
        m_slider->setFixedHeight( 35 ); 
        m_slider->setMaxValue( amaroK::VOLUME_MAX );
        connect( m_slider, SIGNAL( valueChanged( int ) ),
                 this,       SLOT( sliderMoved( int ) ) );
        connect( bar,      SIGNAL( wheelMoved( int ) ),
                 this,       SLOT( wheelMoved( int ) ) );

        bar->insertWidget( id, 0, m_slider, index );
                
        return containerCount() - 1;
    }
    else return -1;
}

void
VolumeAction::engineVolumeChanged( int value )
{
    m_slider->setValue( amaroK::VOLUME_MAX - value );
}

void
VolumeAction::sliderMoved( int value ) //SLOT
{
    EngineController::instance()->setVolume( amaroK::VOLUME_MAX - value );
    pApp->slotShowVolumeOsd();
}

void
VolumeAction::wheelMoved( int delta ) //SLOT
{
    m_slider->setValue( m_slider->value() - delta / 18 );
}


//////////////////////////////////////////////////////////////////////////////////////////
// RandomAction
//////////////////////////////////////////////////////////////////////////////////////////
RandomAction::RandomAction( KActionCollection *ac ) :
    ToggleAction( i18n( "Random &Mode" ), &AmarokConfig::setRandomMode, ac, "random_mode" )
{
    KToggleAction::setChecked( AmarokConfig::randomMode() );
}

//////////////////////////////////////////////////////////////////////////////////////////
// RepeatTrackAction
//////////////////////////////////////////////////////////////////////////////////////////
RepeatTrackAction::RepeatTrackAction( KActionCollection *ac ) :
    ToggleAction( i18n( "Repeat &Track" ), &AmarokConfig::setRepeatTrack, ac, "repeat_track" )
{
    KToggleAction::setChecked( AmarokConfig::repeatTrack() );
}

//////////////////////////////////////////////////////////////////////////////////////////
// RepeatPlaylistAction
//////////////////////////////////////////////////////////////////////////////////////////
RepeatPlaylistAction::RepeatPlaylistAction( KActionCollection *ac ) :
    ToggleAction( i18n( "Repeat &Playlist" ), &AmarokConfig::setRepeatPlaylist, ac, "repeat_playlist" )
{
    KToggleAction::setChecked( AmarokConfig::repeatPlaylist() );
}

#include "actionclasses.moc"
