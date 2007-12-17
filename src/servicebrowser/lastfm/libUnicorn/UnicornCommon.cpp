/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Last.fm Ltd <client@last.fm>                                       *
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

#include "UnicornCommon.h"

#include "logger.h"
#include "md5/md5.h"

#include <QMap>
#include <QUrl>
#include <QHttp>
#include <QCoreApplication>
#include <QDir>

#ifdef WIN32
    #include "windows.h"
//    #include "shfolder.h"
#endif

#include "amarok.h"

using namespace std;

namespace UnicornUtils
{


QString
md5Digest( const char *token )
{
    md5_state_t md5state;
    unsigned char md5pword[16];

    md5_init( &md5state );
    md5_append( &md5state, (unsigned const char *)token, (int)strlen( token ) );
    md5_finish( &md5state, md5pword );

    char tmp[33];
    strncpy( tmp, "\0", sizeof(tmp) );
    for ( int j = 0; j < 16; j++ )
    {
        char a[3];
        sprintf( a, "%02x", md5pword[j] );
        tmp[2*j] = a[0];
        tmp[2*j+1] = a[1];
    }

    return QString::fromAscii( tmp );
}


QString
QHttpStateToString( int state )
{
    switch ( state )
    {
        case QHttp::Unconnected: return QCoreApplication::translate( "WebService", "No connection." );
        case QHttp::HostLookup: return QCoreApplication::translate( "WebService", "Looking up host..." );
        case QHttp::Connecting: return QCoreApplication::translate( "WebService", "Connecting..." );
        case QHttp::Sending: return QCoreApplication::translate( "WebService", "Sending request..." );
        case QHttp::Reading: return QCoreApplication::translate( "WebService", "Downloading." );
        case QHttp::Connected: return QCoreApplication::translate( "WebService", "Connected." );
        case QHttp::Closing: return QCoreApplication::translate( "WebService", "Closing connection..." );
        default: return QString();
    }
}


QString
qtLanguageToLfmLangCode( QLocale::Language qtLang )
{
    switch ( qtLang )
    {
        case QLocale::English:    return "en";
        case QLocale::French:     return "fr";
        case QLocale::Italian:    return "it";
        case QLocale::German:     return "de";
        case QLocale::Spanish:    return "es";
        case QLocale::Portuguese: return "pt";
        case QLocale::Polish:     return "pl";
        case QLocale::Russian:    return "ru";
        case QLocale::Japanese:   return "jp";
        case QLocale::Chinese:    return "cn";
        case QLocale::Swedish:    return "sv";
        case QLocale::Turkish:    return "tr";
        default:                  return "en";
    }
}


QString
lfmLangCodeToIso639( const QString& code )
{
    if ( code == "jp" ) return "ja";
    if ( code == "cn" ) return "zh";

    return code;
}


QString
localizedHostName( const QString& code )
{
    if ( code == "en" ) return "www.last.fm"; //first as optimisation
    if ( code == "pt" ) return "www.lastfm.com.br";
    if ( code == "tr" ) return "www.lastfm.com.tr";
    if ( code == "cn" ) return "cn.last.fm";
    if ( code == "sv" ) return "www.lastfm.se";

    QStringList simple_hosts = QStringList()
            << "fr" << "it" << "de" << "es" << "pl"
            << "ru" << "jp" << "se";

    if ( simple_hosts.contains( code ) )
        return "www.lastfm." + code;

    // else default to english site
    return "www.last.fm";
}


void
parseQuotedStrings( const string& sCompound, vector<string>& separated )
{
    string sCopy(sCompound);

    string::size_type nIdxNext = 0;

    while (nIdxNext < sCopy.size())
    {
        string::size_type nIdxStart = sCopy.find_first_of('\"', nIdxNext);

        if (nIdxStart == string::npos)
        {
            // Not found
            return;
        }

        nIdxStart++;
        if (nIdxStart >= sCopy.size())
        {
            // Buggered string
            return;
        }

        string::size_type nIdxStop = nIdxStart;
        bool bStopFound = false;
        do
        {
            nIdxStop = sCopy.find_first_of('\"', nIdxStop);
            if (nIdxStop == string::npos)
            {
                // Buggered string
                return;
            }
            nIdxStop++;
            if (nIdxStop >= sCopy.size())
            {
                // True stop at end of string
                bStopFound = true;
                break;
            }

            // Check if escaped
            if (sCopy[nIdxStop] == '\"')
            {
                // Remove dupe
                sCopy.erase(nIdxStop, 1);
            }
            else
            {
                // Not escape, true stop
                bStopFound = true;
            }

        } while (!bStopFound);

        string sQuoted = sCopy.substr(nIdxStart, (nIdxStop - 1) - nIdxStart);
        separated.push_back(sQuoted);

        // Position next just after the closing "
        nIdxNext = nIdxStop;

    } // end while

}


void
trim( string& str )
{
    string::size_type pos1 = str.find_first_not_of(" \t\n\f\r");

    if (pos1 == string::npos)
    {
        return;
    }

    string::size_type pos2 = str.find_last_not_of(" \t");

    str = str.substr(pos1, pos2 - pos1 + 1);
}


void
stripBBCode( std::string& str )
{
    string::size_type nIdxNext = 0;

    while (nIdxNext < str.size())
    {
        string::size_type nIdxStart = str.find_first_of('[', nIdxNext);

        if (nIdxStart == string::npos)
        {
            // Not found
            return;
        }

        nIdxStart++;
        if (nIdxStart >= str.size())
        {
            // Buggered string
            return;
        }

        string::size_type nIdxStop = str.find_first_of(']', nIdxStart);
        if (nIdxStop == string::npos)
        {
            // Buggered string
            return;
        }

        // Remove BBCode section
        size_t numRemove = nIdxStop - nIdxStart + 2;
        str.erase(nIdxStart - 1, numRemove);

        nIdxNext = nIdxStop + 1 - numRemove;
    }

}


void
stripBBCode( QString& str )
{
    int nIdxNext = 0;

    while (nIdxNext < str.size())
    {
        int nIdxStart = str.indexOf('[', nIdxNext);

        if (nIdxStart == -1)
        {
            // Not found
            return;
        }

        nIdxStart++;
        if (nIdxStart >= str.size())
        {
            // Buggered string
            return;
        }

        int nIdxStop = str.indexOf(']', nIdxStart);
        if (nIdxStop == -1)
        {
            // Buggered string
            return;
        }

        // Remove BBCode section
        int numRemove = nIdxStop - nIdxStart + 2;
        str.remove(nIdxStart - 1, numRemove);

        nIdxNext = nIdxStop + 1 - numRemove;
    }
}


QString
urlEncodeItem( QString item )
{
    urlEncodeSpecialChars( item );
    item = QUrl::toPercentEncoding( item );

    return item;
}


QString&
urlEncodeSpecialChars(
    QString& str)
{
    str.replace( "&", "%26" );
    str.replace( "/", "%2F" );
    str.replace( ";", "%3B" );
    str.replace( "+", "%2B" );
    str.replace( "#", "%23" );

    return str;
}


QStringList
sortCaseInsensitively( QStringList input )
{
    // This cumbersome bit of code here is how the Qt docs suggests you sort
    // a string list case-insensitively
    QMap<QString, QString> map;
    foreach (QString s, input)
        map.insert( s.toLower(), s );

    QStringList output;
    QMapIterator<QString, QString> i( map );
    while (i.hasNext())
        output += i.next().value();

    return output;
}


QString
appDataPath()
{
    return Amarok::saveLocation();
}


} // namespace UnicornUtils
