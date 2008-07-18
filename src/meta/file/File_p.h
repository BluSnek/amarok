/*
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
   Copyright (C) 2008 Peter ZHOU         <peterzhoulei@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef AMAROK_META_FILE_P_H
#define AMAROK_META_FILE_P_H

#include "charset-detector/include/chardet.h"
#include "Debug.h"
#include "Meta.h"
#include "MetaUtility.h"

#include <QFile>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QString>

#include <KLocale>

// Taglib Includes
#include <fileref.h>
#include <tag.h>
#include <flacfile.h>
#include <id3v1tag.h>
#include <id3v2tag.h>
#include <mpcfile.h>
#include <mpegfile.h>
#include <oggfile.h>
#include <oggflacfile.h>
#include <tlist.h>
#include <tstring.h>
#include <vorbisfile.h>

#ifdef HAVE_MP4V2
#include "metadata/mp4/mp4file.h"
#include "metadata/mp4/mp4tag.h"
#else
#include "metadata/m4a/mp4file.h"
#include "metadata/m4a/mp4itunestag.h"
#endif

namespace MetaFile
{

//d-pointer implementation

struct MetaData
{
    QString title;
    QString artist;
    QString album;
    QString comment;
    QString composer;
    int discNumber;
    int trackNumber;
    int length;
    int fileSize;
    int sampleRate;
    int bitRate;
    int year;

};

class Track::Private : public QObject
{
public:
    Private( Track *t )
        : QObject()
        , tag( 0 )
        , url()
        , batchUpdate( false )
        , album()
        , artist()
        , track( t )
    {}

public:
    TagLib::FileRef fileRef;
    TagLib::Tag *tag;
    KUrl url;
    bool batchUpdate;
    Meta::AlbumPtr album;
    Meta::ArtistPtr artist;
    Meta::GenrePtr genre;
    Meta::ComposerPtr composer;
    Meta::YearPtr year;

    void readMetaData();
    QVariantMap changes;
    void writeMetaData() { DEBUG_BLOCK Meta::Field::writeFields( fileRef, changes ); changes.clear(); readMetaData(); }
    MetaData m_data;

private:
    Track *track;
};

void Track::Private::readMetaData()
{
    #define strip( x ) TStringToQString( x ).trimmed()
    if( tag )
    {
        TagLib::String metaData = tag->title() + tag->artist() + tag->album() + tag->comment();
        const char* buf = metaData.toCString();
        size_t len = strlen( buf );
        int res = 0;
        chardet_t det = NULL;
        char encoding[CHARDET_MAX_ENCODING_NAME];
        chardet_create( &det );
        res = chardet_handle_data( det, buf, len );
        chardet_data_end( det );
        res = chardet_get_charset( det, encoding, CHARDET_MAX_ENCODING_NAME );
        chardet_destroy( det );

        debug() << "Data:" << buf <<endl;
        debug() << "Charset: " << encoding <<endl;

        m_data.title = strip( tag->title() );
        m_data.artist = strip( tag->artist() );
        m_data.album = strip( tag->album() );
        m_data.comment = strip( tag->comment() );
        m_data.trackNumber = tag->track();
        m_data.year = tag->year();

        //Start to decode non-utf8 tags
        QString track_encoding = encoding;
        if ( res == CHARDET_RESULT_OK )
        {
            //http://doc.trolltech.com/4.4/qtextcodec.html
            //http://www.mozilla.org/projects/intl/chardet.html
            if ( track_encoding == "x-euc-tw" ) track_encoding = ""; //no match
            if ( track_encoding == "HZ-GB2312" ) track_encoding = ""; //no match
            if ( track_encoding == "ISO-2022-CN" ) track_encoding = ""; //no match
            if ( track_encoding == "ISO-2022-KR" ) track_encoding = ""; //no match
            if ( track_encoding == "ISO-2022-JP" ) track_encoding = ""; //no match
            if ( track_encoding == "x-mac-cyrillic" ) track_encoding = ""; //no match
            if ( track_encoding == "IBM855" ) track_encoding =""; //no match
            if ( track_encoding == "IBM866" ) track_encoding = "IBM 866";
            if ( track_encoding == "TIS-620" ) track_encoding = ""; //ISO-8859-11, no match
            if ( track_encoding != "" )
            {
                debug () << "Final Codec Name:" << track_encoding.toUtf8() <<endl;
                QTextCodec *codec = QTextCodec::codecForName( track_encoding.toUtf8() );
                m_data.title = codec->toUnicode( m_data.title.toLatin1() );
                m_data.artist = codec->toUnicode( m_data.artist.toLatin1() );
                m_data.album = codec->toUnicode( m_data.album.toLatin1() );
                m_data.comment = codec->toUnicode( m_data.comment.toLatin1() );
            }
        }
    }
    if( !fileRef.isNull() )
    {
        m_data.bitRate = fileRef.audioProperties()->bitrate();
        m_data.sampleRate = fileRef.audioProperties()->sampleRate();
        m_data.length = fileRef.audioProperties()->length();
    }
    //This is pretty messy...
    QString disc;

    if( TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( fileRef.file() ) )
    {
        if( file->ID3v2Tag() )
        {
            const TagLib::ID3v2::FrameListMap flm = file->ID3v2Tag()->frameListMap();
            if( !flm[ "TPOS" ].isEmpty() )
                disc = strip( flm[ "TPOS" ].front()->toString() );

            if( !flm[ "TCOM" ].isEmpty() )
                m_data.composer = strip( flm[ "TCOM" ].front()->toString() );

            if( !flm[ "TPE2" ].isEmpty() )
                m_data.artist = strip( flm[ "TPE2" ].front()->toString() );

        }
    }

    else if( TagLib::Ogg::Vorbis::File *file = dynamic_cast< TagLib::Ogg::Vorbis::File *>( fileRef.file() ) )
    {
        if( file->tag() )
        {
            const TagLib::Ogg::FieldListMap flm = file->tag()->fieldListMap();
            if( !flm[ "COMPOSER" ].isEmpty() )
                m_data.composer = strip( flm[ "COMPOSER" ].front() );
            if( !flm[ "DISCNUMBER" ].isEmpty() )
                disc = strip( flm[ "DISCNUMBER" ].front() );
        }
    }

    else if( TagLib::FLAC::File *file = dynamic_cast< TagLib::FLAC::File *>( fileRef.file() ) )
    {
        if( file->xiphComment() )
        {
            const TagLib::Ogg::FieldListMap flm = file->xiphComment()->fieldListMap();
            if( !flm[ "COMPOSER "].isEmpty() )
                m_data.composer = strip( flm[ "COMPOSER" ].front() );
            if( !flm[ "DISCNUMBER" ].isEmpty() )
                disc = strip( flm[ "DISCNUMBER" ].front() );
        }
    }

    else if( TagLib::MP4::File *file = dynamic_cast<TagLib::MP4::File *>( fileRef.file() ) )
    {
        TagLib::MP4::Tag *mp4tag = dynamic_cast< TagLib::MP4::Tag *>( file->tag() );
        if( mp4tag )
        {
            m_data.composer = strip( mp4tag->composer() );
            disc = QString::number( mp4tag->disk() );
        }
    }

    if( !disc.isEmpty() )
    {
        int i = disc.indexOf( '/' );
        if( i != -1 )
            m_data.discNumber = disc.left( i ).toInt();
        else
            m_data.discNumber = disc.toInt();
    }
#undef strip
    m_data.fileSize = QFile( url.url() ).size();
}

// internal helper classes

class FileArtist : public Meta::Artist
{
public:
    FileArtist( MetaFile::Track::Private *dptr )
        : Meta::Artist()
        , d( dptr )
    {}

    Meta::TrackList tracks()
    {
        return Meta::TrackList();
    }

    Meta::AlbumList albums()
    {
        return Meta::AlbumList();
    }

    QString name() const
    {
        if( d )
        {
            const QString artist = d->m_data.artist;
            if( !artist.isEmpty() )
                return artist;
            else
                return i18nc( "The value is not known", "Unknown" );
        }
        else
            return i18nc( "The value is not known", "Unknown" );
    }

    QString prettyName() const
    {
        return name();
    }

    QPointer<MetaFile::Track::Private> const d;
};

class FileAlbum : public Meta::Album
{
public:
    FileAlbum( MetaFile::Track::Private *dptr )
        : Meta::Album()
        , d( dptr )
    {}

    bool isCompilation() const
    {
        return false;
    }

    bool hasAlbumArtist() const
    {
        return false;
    }

    Meta::ArtistPtr albumArtist() const
    {
        return Meta::ArtistPtr();
    }

    Meta::TrackList tracks()
    {
        return Meta::TrackList();
    }

    QString name() const
    {
        if( d )
        {
            const QString albumName = d->m_data.album;
            if( !albumName.isEmpty() )
                return albumName;
            else
                return i18nc( "The value is not known", "Unknown" );
        }
        else
            return i18nc( "The value is not known", "Unknown" );
    }

    QString prettyName() const
    {
        return name();
    }

    QPixmap image( int size, bool withShadow )
    {
        return Meta::Album::image( size, withShadow );
    }

    QPointer<MetaFile::Track::Private> const d;
};

class FileGenre : public Meta::Genre
{
public:
    FileGenre( MetaFile::Track::Private *dptr )
        : Meta::Genre()
        , d( dptr )
    {}

    Meta::TrackList tracks()
    {
        return Meta::TrackList();
    }

    QString name() const
    {
        if( d && d->tag )
        {
            const QString genreName = TStringToQString( d->tag->genre() ).trimmed();
            if( !genreName.isEmpty() )
                return genreName;
            else
                return i18nc( "The value is not known", "Unknown" );
        }
        else
            return i18nc( "The value is not known", "Unknown" );
    }

    QString prettyName() const
    {
        return name();
    }

    QPointer<MetaFile::Track::Private> const d;
};

class FileComposer : public Meta::Composer
{
public:
    FileComposer( MetaFile::Track::Private *dptr )
        : Meta::Composer()
        , d( dptr )
    {}

    Meta::TrackList tracks()
    {
        return Meta::TrackList();
    }

    QString name() const
    {
        if( d && d->tag )
        {
            AMAROK_NOTIMPLEMENTED
            const QString composer = d->m_data.composer;
            if( !composer.isEmpty() )
                return composer;
            else
                return i18nc( "The value is not known", "Unknown" );
        }
        else
            return i18nc( "The value is not known", "Unknown" );
    }

    QString prettyName() const
    {
        return name();
    }

    QPointer<MetaFile::Track::Private> const d;
};

class FileYear : public Meta::Year
{
public:
    FileYear( MetaFile::Track::Private *dptr )
        : Meta::Year()
        , d( dptr )
    {}

    Meta::TrackList tracks()
    {
        return Meta::TrackList();
    }

    QString name() const
    {
        if( d )
        {
            const QString year = QString::number( d->m_data.year );
            if( !year.isEmpty()  )
                return year;
            else
                return i18nc( "The value is not known", "Unknown" );
        }
        else
            return i18nc( "The value is not known", "Unknown" );
    }

    QString prettyName() const
    {
        return name();
    }

    QPointer<MetaFile::Track::Private> const d;
};


}

#endif
