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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#include "PopupDropperFactory.h"

#include "Debug.h"
#include "ContextView.h"
#include "PaletteHandler.h"
#include "context/popupdropper/PopupDropperAction.h"
#include "context/popupdropper/PopupDropperItem.h"
#include "SvgHandler.h"


#include <kglobal.h>

class PopupDropperFactorySingleton
{
    public:
        PopupDropperFactory instance;
};

K_GLOBAL_STATIC( PopupDropperFactorySingleton, privateInstance )

namespace The {

    PopupDropperFactory*
            popupDropperFactory()
    {
        return &privateInstance->instance;
    }
}


PopupDropperFactory::PopupDropperFactory()
{
}


PopupDropperFactory::~PopupDropperFactory()
{
}


PopupDropper * PopupDropperFactory::createPopupDropper( QWidget * parent )
{
    DEBUG_BLOCK
    PopupDropper* pd = new PopupDropper( parent );
    if( !pd )
        return 0;
    pd->setSvgRenderer( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ) );
    pd->setQuitOnDragLeave( false );
    pd->setFadeInTime( 500 );
    pd->setFadeOutTime( 300 );
    //QColor origWindowColor( The::paletteHandler()->palette().color( QPalette::Window ) );
    //QColor windowColor;
    //windowColor.setRed( 255 - origWindowColor.red() );
    //windowColor.setBlue( 255 - origWindowColor.blue() );
    //windowColor.setGreen( 255 - origWindowColor.green() );
    QColor windowColor( The::paletteHandler()->palette().color( QPalette::Base ) );
    windowColor.setAlpha( 128 );
    QColor textColor( The::paletteHandler()->palette().color( QPalette::Link ) );
    QColor highlightedTextColor( The::paletteHandler()->palette().color( QPalette::LinkVisited ) );
    QColor borderColor( The::paletteHandler()->palette().color( QPalette::Highlight ) );
    QColor fillColor( borderColor );
    fillColor.setAlpha( 48 );
    pd->setColors( windowColor, textColor, highlightedTextColor, borderColor, fillColor );

    return pd;
}

PopupDropper * PopupDropperFactory::createPopupDropper()
{
    return createPopupDropper( Context::ContextView::self() );
}

PopupDropperItem * PopupDropperFactory::createItem( PopupDropperAction * action )
{
    QFont font;
    font.setPointSize( 16 );
    font.setBold( true );

    PopupDropperItem* pdi = new PopupDropperItem();
    pdi->setAction( action );
    pdi->setFont( font );
    pdi->setHoverMsecs( 800 );

    return pdi;
}


