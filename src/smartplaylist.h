// (c) Pierpaolo Di Panfilo 2004
// See COPYING file for licensing information

#ifndef SMARTPLAYLIST_H
#define SMARTPLAYLIST_H

#include <klistview.h>
#include <kurl.h>
#include <qvbox.h>

class SmartPlaylist;
class QPainter;
class QRect;

class SmartPlaylistBox : public QVBox
{
    public:
        SmartPlaylistBox( QWidget *parent, const char *name = 0 );
};


class SmartPlaylistView : public KListView
{
Q_OBJECT
    public:
        SmartPlaylistView( QWidget *parent, const char *name = 0 );
       ~SmartPlaylistView();

       SmartPlaylist *getSmartPlaylist( QString name );
       ///Returns a qstringlist of all every smartplaylist (including item children)
       QStringList    listItems();
       static SmartPlaylistView *instance() { return s_instance; }

    public slots:
        void createCustomPlaylist();
        void removeSelectedPlaylists();

    protected:
        virtual class QDragObject *dragObject();
        virtual void viewportPaintEvent( QPaintEvent* );

    private slots:
        void makePlaylist( QListViewItem* );
        void showContextMenu( QListViewItem *item, const QPoint &p, int );
        void collectionScanDone();

    private:
        void loadDefaultPlaylists();
        void loadCustomPlaylists();
        QString customPlaylistsFile();

        bool m_loaded;

        static SmartPlaylistView *s_instance;
};


class SmartPlaylist : public KListViewItem
{
    public:
        SmartPlaylist( const QString &name, const QString &query, KListView *parent );
        SmartPlaylist( const QString &name, const QString &query, SmartPlaylist *parent );

        void setCustom( bool b ) { m_custom = b; setDragEnabled( true ); }
        bool isCustom() const { return m_custom; }

        /// used for sorting
        void setKey( int pos ) { m_key = pos; }
        QString key( int c, bool ) const;

        QString query() { return isCustom() ? sqlForUrls : sqlForTags; }

        KURL::List urls() const;

        QString sqlForUrls;
        QString sqlForTags;

    private:
        bool m_custom;
        int  m_key;
};

#endif
