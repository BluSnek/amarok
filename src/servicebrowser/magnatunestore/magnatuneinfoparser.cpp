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

#include "magnatuneinfoparser.h"

#include "debug.h"
#include "ContextStatusBar.h"

#include <KLocale>

#include <QFile>

using namespace Meta;

void MagnatuneInfoParser::getInfo(ArtistPtr artist)
{

    MagnatuneArtist * magnatuneArtist = dynamic_cast<MagnatuneArtist *>( artist.data() );
    if ( magnatuneArtist == 0) return;

    debug() << "MagnatuneInfoParser: getInfo about artist";

    // first get the entire artist html page
   /* QString tempFile;
    QString orgHtml;*/

    m_infoDownloadJob = KIO::storedGet( magnatuneArtist->magnatuneUrl() );
    Amarok::ContextStatusBar::instance()->newProgressOperation( m_infoDownloadJob ).setDescription( i18n( "Fetching Artist Info" ) );
    connect( m_infoDownloadJob, SIGNAL(result(KJob *)), SLOT( artistInfoDownloadComplete( KJob*) ) );

    Amarok::ContextStatusBar::instance() ->newProgressOperation( m_infoDownloadJob )
    .setDescription( i18n( "Fetching artist info..." ) );
}


void MagnatuneInfoParser::getInfo(AlbumPtr album)
{


    MagnatuneAlbum * magnatuneAlbum = dynamic_cast<MagnatuneAlbum *>( album.data() );

    const QString artistName = album->albumArtist()->name();

    QString infoHtml = "<HTML><HEAD><META HTTP-EQUIV=\"Content-Type\" "
                       "CONTENT=\"text/html; charset=iso-8859-1\"></HEAD><BODY>";

    infoHtml += "<div align=\"center\"><strong>";
    infoHtml += artistName;
    infoHtml += "</strong><br><em>";
    infoHtml += magnatuneAlbum->name();
    infoHtml += "</em><br><br>";
    infoHtml += "<img src=\"" + magnatuneAlbum->coverUrl() +
                "\" align=\"middle\" border=\"1\">";

    infoHtml += "<br><br>Genre: ";// + magnatuneAlbum->
    infoHtml += "<br>Release Year: " + QString::number( magnatuneAlbum->launchYear() );

    if ( !magnatuneAlbum->description().isEmpty() ) {

        //debug() << "MagnatuneInfoParser: Writing description: '" << album->getDescription() << "'";
        infoHtml += "<br><br><b>Description:</b><br><p align=\"left\" >" + magnatuneAlbum->description();

    }

    infoHtml += "</p><br><br>From Magnatune.com</div>";
    infoHtml += "</BODY></HTML>";

    emit ( info( infoHtml ) );
}

void MagnatuneInfoParser::getInfo(TrackPtr track)
{
    Q_UNUSED( track );
    return;
}




void
MagnatuneInfoParser::artistInfoDownloadComplete( KJob *downLoadJob )
{

    if ( !downLoadJob->error() == 0 )
    {
        //TODO: error handling here
        return ;
    }
    if ( downLoadJob != m_infoDownloadJob )
        return ; //not the right job, so let's ignore it



    QString infoString = ((KIO::StoredTransferJob* )downLoadJob)->data();
    //debug() << "MagnatuneInfoParser: Artist info downloaded: " << infoString;
    infoString = extractArtistInfo( infoString );

    //debug() << "html: " << trimmedInfo;

    emit ( info( infoString ) );

}

QString
MagnatuneInfoParser::extractArtistInfo( const QString &artistPage )
{
    QString trimmedHtml;


    int sectionStart = artistPage.indexOf( "<!-- ARTISTBODY -->" );
    int sectionEnd = artistPage.indexOf( "<!-- /ARTISTBODY -->", sectionStart );

    trimmedHtml = artistPage.mid( sectionStart, sectionEnd - sectionStart );

    int buyStartIndex = trimmedHtml.indexOf( "<!-- PURCHASE -->" );
    int buyEndIndex;

    //we are going to integrate the buying of music (I hope) so remove these links

    while ( buyStartIndex != -1 )
    {
        buyEndIndex = trimmedHtml.indexOf( "<!-- /PURCHASE -->", buyStartIndex ) + 18;
        trimmedHtml.remove( buyStartIndex, buyEndIndex - buyStartIndex );
        buyStartIndex = trimmedHtml.indexOf( "<!-- PURCHASE -->", buyStartIndex );
    }


    QString infoHtml = "<HTML><HEAD><META HTTP-EQUIV=\"Content-Type\" "
                       "CONTENT=\"text/html; charset=iso-8859-1\"></HEAD><BODY>";

    infoHtml += trimmedHtml;
    infoHtml += "</BODY></HTML>";


    return infoHtml;
}


#include "magnatuneinfoparser.moc"

