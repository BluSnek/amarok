
/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/



#include "MagnatuneMeta.h"
#include "MagnatuneStore.h"

#include "Amarok.h"
#include "debug.h"

#include "debug.h"


#include <KIcon>
#include <KLocale>
#include <KStandardDirs>

#include <QObject>

using namespace Meta;

MagnatuneMetaFactory::MagnatuneMetaFactory( const QString & dbPrefix, MagnatuneStore * store )
    : ServiceMetaFactory( dbPrefix )
    , m_membershipPrefix( QString() )
    , m_streamType( OGG )
    , m_userName( QString() )
    , m_password( QString() )
    , m_store( store )
{
}

void MagnatuneMetaFactory::setMembershipInfo( QString prefix, QString userName, QString password )
{
    m_membershipPrefix = prefix;
    m_userName = userName;
    m_password = password;
}

void MagnatuneMetaFactory::setStreamType(int type)
{
    m_streamType = type;
}



int MagnatuneMetaFactory::getTrackSqlRowCount()
{
   return ServiceMetaFactory::getTrackSqlRowCount() + 2;
}

QString MagnatuneMetaFactory::getTrackSqlRows()
{
    DEBUG_BLOCK
    QString sqlRows = ServiceMetaFactory::getTrackSqlRows();

    sqlRows += ", ";
    sqlRows += tablePrefix() + "_tracks.preview_lofi, ";
    sqlRows += tablePrefix() + "_tracks.preview_ogg ";

    return sqlRows;
}


TrackPtr MagnatuneMetaFactory::createTrack(const QStringList & rows)
{

    MagnatuneTrack * track = new MagnatuneTrack( rows );


    if ( m_streamType == OGG ) {
        track->setUrl( track->oggUrl() );
    } else if (  m_streamType == LOFI ) {
        track->setUrl( track->lofiUrl() );
    }

    if ( !m_membershipPrefix.isEmpty() ) {
        QString url = track->url();
        url.replace( "http://he3.", "http://" + m_userName + ":" + m_password + "@" + m_membershipPrefix + "." );
        
        if ( m_streamType == MP3 ) {
            url.replace( ".mp3", "_nospeech.mp3" );
        }  else if ( m_streamType == OGG ) {
            url.replace( ".ogg", "_nospeech.ogg" );
        }
        
        track->setUrl( url );

        debug()  << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!PREFIX: " << m_membershipPrefix;
        
        if ( m_membershipPrefix == "download" )
            track->setDownloadMembership();
    }

    return TrackPtr( track );
}

int MagnatuneMetaFactory::getAlbumSqlRowCount()
{
    return ServiceMetaFactory::getAlbumSqlRowCount() + 3;
}

QString MagnatuneMetaFactory::getAlbumSqlRows()
{
    DEBUG_BLOCK
    QString sqlRows = ServiceMetaFactory::getAlbumSqlRows();

    sqlRows += ", ";
    sqlRows += tablePrefix() + "_albums.cover_url, ";
    sqlRows += tablePrefix() + "_albums.year, ";
    sqlRows += tablePrefix() + "_albums.album_code ";


    return sqlRows;
}

AlbumPtr MagnatuneMetaFactory::createAlbum(const QStringList & rows)
{
    MagnatuneAlbum * album = new MagnatuneAlbum( rows );
    album->setStore( m_store );

    if ( m_membershipPrefix == "download" )
        album->setDownloadMembership();

    return AlbumPtr( album );
}

int MagnatuneMetaFactory::getArtistSqlRowCount()
{
    return ServiceMetaFactory::getArtistSqlRowCount() + 2;
}

QString MagnatuneMetaFactory::getArtistSqlRows()
{
    DEBUG_BLOCK
    QString sqlRows = ServiceMetaFactory::getArtistSqlRows();

    sqlRows += ", ";
    sqlRows += tablePrefix() + "_artists.photo_url, ";
    sqlRows += tablePrefix() + "_artists.artist_page ";

    return sqlRows;
}

ArtistPtr MagnatuneMetaFactory::createArtist(const QStringList & rows)
{
    return ArtistPtr( new MagnatuneArtist( rows ) );
}


GenrePtr MagnatuneMetaFactory::createGenre(const QStringList & rows)
{
    return GenrePtr( new MagnatuneGenre( rows ) );
}


//// MagnatuneTrack ////

MagnatuneTrack::MagnatuneTrack( const QString &name )
    : ServiceTrack( name )
    , m_downloadMembership ( false )
    , m_purchaseCustomAction( 0 )
    , m_purchaseCurrentTrackAction( 0 )
    , m_showInServiceAction( 0 )
{
}

MagnatuneTrack::MagnatuneTrack(const QStringList & resultRow)
    : ServiceTrack( resultRow )
    , m_downloadMembership ( false )
    , m_purchaseCustomAction( 0 )
    , m_purchaseCurrentTrackAction( 0 )
    , m_showInServiceAction( 0 )
{
    DEBUG_BLOCK
    m_lofiUrl = resultRow[7];
    m_oggUrl = resultRow[8];
}

QString MagnatuneTrack::lofiUrl()
{
    return m_lofiUrl;
}

void MagnatuneTrack::setLofiUrl(const QString & url)
{
    m_lofiUrl = url;
}

QString Meta::MagnatuneTrack::oggUrl() const
{
    return m_oggUrl;
}


void Meta::MagnatuneTrack::setOggUrl( const QString& url )
{
    m_oggUrl = url;
}

void Meta::MagnatuneTrack::setDownloadMembership()
{
    m_downloadMembership = true;
}


QList< QAction * > Meta::MagnatuneTrack::customActions()
{
    DEBUG_BLOCK
    QList< QAction * > actions;

    QString text = i18n( "&Buy" );
    if ( m_downloadMembership )
        text = i18n( "&Download" );

    if ( !m_purchaseCustomAction ) {
        m_purchaseCustomAction = new QAction( KIcon("get-hot-new-stuff-amarok" ), text, 0 );
        MagnatuneAlbum * mAlbum = static_cast<MagnatuneAlbum *> ( album().data() );
        QObject::connect( m_purchaseCustomAction, SIGNAL( activated() ), mAlbum->store(), SLOT( purchase() ) );
    }

    actions.append( m_purchaseCustomAction );
    return actions;
}

QList< QAction * > Meta::MagnatuneTrack::currentTrackActions()
{

    DEBUG_BLOCK
    QList< QAction * > actions;

    QString text = i18n( "Magnatune.com: &Buy" );
    if ( m_downloadMembership )
        text = i18n( "Magnatune.com: &Download" );

    if ( !m_purchaseCurrentTrackAction ) {
        m_purchaseCurrentTrackAction = new QAction( KIcon("get-hot-new-stuff-amarok" ), text, 0 );
        MagnatuneAlbum * mAlbum = static_cast<MagnatuneAlbum *> ( album().data() );
        QObject::connect( m_purchaseCurrentTrackAction, SIGNAL( activated() ), mAlbum->store(), SLOT( purchaseCurrentTrackAlbum() ) );
    }

    actions.append( m_purchaseCurrentTrackAction );

    if ( !m_showInServiceAction ) {

        MagnatuneAlbum * malbum = dynamic_cast<MagnatuneAlbum *> ( album().data() );

        if ( malbum )
            m_showInServiceAction = new ShowInServiceAction( malbum->store(), this );
    }

    actions.append( m_showInServiceAction );
    return actions;

}


QString Meta::MagnatuneTrack::sourceName()
{
    return "Magnatune.com";
}

QString Meta::MagnatuneTrack::sourceDescription()
{
    return "The non evil record label that is fair to artists and customers alike";
}

QPixmap Meta::MagnatuneTrack::emblem()
{
    return QPixmap( KStandardDirs::locate( "data", "amarok/images/emblem-magnatune.png" ) );
}


QList< QString > Meta::MagnatuneTrack::moods()
{
    return m_moods;
}

void Meta::MagnatuneTrack::setMoods(QList< QString > moods)
{
    m_moods = moods;
}


//// MagnatuneArtist ////

MagnatuneArtist::MagnatuneArtist( const QString &name )
    : ServiceArtist( name )
{
}

MagnatuneArtist::MagnatuneArtist(const QStringList & resultRow)
    : ServiceArtist( resultRow )
{
    m_photoUrl = resultRow[3];
    m_magnatuneUrl = resultRow[4];


}

void MagnatuneArtist::setPhotoUrl( const QString &photoUrl )
{
    m_photoUrl = photoUrl;
}

QString MagnatuneArtist::photoUrl( ) const
{
    return m_photoUrl;
}

void MagnatuneArtist::setMagnatuneUrl( const QString & magnatuneUrl )
{
    m_magnatuneUrl = magnatuneUrl;
}

QString MagnatuneArtist::magnatuneUrl() const
{
    return m_magnatuneUrl;
}




//// MagnatuneAlbum ////

MagnatuneAlbum::MagnatuneAlbum( const QString &name )
    : ServiceAlbumWithCover( name )
    , m_coverUrl()
    , m_launchYear( 0 )
    , m_albumCode()
    , m_store ( 0 )
    , m_downloadMembership ( false )

{
}

MagnatuneAlbum::MagnatuneAlbum(const QStringList & resultRow)
    : ServiceAlbumWithCover( resultRow )
    , m_downloadMembership ( false )
{
    debug() << "create album from result row: " << resultRow;


    m_coverUrl = resultRow[4];
    m_launchYear = resultRow[5].toInt();
    m_albumCode = resultRow[6];

    m_store = 0;
    

}

MagnatuneAlbum::~ MagnatuneAlbum()
{
}


void MagnatuneAlbum::setLaunchYear( int launchYear )
{
    m_launchYear = launchYear;
}

int MagnatuneAlbum::launchYear( ) const
{
    return m_launchYear;
}

void MagnatuneAlbum::setAlbumCode(const QString & albumCode)
{
    m_albumCode = albumCode;
}

QString MagnatuneAlbum::albumCode()
{
    return m_albumCode;

}

void MagnatuneAlbum::setCoverUrl(const QString & coverUrl)
{
    m_coverUrl = coverUrl;
}

QString MagnatuneAlbum::coverUrl() const
{
    return m_coverUrl;
}

void Meta::MagnatuneAlbum::setStore(MagnatuneStore * store)
{
    m_store = store;
}

MagnatuneStore * Meta::MagnatuneAlbum::store()
{
    return m_store;
}

void Meta::MagnatuneAlbum::setDownloadMembership()
{
    DEBUG_BLOCK
    m_downloadMembership = true;
}

QList< QAction * > MagnatuneAlbum::customActions()
{
    DEBUG_BLOCK
    QList< QAction * > actions;

    QString text = i18n( "&Buy" );
    if ( m_downloadMembership )
        text = i18n( "&Download" );

    QAction * action = new QAction( KIcon("get-hot-new-stuff-amarok" ), text, 0 );

    QObject::connect( action, SIGNAL( activated() ) , m_store, SLOT( purchase() ) );

    actions.append( action );
    return actions;
}






MagnatuneGenre::MagnatuneGenre( const QString & name )
    : ServiceGenre( name )
{
}

MagnatuneGenre::MagnatuneGenre( const QStringList & resultRow )
    : ServiceGenre( resultRow )
{
}

