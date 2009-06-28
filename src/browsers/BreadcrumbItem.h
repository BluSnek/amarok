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
 
#ifndef BREADCRUMBITEM_H
#define BREADCRUMBITEM_H

#include "widgets/ElidingButton.h"

#include <KHBox>

class BrowserCategory;
class BreadcrumbItemButton;
class BreadcrumbItemMenuButton;

/**
 *  A widget representing a single "breadcrumb" item
 *  @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
 */

class BreadcrumbItem : public KHBox
{
    Q_OBJECT
public:
    BreadcrumbItem( BrowserCategory * category );
    ~BreadcrumbItem();

    void setActive( bool active );

    QSizePolicy sizePolicy () const;

protected slots:
    void updateSizePolicy();

private:
    BrowserCategory          *m_category;
    BreadcrumbItemMenuButton *m_menuButton;
    BreadcrumbItemButton     *m_mainButton;
};

#endif
