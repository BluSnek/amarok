/***************************************************************************
  begin                : Fre Nov 15 2002
  copyright            : (C) Mark Kretschmann <markey@web.de>
                       : (C) Max Howell <max.howell@methylblue.com>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_PLAYLISTWINDOW_H
#define AMAROK_PLAYLISTWINDOW_H

#include "amarok_export.h"
#include "widgets/MainToolbar.h"

#include <khbox.h>          //baseclass for DynamicBox
#include <kxmlguiwindow.h>
#include <kxmlguiclient.h>  //baseclass (for XMLGUI)

class KMenu;
class MediaBrowser;
class QMenuBar;
class QTimer;
class SearchWidget;
class SideBar;
class QSplitter;
class PlaylistFileProvider;

/**
  * @class MainWindow
  * @short The MainWindow widget class.
  *
  * This is the main window widget (the Playlist not Player).
  */
class AMAROK_EXPORT MainWindow : public KXmlGuiWindow
{
    Q_OBJECT

    public:
        MainWindow();
       ~MainWindow();

        void init();

        //allows us to switch browsers from within other browsers etc
        void showBrowser( const QString& name );
        void addBrowser( const QString &name, QWidget *widget, const QString &text, const QString &icon );

        //takes into account minimized, multiple desktops, etc.
        bool isReallyShown() const;

        virtual bool eventFilter( QObject*, QEvent* );

        //instance is declared in KXMLGUI
        static MainWindow *self() { return s_instance; }

        void activate();

        SideBar *sideBar() const { return m_browsers; }
        void deleteBrowsers();

    signals:
        void loveTrack( Meta::TrackPtr );

    public slots:
        void showHide();
        void loveTrack();

    private slots:
        void slotShrinkBrowsers( int index ) const;
        void savePlaylist() const;
        void slotBurnPlaylist() const;
        void slotShowCoverManager() const;
        void slotPlayMedia();
        void slotAddLocation( bool directPlay = false );
        void slotAddStream();
        // TODO: reimplement with last.fm service
#if 0
        void playLastfmPersonal();
        void addLastfmPersonal();
        void playLastfmNeighbor();
        void addLastfmNeighbor();
        void playLastfmCustom();
        void addLastfmCustom();
        void playLastfmGlobaltag();
        void addLastfmGlobaltag();
#endif
        void playAudioCD();
        void showQueueManager();
        void showScriptSelector();
        void showStatistics();
        void toolsMenuAboutToShow();
        void slotToggleFocus();
//         void slotEditFilter();
        void slotSetFilter( const QString &filter );

    protected:
        virtual void closeEvent( QCloseEvent* );
        virtual void showEvent( QShowEvent* );
        virtual QSize sizeHint() const;

        virtual void paletteChange( const QPalette & oldPalette );

    private slots:
        void setRating1() { setRating( 1 ); }
        void setRating2() { setRating( 2 ); }
        void setRating3() { setRating( 3 ); }
        void setRating4() { setRating( 4 ); }
        void setRating5() { setRating( 5 ); }

    private:
        void setRating( int n );

        QMenuBar      *m_menubar;
        KMenu         *m_toolsMenu;
        KMenu         *m_settingsMenu;
        SideBar       *m_browsers;
        QStringList    m_browserNames;
        KMenu         *m_searchMenu;

        SearchWidget  *m_searchWidget;
        MainToolbar    *m_controlBar;
        QTimer        *m_timer;  //search filter timer
        QStringList    m_lastfmTags;
        MediaBrowser  *m_currMediaBrowser;
        QSplitter     *m_splitter;

        PlaylistFileProvider *m_playlistFiles;

        void    createActions();
        void    createMenus();
        int m_lastBrowser;
        int m_searchField;
        static MainWindow *s_instance;
};


#endif //AMAROK_PLAYLISTWINDOW_H
