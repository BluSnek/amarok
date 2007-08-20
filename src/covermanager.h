
// (c) Pierpaolo Di Panfilo 2004
// See COPYING file for licensing information

#ifndef COVERMANAGER_H
#define COVERMANAGER_H

#include "coverfetcher.h"

#include <q3ptrlist.h>
#include <QSplitter>
//Added by qt3to4:
#include <QDropEvent>
#include <Q3ValueList>
#include <QLabel>
#include <k3iconview.h>
#include <QDialog>
#include <QPixmap>

class Q3ListViewItem;
class CoverViewItem;
class KLineEdit;
class KPushButton;
class KMenu;
class QToolButton;
class QLabel;
class K3ListView;
class CoverView;
class KHBox;
class QProgressBar;
class QHBoxLayout;
class PixmapViewer;

class CoverManager : public QSplitter
{
        Q_OBJECT

        static CoverManager *s_instance;

    public:
        CoverManager();
       ~CoverManager();

        static CoverManager *instance() { return s_instance; }

        static void showOnce( const QString &artist = QString() );
        static void viewCover( const QString& artist, const QString& album, QWidget *parent=0 );

        void setStatusText( QString text );

        // Return the top level domain for the current locale
        static QString amazonTld();
    
    public slots:
        void updateStatusBar();
        void changeLocale( int id );

    private slots:
        void init();
        
        void slotArtistSelected( Q3ListViewItem* );
        void coverItemExecuted( Q3IconViewItem *item );
        void showCoverMenu( Q3IconViewItem *item, const QPoint& );
        void slotSetFilter();
        void slotSetFilterTimeout();
        
        void slotShowAllAlbums()          { changeView( AllAlbums );          }
        void slotShowAlbumsWithCover()    { changeView( AlbumsWithCover );    }
        void slotShowAlbumsWithoutCover() { changeView( AlbumsWithoutCover ); }
        void changeView( int id );

        void slotSetLocaleIntl() { changeLocale( CoverFetcher::International ); }
        void slotSetLocaleCa()   { changeLocale( CoverFetcher::Canada );        }
        void slotSetLocaleDe()   { changeLocale( CoverFetcher::Germany );       }
        void slotSetLocaleFr()   { changeLocale( CoverFetcher::France );        }
        void slotSetLocaleJp()   { changeLocale( CoverFetcher::Japan );         }
        void slotSetLocaleUk()   { changeLocale( CoverFetcher::UK );            }
        
        void fetchMissingCovers();
        void fetchCoversLoop();
        void coverFetched( const QString&, const QString& );
        void coverRemoved( const QString&, const QString& );
        void coverFetcherError();
        void stopFetching();


        void setCustomSelectedCovers();
        void fetchSelectedCovers();
        void deleteSelectedCovers();
        void viewSelectedCover();
        void playSelectedAlbums();

    private:
        enum View { AllAlbums=0, AlbumsWithCover, AlbumsWithoutCover };

        void loadCover( const QString &, const QString & );
        Q3PtrList<CoverViewItem> selectedItems();

        K3ListView     *m_artistView;
        CoverView      *m_coverView;
        KLineEdit      *m_searchEdit;
        KPushButton    *m_fetchButton;
        KMenu          *m_amazonLocaleMenu;
        KMenu          *m_viewMenu;
        QToolButton    *m_amazonLocaleButton;
        QToolButton    *m_viewButton;
        int             m_currentLocale;
        int             m_currentView;

        //status bar widgets
        QLabel         *m_statusLabel;
        KHBox          *m_progressBox;
        QProgressBar   *m_progress;
        QString         m_oldStatusText;

        QTimer         *m_timer;              //search filter timer
        Q3PtrList<Q3IconViewItem> m_coverItems; //used for filtering
        QString         m_filter;


        // Used by fetchCoversLoop() for temporary storage
        QStringList m_fetchCovers;
        uint m_fetchCounter;

        //used to display information about cover fetching in the status bar
        int m_fetchingCovers;
        int m_coversFetched;
        int m_coverErrors;
};

class CoverView : public K3IconView
{
        Q_OBJECT

    public:
        explicit CoverView( QWidget *parent = 0, const char *name = 0, Qt::WFlags f = 0 );

    protected:
        Q3DragObject *dragObject();

    private slots:
        void setStatusText( Q3IconViewItem *item );
};

class CoverViewItem : public K3IconViewItem
{
    public:
        CoverViewItem( Q3IconView *parent, Q3IconViewItem *after, const QString &artist, const QString &album );

        void loadCover();
        bool hasCover() const;
        bool canRemoveCover() const { return !m_embedded && hasCover(); }
        QString artist() const { return m_artist; }
        QString album() const { return m_album; }
        QPixmap coverPixmap() const { return m_coverPixmap; }

    protected:
        void paintItem(QPainter* painter, const QColorGroup& colorGroup);
        void paintFocus(QPainter *, const QColorGroup &) { }
        void dropped( QDropEvent *, const Q3ValueList<Q3IconDragItem> & );
        void dragEntered();
        void dragLeft();
        void calcRect( const QString& text_ = QString() );

    private:
        QString m_artist;
        QString m_album;
        QString m_coverImagePath;
        QPixmap m_coverPixmap;
        bool    m_embedded;
};


class CoverViewDialog : public QDialog {
        Q_OBJECT

    public:
        CoverViewDialog(const QString& artist, const QString& album, QWidget *parent);

    private:
        QHBoxLayout *m_layout;
        QPixmap m_pixmap;
        PixmapViewer *m_pixmapViewer;
        QLabel *m_label;
};

#endif
