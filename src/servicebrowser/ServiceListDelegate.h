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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef SERVICELISTDELEGATE_H
#define SERVICELISTDELEGATE_H

#include "../widgets/SvgHandler.h"

#include <QItemDelegate>
#include <QListView>
#include <QSvgRenderer>



/**
A delegate for displaying a nice overview of a service

	@author
*/
class ServiceListDelegate : public QItemDelegate, public SvgHandler
{
public:
    ServiceListDelegate( QListView *view );
    ~ServiceListDelegate();


    void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
    QSize sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const;

    void paletteChange();

private:

    QSvgRenderer *m_svgRendererActive;
    QSvgRenderer *m_svgRendererInactive;

    QListView *m_view;

};

#endif
