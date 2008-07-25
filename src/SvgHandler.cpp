/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *             (c) 2008  Jeff Mitchell <kde-dev@emailgoeshere.com>         *
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
 
#include "SvgHandler.h"

#include "App.h"
#include "Debug.h"
#include "MainWindow.h"
#include "SvgTinter.h"

#include <KStandardDirs>

#include <QHash>
#include <QPainter>
#include <QPixmapCache>
#include <QReadLocker>
#include <QReadWriteLock>
#include <QWriteLocker>

class SvgHandler::Private
{
    public:
        QHash<QString,KSvgRenderer*> renderers;
        QReadWriteLock lock;

        bool loadSvg( const QString& name );
        QString themeFile;
        bool customTheme;
};

class SvgHandlerSingleton
{
    public:
        SvgHandler instance;
};

K_GLOBAL_STATIC( SvgHandlerSingleton, s_privateInstance )

namespace The {
    SvgHandler*
    svgHandler()
    {
        return &s_privateInstance->instance;;
    }
}



SvgHandler::SvgHandler()
    : d( new Private() )
{
    qAddPostRoutine( s_privateInstance.destroy );  // Ensures that the dtor gets called when QApplication destructs

    //use default theme
    d->themeFile = "amarok/images/default-theme.svg";
    d->customTheme = false;
}

SvgHandler::~SvgHandler()
{
    DEBUG_BLOCK

    delete d;
}


bool SvgHandler::Private::loadSvg( const QString& name )
{
    QString svgFilename;
    
    if ( !customTheme )
        svgFilename = KStandardDirs::locate( "data", name );
    else
        svgFilename = name;
    
    KSvgRenderer *renderer = new KSvgRenderer( The::svgTinter()->tint( svgFilename ).toAscii() );

    if ( ! renderer->isValid() )
    {
        debug() << "Bluddy 'ell guvna, aye canna' load ya Ess Vee Gee at " << svgFilename;
        delete renderer;
        return false;
    }
    QWriteLocker writeLocker( &lock );

    if( renderers[name] )
        delete renderers[name];

    renderers[name] = renderer;
    return true;
}

KSvgRenderer* SvgHandler::getRenderer( const QString& name )
{
    QReadLocker readLocker( &d->lock );
    if( ! d->renderers[name] )
    {
        readLocker.unlock();
        if( ! d->loadSvg( name ) )
        {
            QWriteLocker writeLocker( &d->lock );
            d->renderers[name] = new KSvgRenderer();
        }
        readLocker.relock();
    }
    return d->renderers[name];
}

KSvgRenderer * SvgHandler::getRenderer()
{
    return getRenderer( d->themeFile );
}

QPixmap SvgHandler::renderSvg( const QString &name, const QString& keyname, int width, int height, const QString& element )
{
    QPixmap pixmap( width, height );
    pixmap.fill( Qt::transparent );

    QReadLocker readLocker( &d->lock );
    if( ! d->renderers[name] )
    {
        readLocker.unlock();
        if( ! d->loadSvg( name ) )
            return pixmap;
        readLocker.relock();
    }

    const QString key = QString("%1:%2x%3")
        .arg( keyname )
        .arg( width )
        .arg( height );


    if ( !QPixmapCache::find( key, pixmap ) ) {
//         debug() << QString("svg %1 not in cache...").arg( key );

        QPainter pt( &pixmap );
        if ( element.isEmpty() )
            d->renderers[name]->render( &pt, QRectF( 0, 0, width, height ) );
        else
            d->renderers[name]->render( &pt, element, QRectF( 0, 0, width, height ) );
  
        QPixmapCache::insert( key, pixmap );
    }

    return pixmap;
}

QPixmap SvgHandler::renderSvg(const QString & keyname, int width, int height, const QString & element)
{
    return renderSvg( d->themeFile, keyname, width, height, element );
}

QPixmap SvgHandler::renderSvgWithDividers(const QString & keyname, int width, int height, const QString & element)
{

    QString name = d->themeFile;
    
    QPixmap pixmap( width, height );
    pixmap.fill( Qt::transparent );

    QReadLocker readLocker( &d->lock );
    if( ! d->renderers[name] )
    {
        readLocker.unlock();
        if( ! d->loadSvg( name ) )
            return pixmap;
        readLocker.relock();
    }

    const QString key = QString("%1:%2x%3-div")
            .arg( keyname )
            .arg( width )
            .arg( height );


    if ( !QPixmapCache::find( key, pixmap ) ) {
//         debug() << QString("svg %1 not in cache...").arg( key );

        QPainter pt( &pixmap );
        if ( element.isEmpty() )
            d->renderers[name]->render( &pt, QRectF( 0, 0, width, height ) );
        else
            d->renderers[name]->render( &pt, element, QRectF( 0, 0, width, height ) );


        //add dividers. 5% spacing on each side
        int margin = width / 20;
        
        d->renderers[name]->render( &pt, "divider_top", QRectF( margin, 0 , width - 1 * margin, 1 ) );
        d->renderers[name]->render( &pt, "divider_bottom", QRectF( margin, height - 1 , width - 2 * margin, 1 ) );
    
        QPixmapCache::insert( key, pixmap );
    }

    return pixmap;

}


void SvgHandler::reTint()
{
    The::svgTinter()->init();
    d->loadSvg( d->themeFile );
}

QString SvgHandler::themeFile()
{
    return d->themeFile;
}

void SvgHandler::setThemeFile( const QString & themeFile )
{
    DEBUG_BLOCK
    debug() << "got new theme file: " << themeFile;
    d->themeFile = themeFile;
    d->customTheme = true;
    reTint();
    
    //redraw entire app....
    QPixmapCache::clear();
    App::instance()->mainWindow()->update();
}


