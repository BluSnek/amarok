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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/



#include "OpmlDirectoryMeta.h"

#include "OpmlDirectoryService.h"

#include "debug.h"

using namespace Meta;

OpmlDirectoryMetaFactory::OpmlDirectoryMetaFactory( const QString & dbPrefix, OpmlDirectoryService * service )
    : ServiceMetaFactory( dbPrefix )
    , m_service( service )
{
}

TrackPtr OpmlDirectoryMetaFactory::createTrack( const QStringList & rows )
{
    return TrackPtr( new OpmlDirectoryFeed( rows ) );
}


AlbumPtr OpmlDirectoryMetaFactory::createAlbum( const QStringList & rows )
{
    return AlbumPtr( new OpmlDirectoryCategory( rows ) );
}


//// OpmlDirectoryFeed ////

OpmlDirectoryFeed::OpmlDirectoryFeed( const QString &name )
    : ServiceTrack( name )
{
}

OpmlDirectoryFeed::OpmlDirectoryFeed( const QStringList & resultRow )
    : ServiceTrack( resultRow )
{
}

QList< QAction * > Meta::OpmlDirectoryFeed::customActions()
{
    DEBUG_BLOCK
    QList< QAction * > actions;
   /* QAction * action = new QAction( KIcon("get-hot-new-stuff-amarok" ), i18n( "&Download" ), 0 );

    JamendoAlbum * jAlbum = static_cast<JamendoAlbum *> ( album().data() );

    QObject::connect( action, SIGNAL( activated() ), jAlbum->service(), SLOT( download() ) );

    actions.append( action );*/
    return actions;
}



OpmlDirectoryCategory::OpmlDirectoryCategory( const QString & name )
    : ServiceAlbum( name )
{
}

OpmlDirectoryCategory::OpmlDirectoryCategory( const QStringList & resultRow )
    : ServiceAlbum( resultRow )
{
}





