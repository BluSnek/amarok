/***************************************************************************
 *   Copyright (c) 2009  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 
#ifndef EXPANDINGCONTROLSWIDGET_H
#define EXPANDINGCONTROLSWIDGET_H

#include "widgets/HorizontalDivider.h"

#include <KVBox>

#include <QToolButton>


/**
A widget for displaying controls that should not always be visible. Displays a thin "expand" button by default and expands to show the controls and a similar "collapse" when this is pressed.

	@author 
*/
class ExpandingControlsWidget : public KVBox
{
    Q_OBJECT
public:
    ExpandingControlsWidget( QWidget * parent );
    ~ExpandingControlsWidget();
    void setMainWidget( QWidget * mainWidget );

protected slots:
    void toggleExpanded();
    
private:

    HorizontalDivider * m_divider;
    QWidget * m_mainWidget;
    bool m_expanded;
    QToolButton * m_expandButton;

};

#endif
