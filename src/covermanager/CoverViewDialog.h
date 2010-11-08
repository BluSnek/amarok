/****************************************************************************************
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_COVERVIEWDIALOG_H
#define AMAROK_COVERVIEWDIALOG_H

#include "core/meta/Meta.h"
#include "widgets/PixmapViewer.h"

#include <KApplication>
#include <KDialog> //baseclass
#include <KLocale>
#include <KWindowSystem>

#include <QHBoxLayout>
#include <QDesktopWidget>

class AMAROK_EXPORT CoverViewDialog : public QDialog
{
    public:
        CoverViewDialog( Meta::AlbumPtr album, QWidget *parent )
            : QDialog( parent )
        {
            init();
            setWindowTitle( KDialog::makeStandardCaption( i18n("%1 - %2",
                            album->albumArtist()? album->albumArtist()->prettyName() : i18n( "Various Artists" ),
                            album->prettyName() ) ) );
            createViewer( album->image(), parent );
        }

        CoverViewDialog( const QPixmap &pixmap, QWidget *parent )
            : QDialog( parent )
        {
            init();
            setWindowTitle( KDialog::makeStandardCaption( i18n( "Cover View" ) ) );
            createViewer( pixmap, parent );
        }

    private:
        void init()
        {
            setAttribute( Qt::WA_DeleteOnClose );
            kapp->setTopWidget( this );
            #ifdef Q_WS_X11
            KWindowSystem::setType( winId(), NET::Utility );
            #endif
        }

        void createViewer( const QPixmap &pixmap, const QWidget *widget )
        {
            int screenNumber = KApplication::desktop()->screenNumber( widget );
            PixmapViewer *pixmapViewer = new PixmapViewer( this, pixmap, screenNumber );
            QHBoxLayout *layout = new QHBoxLayout( this );
            layout->addWidget( pixmapViewer );
            layout->setSizeConstraint( QLayout::SetFixedSize );
            layout->setContentsMargins( 0, 0, 0, 0 );

            QPoint topLeft = mapFromParent( widget->geometry().center() );
            topLeft -= QPoint( pixmap.width() / 2, pixmap.height() / 2 );
            move( topLeft );
        }
};

#endif
