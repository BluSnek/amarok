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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.         *
 ***************************************************************************/

#include "Mp3tunesServiceCollection.h"

#include "Mp3tunesServiceQueryMaker.h"

Mp3tunesServiceCollection::Mp3tunesServiceCollection( const QString &sessionId )
 : ServiceCollection()
 , m_sessionId( sessionId )
{
}


Mp3tunesServiceCollection::~Mp3tunesServiceCollection()
{
}

QueryMaker * Mp3tunesServiceCollection::queryMaker()
{
    return new Mp3tunesServiceQueryMaker( this, m_sessionId );
}

QString Mp3tunesServiceCollection::collectionId() const
{
    return "Mp3Tunes collection";
}

QString Mp3tunesServiceCollection::prettyName() const
{
    return collectionId();
}


