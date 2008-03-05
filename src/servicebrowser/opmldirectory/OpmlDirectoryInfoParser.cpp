/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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

#include "OpmlDirectoryInfoParser.h"

#include "ContextStatusBar.h"
#include "debug.h"
#include "OpmlDirectoryMeta.h"

#include <KLocale>

#include <QDomDocument>

using namespace Meta;

OpmlDirectoryInfoParser::OpmlDirectoryInfoParser()
 : InfoParserBase()
{
}


OpmlDirectoryInfoParser::~OpmlDirectoryInfoParser()
{
}

void OpmlDirectoryInfoParser::getInfo(ArtistPtr artist)
{

}

void OpmlDirectoryInfoParser::getInfo(AlbumPtr album)
{

}

void OpmlDirectoryInfoParser::getInfo(TrackPtr track)
{
    DEBUG_BLOCK

    OpmlDirectoryFeed * feed = dynamic_cast<OpmlDirectoryFeed *>( track.data() );

    if ( feed == 0) return;

    debug() << "OpmlDirectoryInfoParser: getInfo about feed: " << feed->url();


    m_rssDownloadJob = KIO::storedGet( feed->url(), KIO::Reload, KIO::HideProgressInfo );
    Amarok::ContextStatusBar::instance()->newProgressOperation( m_rssDownloadJob ).setDescription( i18n( "Fetching Podcast Info" ) );
    connect( m_rssDownloadJob, SIGNAL(result(KJob *)), SLOT( rssDownloadComplete( KJob*) ) );


}

void OpmlDirectoryInfoParser::rssDownloadComplete(KJob * downLoadJob)
{

    if ( !downLoadJob->error() == 0 )
    {
        //TODO: error handling here
        return ;
    }
    
    if ( downLoadJob != m_rssDownloadJob )
        return ; //not the right job, so let's ignore it



    QString rssString = ((KIO::StoredTransferJob* ) downLoadJob)->data();

    debug() << "rss: " << rssString;

    QDomDocument doc( "reply" );
    doc.setContent( rssString );

    //there might be an rss node, there might not...

    QDomElement element = doc.firstChildElement( "rss" );
    if ( !element.isNull() ) {
        element = element.firstChildElement( "channel" );
    } else {
        element = doc.firstChildElement( "channel" );
    }

    QString description = element.firstChildElement( "description" ).text();
    QString title = element.firstChildElement( "title" ).text();
    
    QString imageUrl = QString();
    QDomElement image = element.firstChildElement( "image" );
    
    if ( !image.isNull() ) {

        imageUrl = image.firstChildElement( "url" ).text();
    }


    QString infoHtml = "<HTML><HEAD><META HTTP-EQUIV=\"Content-Type\" "
            "CONTENT=\"text/html; charset=iso-8859-1\"></HEAD><BODY>";

    infoHtml += "<div align=\"center\"><strong>";
    infoHtml += title;
    infoHtml += "</strong><br><br>";

    if ( !imageUrl.isEmpty() ) { 
        infoHtml += "<img src=\"" + imageUrl + "\" align=\"middle\" border=\"1\">";
    }
    
    infoHtml += "<br><p align=\"left\" >" + description;
    infoHtml += "</BODY></HTML>";


    emit ( info( infoHtml ) );

    downLoadJob->deleteLater();
}

#include "OpmlDirectoryInfoParser.moc"
