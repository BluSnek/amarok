/*
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

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

#include "File.h"
#include "File_p.h"

#include "Meta.h"
#include "meta/EditCapability.h"
#include "MetaUtility.h"

#include <QPointer>
#include <QString>

using namespace MetaFile;

class EditCapabilityImpl : public Meta::EditCapability
{
    Q_OBJECT
    public:
        EditCapabilityImpl( MetaFile::Track *track )
            : Meta::EditCapability()
            , m_track( track )
        {}

        virtual bool isEditable() const { return m_track->isEditable(); }
        virtual void setAlbum( const QString &newAlbum ) { m_track->setAlbum( newAlbum ); }
        virtual void setArtist( const QString &newArtist ) { m_track->setArtist( newArtist ); }
        virtual void setComposer( const QString &newComposer ) { m_track->setComposer( newComposer ); }
        virtual void setGenre( const QString &newGenre ) { m_track->setGenre( newGenre ); }
        virtual void setYear( const QString &newYear ) { m_track->setYear( newYear ); }
        virtual void setTitle( const QString &newTitle ) { m_track->setTitle( newTitle ); }
        virtual void setComment( const QString &newComment ) { m_track->setComment( newComment ); }
        virtual void setTrackNumber( int newTrackNumber ) { m_track->setTrackNumber( newTrackNumber ); }
        virtual void setDiscNumber( int newDiscNumber ) { m_track->setDiscNumber( newDiscNumber ); }
        virtual void beginMetaDataUpdate() { m_track->beginMetaDataUpdate(); }
        virtual void endMetaDataUpdate() { m_track->endMetaDataUpdate(); }
        virtual void abortMetaDataUpdate() { m_track->abortMetaDataUpdate(); }

    private:
        KSharedPtr<MetaFile::Track> m_track;
};

Track::Track( const KUrl &url )
    : Meta::Track()
    , d( new Track::Private( this ) )
{
#ifdef COMPLEX_TAGLIB_FILENAME
    const wchar_t * encodedName = reinterpret_cast<const wchar_t *>(url.path().utf16());
#else
    QByteArray fileName = QFile::encodeName( url.path() );
    const char * encodedName = fileName.constData(); // valid as long as fileName exists
#endif
    d->url = url;
    d->fileRef = TagLib::FileRef( encodedName, true, TagLib::AudioProperties::Fast );
    if( !d->fileRef.isNull() )
    {
        d->tag = d->fileRef.tag();
    }
    d->readMetaData();
    d->album = Meta::AlbumPtr( new MetaFile::FileAlbum( QPointer<MetaFile::Track::Private>( d ) ) );
    d->artist = Meta::ArtistPtr( new MetaFile::FileArtist( QPointer<MetaFile::Track::Private>( d ) ) );
    d->genre = Meta::GenrePtr( new MetaFile::FileGenre( QPointer<MetaFile::Track::Private>( d ) ) );
    d->composer = Meta::ComposerPtr( new MetaFile::FileComposer( QPointer<MetaFile::Track::Private>( d ) ) );
    d->year = Meta::YearPtr( new MetaFile::FileYear( QPointer<MetaFile::Track::Private>( d ) ) );
}

Track::~Track()
{
    delete d;
}

QString
Track::name() const
{
    if( d )
    {
        const QString trackName = d->m_data.title;
        if( !trackName.isEmpty() )
        {
            return trackName;
        }
        else
        {
            //lets use the filename, or it will look really dull in the playlist
            return d->url.fileName();
        }
    }
    else
        return "This is a bug!";
}

QString
Track::prettyName() const
{
    return name();
}

QString
Track::fullPrettyName() const
{
    return name();
}

QString
Track::sortableName() const
{
    return name();
}


KUrl
Track::playableUrl() const
{
    return d->url;
}

QString
Track::prettyUrl() const
{
    return d->url.path();
}

QString
Track::url() const
{
    return d->url.url();
}

bool
Track::isPlayable() const
{
    //simple implementation, check internet connectivity or ping server?
    return true;
}

bool
Track::isEditable() const
{
    //not this probably needs more work on *nix
    return QFile::permissions( d->url.path() ) & QFile::WriteUser;
}

Meta::AlbumPtr
Track::album() const
{
    return d->album;
}

Meta::ArtistPtr
Track::artist() const
{
    return d->artist;
}

Meta::GenrePtr
Track::genre() const
{
    return d->genre;
}

Meta::ComposerPtr
Track::composer() const
{
    return d->composer;
}

Meta::YearPtr
Track::year() const
{
    return d->year;
}

void
Track::setAlbum( const QString &newAlbum )
{
    DEBUG_BLOCK
    d->changes.insert( Meta::Field::ALBUM, QVariant( newAlbum ) );
    debug() << "CHANGES HERE: " << d->changes;
    if( !d->batchUpdate )
    {
        d->m_data.album = newAlbum;
        d->writeMetaData();
        notifyObservers();
    }
}

void
Track::setArtist( const QString& newArtist )
{
    d->changes.insert( Meta::Field::ARTIST, QVariant( newArtist ) );
    if( !d->batchUpdate )
    {
        d->m_data.artist = newArtist;
        d->writeMetaData();
        notifyObservers();
    }
}

void
Track::setGenre( const QString& newGenre )
{
    d->changes.insert( Meta::Field::GENRE, QVariant( newGenre ) );
    if( !d->batchUpdate )
    {
        d->writeMetaData();
        notifyObservers();
    }
}

void
Track::setComposer( const QString& newComposer )
{
    AMAROK_NOTIMPLEMENTED
    Q_UNUSED( newComposer );
    #if 0
    d->metaInfo.item( Meta::Field::xesamPrettyToFullFieldName( Meta::Field::COMPOSER ) ).setValue( newComposer );
    if( !d->batchUpdate )
    {
        d->metaInfo.applyChanges();
        notifyObservers();
    }
    #endif
}

void
Track::setYear( const QString& newYear )
{
    d->changes.insert( Meta::Field::YEAR, QVariant( newYear ) );
    if( !d->batchUpdate )
    {
        d->m_data.year = newYear.toInt();
        d->writeMetaData();
        notifyObservers();
    }
}

void
Track::setTitle( const QString &newTitle )
{
    d->changes.insert( Meta::Field::TITLE, QVariant( newTitle ) );
    if( !d->batchUpdate )
    {
        d->m_data.title = newTitle;
        d->writeMetaData();
        notifyObservers();
    }
}

QString
Track::comment() const
{
    if( d && d->tag )
    {
        const QString commentName = d->m_data.comment;
        if( !commentName.isEmpty() )
            return commentName;
        else
            return QString();
    }
    return QString();
}

void
Track::setComment( const QString& newComment )
{
    d->changes.insert( Meta::Field::COMMENT, QVariant( newComment ) );
    if( !d->batchUpdate )
    {
        d->m_data.comment = newComment;
        d->writeMetaData();
        notifyObservers();
    }
}

double
Track::score() const
{
    return 0.0;
}

void
Track::setScore( double newScore )
{
    Q_UNUSED( newScore )
}

int
Track::rating() const
{
    return 0;
}

void
Track::setRating( int newRating )
{
    Q_UNUSED( newRating )
}

int
Track::trackNumber() const
{
    if( d && d->tag )
    {
        return d->m_data.trackNumber;
    }
    return 0;
}

void
Track::setTrackNumber( int newTrackNumber )
{
    d->changes.insert( Meta::Field::TRACKNUMBER, QVariant( newTrackNumber ) );
    if( !d->batchUpdate )
    {
        d->m_data.trackNumber = newTrackNumber;
        d->writeMetaData();
        notifyObservers();
    }
}

int
Track::discNumber() const
{
    if( d && d->tag )
    {
        return d->m_data.discNumber;
    }
    return 0;
}

void
Track::setDiscNumber( int newDiscNumber )
{
    AMAROK_NOTIMPLEMENTED
    Q_UNUSED( newDiscNumber );
    #if 0
    d->metaInfo.item( Meta::Field::xesamPrettyToFullFieldName( Meta::Field::DISCNUMBER ) ).setValue( newDiscNumber );
    if( !d->batchUpdate )
    {
        d->metaInfo.applyChanges();
        notifyObservers();
    }
    #endif
}

int
Track::length() const
{
    if( d && !d->fileRef.isNull() )
    {
        int length = d->m_data.length;
        if( length == -2 /*Undetermined*/ )
            length = 0;
        return length;
    }
    return 0;
}

int
Track::filesize() const
{
    return d->m_data.fileSize;
}

int
Track::sampleRate() const
{
    if( d && !d->fileRef.isNull() )
    {
        int sampleRate = d->m_data.sampleRate;
        if( sampleRate == -2 /*Undetermined*/ )
            sampleRate = 0;
        return sampleRate;
    }
    return 0;
}

int
Track::bitrate() const
{
    if( d && !d->fileRef.isNull() )
    {
        int bitrate = d->m_data.bitRate;
        if( bitrate == -2 /*Undetermined*/ )
            bitrate = 0;
        return bitrate;
    }
    return 0;
}

uint
Track::lastPlayed() const
{
    return 0;
}

int
Track::playCount() const
{
    return 0;
}

QString
Track::type() const
{
    return "";
}

void
Track::beginMetaDataUpdate()
{
    d->batchUpdate = true;
}

void
Track::endMetaDataUpdate()
{
 DEBUG_LINE_INFO
    debug() << "CHANGES HERE: " << d->changes;
    d->writeMetaData();
    d->batchUpdate = false;
    notifyObservers();
}

void
Track::abortMetaDataUpdate()
{
    d->changes.clear();
    d->batchUpdate = false;
}

void
Track::finishedPlaying( double playedFraction )
{
    Q_UNUSED( playedFraction );
    //TODO
}

bool
Track::inCollection() const
{
    return false;
}

Collection*
Track::collection() const
{
    return 0;
}

bool
Track::hasCapabilityInterface( Meta::Capability::Type type ) const
{
    return type == Meta::Capability::Editable;
}

Meta::Capability*
Track::asCapabilityInterface( Meta::Capability::Type type )
{
    switch( type )
    {
        case Meta::Capability::Editable:
            return new EditCapabilityImpl( this );
            break;
        default:
            return 0;
    }
}

#include "File.moc"
