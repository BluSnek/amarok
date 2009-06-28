/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#include "MagnatuneUrlRunner.h"

MagnatuneUrlRunner::MagnatuneUrlRunner()
 : QObject()
 , AmarokUrlRunnerBase()
{
}

MagnatuneUrlRunner::~MagnatuneUrlRunner()
{
}

QString MagnatuneUrlRunner::command() const
{
    return "service_magnatune";
}

bool MagnatuneUrlRunner::run( AmarokUrl url )
{
    DEBUG_BLOCK
    if ( url.numberOfArgs() > 0 )
    {
        QString command = url.arg( 0 );

        if( command == "show_favorites" )
        {
            emit( showFavorites() );
        }
        else if ( command == "show_home" )
        {
            emit( showHome() );
        }
        else if ( command == "download" || command == "purchase" || command == "buy" )
        {
            //allow some aliases for this command as the context might make one of
            //them more appropriate. In any case, non and stream  members will be given the
            //purchase dialog and will have to pay, download members will get the
            //free download

            if ( url.numberOfArgs() == 2 )
            {
                QString sku = url.arg( 1 );
                emit( buyOrDownload( sku ) );
            }
        }

    }
    return true;
}


#include "MagnatuneUrlRunner.moc"
