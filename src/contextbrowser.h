// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information

#ifndef AMAROK_CONTEXTBROWSER_H
#define AMAROK_CONTEXTBROWSER_H

#include "engineobserver.h"
#include <qtabwidget.h>

class CollectionDB;
class Color;
class MetaBundle;

class QPalette;

class KHTMLPart;
class KTabBar;

namespace KIO { class Job; }

class ContextBrowser : public QTabWidget, public EngineObserver
{
    Q_OBJECT

        friend class CurrentTrackJob;

        static ContextBrowser *s_instance;

    public:
        ContextBrowser( const char *name );
       ~ContextBrowser();

        static ContextBrowser *instance() { return s_instance; }

        void setFont( const QFont& );
        void setStyleSheet();

    public slots:
        void openURLRequest(const KURL &url );
        void collectionScanStarted();
        void collectionScanDone();
        void renderView();

    protected:
        void engineNewMetaData( const MetaBundle&, bool );
        void engineStateChanged( Engine::State );
        void paletteChange( const QPalette& );

    private slots:
        void tabChanged( QWidget *page );
        void slotContextMenu( const QString& urlString, const QPoint& point );
        void showHome();
        void showCurrentTrack();
        void showLyrics( const QString& hash = QString::null );
        void showLyricSuggestions();

        void lyricsData( KIO::Job* job, const QByteArray& data );
        void lyricsResult( KIO::Job* job );
        void coverFetched( const QString &artist, const QString &album );
        void coverRemoved( const QString &artist, const QString &album );
        void similarArtistsFetched( const QString &artist );

    private:
        void setStyleSheet_Default( QString& styleSheet );
        void setStyleSheet_ExternalStyle( QString& styleSheet, QString& themeName );
        void showIntroduction();
        void saveHtmlData();
        void showScanning();

        KHTMLPart    *m_homePage;
        KHTMLPart    *m_currentTrackPage;
        KHTMLPart    *m_lyricsPage;
        // These control if is needed to rewrite the html for the pages
        // true -> need rebuild
        bool          m_dirtyHomePage;
        bool          m_dirtyCurrentTrackPage;
        bool          m_dirtyLyricsPage;

        QString       m_styleSheet;
        bool          m_emptyDB;
        QString       m_lyrics;
        QStringList   m_lyricSuggestions;
        QStringList   m_lyricHashes;
        QString       m_lyricAddUrl;
        QString       m_lyricSearchUrl;
        QString       m_HTMLSource;
        QStringList   m_metadataHistory;
        KURL          m_currentURL;

        bool          m_suggestionsOpen;
        bool          m_favouritesOpen;
};

#endif /* AMAROK_CONTEXTBROWSER_H */
