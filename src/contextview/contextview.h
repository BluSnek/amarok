/***************************************************************************
 * copyright     : (C) 2007 Seb Ruiz <ruiz@kde.org>                        *
 *                 (C) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com> *
 *                :(C) 2007 Leonardo Franchi  <lfranchi@gmail.com>         *
 *                :(C) 2007 Leonardo Franchi  <lfranchi@gmail.com>         *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_CONTEXTVIEW_H
#define AMAROK_CONTEXTVIEW_H

#include "contextbox.h"
#include "engineobserver.h"
#include "GenericInfoBox.h"

#include <kio/job.h>

#include <QGraphicsView>

class QGraphicsScene;
class QResizeEvent;
class QWheelEvent;

using namespace Context;

class ContextView : public QGraphicsView, public EngineObserver
{
    Q_OBJECT
    static ContextView *s_instance;

    public:

        static const int BOX_PADDING = 30;
        static const int WIKI_MAX_HISTORY = 10;
        ~ContextView() { /* delete, disconnect and disembark */ }

        static ContextView *instance()
        {
            if( !s_instance )
                return new ContextView();
            return s_instance;
        }

        void clear();

        void addContextBox( QGraphicsItem *newBox, int after = -1 /*which position to place the new box*/, bool fadeIn = false);

        void showLyrics( const QString& url );

    public slots:
        void lyricsResult( QByteArray cXmlDoc = 0, bool cached = false ) ;

    protected:
        void engineNewMetaData( const MetaBundle&, bool );
        void engineStateChanged( Engine::State, Engine::State = Engine::Empty );
        void resizeEvent( QResizeEvent *event );
        void wheelEvent( QWheelEvent *event );


    private:
        enum ShuffleDirection { ShuffleUp, ShuffleDown };
        /**
         * Creates a new context view widget with parent \p parent
         * Constructor is private since the view is a singleton class
         */
        ContextView();

        void initiateScene();

        void scaleView( qreal factor );

        void shuffleItems( QList<QGraphicsItem*> items, qreal distance, int direction );

        /// Page Views ////////////////////////////////////////
        void showHome();
        void showCurrentTrack();

        /// Wikipedia box /////////////////////////////////////
        QString wikiArtistPostfix();
        QString wikiAlbumPostfix();
        QString wikiTrackPostfix();
        QString wikiLocale();
        void setWikiLocale( const QString& );
        QString wikiURL( const QString& item );
        void reloadWikipedia();
        void showWikipediaEntry( const QString& entry, bool replaceHistory = false );
        void showWikipedia( const QString& url = QString(), bool fromHistory = false, bool replaceHistory = false );

        /// Attributes ////////////////////////////////////////
        QGraphicsScene *m_contextScene; ///< Pointer to the scene which holds all our items

        /// Lyrics box attributes /////////////////////////////
        GenericInfoBox *m_lyricsBox;

        bool            m_dirtyLyricsPage;
        bool            m_lyricsVisible;
        QString         m_HTMLSource;

        /// Wikipedia box attributes ///////////////////////////
        GenericInfoBox *m_wikiBox;

        KJob* m_wikiJob;
        QString m_wikiCurrentEntry;
        QString m_wikiCurrentUrl;
        QString m_wikiBaseUrl;
        bool m_dirtyWikiPage;
        bool m_wikiVisible;
        QString m_wikiHTMLSource;
        QString m_wikiLanguages;

        QString m_wiki; // wiki source

        QStringList m_wikiBackHistory;
        QStringList m_wikiForwardHistory;

        static QString s_wikiLocale;
    private slots:

        void introAnimationComplete();
        void testBoxLayout();

        /// Wikipedia slots
    /*void wikiConfigChanged( int );
        void wikiConfigApply();
        void wikiConfig(); */
        void wikiArtistPage();
        void wikiAlbumPage();
        void wikiTitlePage();
        void wikiExternalPage();

        void wikiResult( KJob* job );

};

#endif
