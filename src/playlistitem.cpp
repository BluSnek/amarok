/***************************************************************************
                        playlistitem.cpp  -  description
                           -------------------
  begin                : Die Dez 3 2002
  copyright            : (C) 2002 by Mark Kretschmann
  email                : markey@web.de
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "amarokconfig.h"
#include <cmath>
#include "debug.h"
#include "playlist.h"
#include "collectiondb.h"
#include <kfilemetainfo.h>
#include <kiconloader.h>
#include <kstringhandler.h>
#include "metabundle.h"
#include "playlistitem.h"
#include <qpainter.h>
#include <qpen.h>
#include <qpixmap.h>
#include <qrect.h>

QColor PlaylistItem::glowText = Qt::white;
QColor PlaylistItem::glowBase = Qt::white;
bool   PlaylistItem::s_pixmapChanged = false;


/// These are untranslated and used for storing/retrieving XML playlist
const QString PlaylistItem::columnName( int c ) //static
{
    switch( c ) {
        case TrackName: return "TrackName";
        case Title:     return "Title";
        case Artist:    return "Artist";
        case Album:     return "Album";
        case Year:      return "Year";
        case Comment:   return "Comment";
        case Genre:     return "Genre";
        case Track:     return "TrackNo";
        case Directory: return "Directory";
        case Length:    return "Length";
        case Bitrate:   return "Bitrate";
        case Score:     return "Score";
        case Extension:    return "Extension";
        case Playcount:    return "Playcount";
    }
    return "<ERROR>";
}


//statics
QString PlaylistItem::stringStore[STRING_STORE_SIZE];


PlaylistItem::PlaylistItem( QListView *listview, QListViewItem *item )
        : KListViewItem( listview, item )
{
    setVisible( false );
}

PlaylistItem::PlaylistItem( const MetaBundle &bundle, QListViewItem *lvi )
        : KListViewItem( lvi->listView(), lvi->itemAbove(), trackName( bundle.url() ) )
        , m_url( bundle.url() )
        , m_missing( false )
        , m_enabled( true )
{
    setDragEnabled( true );

    setText( bundle );
}

PlaylistItem::PlaylistItem( QDomNode node, QListViewItem *item )
        : KListViewItem( item->listView(), item->itemAbove() )
        , m_url( node.toElement().attribute( "url" ) )
        , m_missing( false )
        , m_enabled( true )
{
    setDragEnabled( true );
    KListViewItem::setText( TrackName, trackName( m_url ) );

    //NOTE we use base versions to speed this up (this function is called 100s of times during startup)
    for( uint x = 1, n = listView()->columns(); x < n; ++x ) {
        const QString text = node.namedItem( columnName( x ) ).toElement().text();

        switch( x ) {
        case Artist:
        case Album:
        case Genre:
        case Year:
        case Directory:
            KListViewItem::setText( x, attemptStore( text ) );
            continue;
        case Score:
            KListViewItem::setText( x,
                    QString::number( CollectionDB::instance()->getSongPercentage( m_url.path() ) ) );
            continue;
        case Playcount:
            KListViewItem::setText( x,
                    QString::number( CollectionDB::instance()->getPlayCount( m_url.path() ) ) );
            continue;
        case Extension:
        default:
            KListViewItem::setText( x, text );
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
/////////////////////////////////////////////////////////////////////////////////////


QString PlaylistItem::text( int column ) const
{
    //if there is no text set for the title, return a pretty version of the track name

    if( column == Title && KListViewItem::text( Title ).isEmpty()
            // this is important, as we don't want to show the trackname twice
            && listView()->columnWidth( TrackName ) == 0 )
    {
        return MetaBundle::prettyTitle( KListViewItem::text( TrackName ) );
    }

    return KListViewItem::text( column );
}


QString
PlaylistItem::seconds() const
{
    QString length = exactText( Length );

    if( length == "?" ) return QString();
    if( length == "-" ) length += '1';
    else if( !length.isEmpty() )
    {
        int m = length.section( ':', 0, 0 ).toInt();
        int s = length.section( ':', 1, 1 ).toInt();

        length.setNum( m * 60 + s );
    }

    return length;
}

void PlaylistItem::setEnabled( bool enabled )
{
    m_enabled = enabled;
    setDropEnabled( enabled ); // this forbids items to be dropped into a history queue.

    repaint();
}

void PlaylistItem::setText( const MetaBundle &bundle )
{
    setText( Title,     bundle.title() );
    setText( Artist,    bundle.artist() );
    setText( Album,     bundle.album() );
    setText( Year,      bundle.year() );
    setText( Comment,   bundle.comment() );
    setText( Genre,     bundle.genre() );
    setText( Track,     bundle.track() );
    setText( Directory, bundle.url().isLocalFile() ? bundle.url().directory() : bundle.prettyURL() );
    setText( Length,    bundle.prettyLength() );
    setText( Bitrate,   bundle.prettyBitrate() );
    setText( Extension,    bundle.fileExtension() );

    m_missing = !bundle.exists();

    const int score = CollectionDB::instance()->getSongPercentage( bundle.url().path() );
    if ( score )
        setText( Score, QString::number( score ) );

    const int playcount = CollectionDB::instance()->getPlayCount( bundle.url().path() );
    if ( playcount )
        setText( Playcount, QString::number( playcount ) );
    else
          setText( Playcount, QString::number( 0 ) );  //Never played before.
}


void PlaylistItem::setText( int column, const QString &newText )
{
    //NOTE prettyBitrate() is special and the returned string should not be modified
    //     as it is implicately shared for the common values in class MetaBundle
    //NOTE track() may also be special

    switch( column ) {
    case Artist:
    case Album:
    case Genre:
    case Directory:

        //these are good candidates for the stringStore
        //NOTE title is not a good candidate, it probably will never repeat in the playlist
        KListViewItem::setText( column, attemptStore( newText ) );
        break;

    case Length:
        //TODO consider making this a dynamically generated string
        KListViewItem::setText( Length, newText.isEmpty() ? newText : newText + ' ' ); //padding makes it neater
        break;

    case Track:
    case Year:
        KListViewItem::setText( column, newText == "0" ? QString::null : attemptStore( newText ) );
        break;

     case Extension:
     case Playcount:

    default:
        KListViewItem::setText( column, newText );
        break;
    }
}

bool
PlaylistItem::operator== ( const PlaylistItem & item ) const
{
    return item.url() == this->url();
}

bool
PlaylistItem::operator< ( const PlaylistItem & item ) const
{
    return item.url() < this->url();
}

/////////////////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
/////////////////////////////////////////////////////////////////////////////////////

int
PlaylistItem::compare( QListViewItem *i, int col, bool ascending ) const
{
    QString a =    text( col ).lower();
    QString b = i->text( col ).lower();

    switch( col )  //we must pad numbers to sort them lexically, so we must special case those columns
    {
        case Track:
        case Score:
        case Length:
        case Extension:
        case Playcount:
        case Bitrate:
            a = a.rightJustify( b.length(), '0' ); //all these columns shouldn't become negative
            b = b.rightJustify( a.length(), '0' ); //so simply left-padding is sufficient
            break;

        case Year:
            if( a == b )
                return this->compare( i, Artist, ascending );
            break;

        case Artist:
            if( a == b ) //if same artist, try to sort by album
                return this->compare( i, Album, ascending );
            break;

        case Album:
            if( a == b ) //if same album, try to sort by track
                //TODO only sort in ascending order?
                return this->compare( i, Track, true ) * (ascending ? 1 : -1);
            break;

        default:;
    }

    return QString::localeAwareCompare( a, b );
}

void PlaylistItem::paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align )
{
    //TODO add spacing on either side of items
    //p->translate( 2, 0 ); width -= 3;

    const int playNext = listView()->m_nextTracks.findRef( this ) + 1;

    if( this == listView()->currentTrack() && !isSelected() )
    {
        static paintCacheItem paintCache[NUM_COLUMNS];

        // Convert QColor to string for use as key in QMap
        const QString colorKey =
            QString::number( glowBase.red() ) +
            QString::number( glowBase.green() ) +
            QString::number( glowBase.blue() );

        const bool cacheValid =
            paintCache[column].width == width &&
            paintCache[column].height == height() &&
            paintCache[column].text == text( column ) &&
            paintCache[column].font == p->font() &&
            !s_pixmapChanged;

        // If any parameter changed, we must regenerate all pixmaps
        if ( !cacheValid )
        {
            paintCache[column].map.clear();
            //So we don't regenerate all pixmap without really having to do it.
            s_pixmapChanged = false;
        }

        // Determine if we need to repaint the pixmap, or paint from cache
        if ( paintCache[column].map.find( colorKey ) == paintCache[column].map.end() )
        {
            // Update painting cache
            paintCache[column].width = width;
            paintCache[column].height = height();
            paintCache[column].text = text( column );
            paintCache[column].font = p->font();
            paintCache[column].map[colorKey] = QPixmap( width, height() );

            // Don't try to draw if width or height is 0, as this crashes Qt
            if ( paintCache[column].map[colorKey].isNull() ) return;

            QPainter paint( &paintCache[column].map[colorKey], true );

            // Here we draw the shaded background
            int h, s, v;
            glowBase.getHsv( &h, &s, &v );
            QColor col;

            for ( int i = 0; i < height(); i++ ) {
                col.setHsv( h, s, static_cast<int>( sin( (float)i / ( (float)height() / 4 ) ) * 32.0 + 196 ) );
                paint.setPen( col );
                paint.drawLine( 0, i, width, i );
            }

            // Draw the pixmap, if present
            int leftMargin = 1;
            if ( pixmap( column ) ) {
                paint.drawPixmap( leftMargin, height() / 2 - pixmap( column )->height() / 2, *pixmap( column ) );
                leftMargin += pixmap( column )->width() - 2; //-1 seems to be required
            }

            if( align != Qt::AlignCenter )
               align |= Qt::AlignVCenter;

            // Draw the text
            paint.setFont( p->font() );
            paint.setPen( glowText );
            const QString _text = KStringHandler::rPixelSqueeze( text( column ), p->fontMetrics(), width - 5 );
            paint.drawText( leftMargin, 0, width, height(), align, _text );

            paint.end();
        }

        p->drawPixmap( 0, 0, paintCache[column].map[colorKey] );
    }
    else {
        QColorGroup _cg = cg;
        //FIXME not acceptable to hardocode the colour
        QColor disabledText = QColor( 172, 172, 172 );
        if( m_missing || !m_enabled )
            _cg.setColor( QColorGroup::Text, disabledText );

        KListViewItem::paintCell( p, _cg, column, width, align );
    }

    // Here we draw the "Play as next" symbol:

    //figure out if we are in the actual physical first column
    if( playNext && column == listView()->m_firstColumn )
    {
        QString str = QString::number( playNext );

        //draw the symbol's outline
              uint fw = p->fontMetrics().width( str ) + 2;
        const uint w  = 16; //keep this even
        const uint h  = height() - 2;

        p->setBrush( cg.highlight() );
        p->setPen( cg.highlight().dark() ); //TODO blend with background color
        p->drawEllipse( width - fw - w/2, 1, w, h );
        p->drawRect( width - fw, 1, fw, h );
        p->setPen( cg.highlight() );
        p->drawLine( width - fw, 2, width - fw, h - 1 );

        //draw the shadowed inner text
        //NOTE we can't set an arbituary font size or family, these settings are already optional
        //and user defaults should also take presidence if no playlist font has been selected
        //const QFont smallFont( "Arial", (playNext > 9) ? 9 : 12 );
        //p->setFont( smallFont );
        //TODO the shadow is hard to do well when using a dark font color
        //TODO it also looks cluttered for small font sizes
        //p->setPen( cg.highlightedText().dark() );
        //p->drawText( width - w + 2, 3, w, h-1, Qt::AlignCenter, str );
        fw += 2; //add some more padding
        p->setPen( cg.highlightedText() );
        p->drawText( width - fw, 2, fw, h-1, Qt::AlignCenter, str );
    }

    if( !isSelected() )
    {
        p->setPen( QPen( cg.mid(), 0, Qt::SolidLine ) );
        p->drawLine( width - 1, 0, width - 1, height() - 1 );
    }
}


void PlaylistItem::setup()
{
    KListViewItem::setup();

    if( this == listView()->currentTrack() )
        setHeight( listView()->fontMetrics().height() * 2 );
}


const QString &PlaylistItem::attemptStore( const QString &candidate ) //static
{
    //principal is to cause collisions at reasonable rate to reduce memory
    //consumption while not using such a big store that it is mostly filled with empty QStrings
    //because collisions are so rare

    if( candidate.isEmpty() ) return candidate; //nothing to try to share

    const uchar hash = candidate[0].unicode() % STRING_STORE_SIZE;


    if( stringStore[hash] != candidate ) //then replace
    {
        stringStore[hash] = candidate;
    }

    return stringStore[hash];
}
