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
 
#include "BreadcrumbItem.h"
#include "BreadcrumbItemButton.h"

#include "BrowserCategoryList.h"
#include "Debug.h"

#include <QMenu>

BreadcrumbItem::BreadcrumbItem( BrowserCategory * category )
    : KHBox( 0 )
    , m_category( category )
    , m_menuButton( 0 )
{

    //figure out if we want to add a menu to this item. A menu allows you to select
    //any of the _sibling_ items. (yes, I know, this is different from how Dolphin
    //does it, but I find the Dolphin way amazingly unintuitive and I always get it
    //wrong when using it...)
    
    BrowserCategoryList * parentList = category->parentList();
    if ( parentList )
    {
        m_menuButton = new BreadcrumbItemMenuButton( this );
        QMenu *menu = new QMenu( this );
        
        QMap<QString,BrowserCategory *> siblingMap =  parentList->categories();

        QStringList siblingNames = siblingMap.keys();

        foreach( QString siblingName, siblingNames )
        {
            //no point in adding ourselves to this menu
            if ( siblingName == m_category->name() )
                continue;
            
            BrowserCategory * siblingCategory = siblingMap.value( siblingName );
            
            QAction * action = menu->addAction( siblingCategory->icon(), siblingCategory->prettyName() );
            connect( action, SIGNAL( triggered() ), siblingMap.value( siblingName ), SLOT( activate() ) );
            
        }

        m_menuButton->setMenu( menu );

        //do a little magic to line up items in the menu with the current item
        int offset = 6;

        menu->setContentsMargins( offset, 1, 1, 2 );
    }

    //m_mainButton = new BreadcrumbItemButton( category->icon(), category->prettyName(), this );
    m_mainButton = new BreadcrumbUrlMenuButton( category->icon(), category->prettyName(), "navigate",  this );
    
    connect( m_mainButton, SIGNAL( sizePolicyChanged() ), this, SLOT( updateSizePolicy() ) );

    //if this is a list, make cliking on this item cause us
    //to navigate to its home.
    BrowserCategoryList *list = dynamic_cast<BrowserCategoryList*>( category );
    if ( list )
    {
        connect( m_mainButton, SIGNAL( clicked( bool ) ), list, SLOT( home() ) );
    }

    hide();

    updateSizePolicy();
}

BreadcrumbItem::~BreadcrumbItem()
{
    DEBUG_BLOCK
}

void
BreadcrumbItem::setActive( bool active )
{
    m_mainButton->setActive( active );
}

QSizePolicy BreadcrumbItem::sizePolicy() const
{
    return m_mainButton->sizePolicy();
}

void BreadcrumbItem::updateSizePolicy()
{
    setSizePolicy( m_mainButton->sizePolicy() );
}



