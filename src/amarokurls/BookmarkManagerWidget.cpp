/****************************************************************************************
 * Copyright (c) 2008, 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>              *
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

#include "BookmarkManagerWidget.h"

#include "AmarokUrl.h"
#include "BookmarkModel.h"
#include "NavigationUrlGenerator.h"
#include "PlayUrlGenerator.h"
#include "ProgressWidget.h"

#include <KAction>
#include <KIcon>
#include <KLocale>
#include <KVBox>

#include <QLabel>

BookmarkManagerWidget::BookmarkManagerWidget( QWidget * parent )
 : KVBox( parent )
{

    setContentsMargins( 0,0,0,0 );

    m_toolBar = new QToolBar( this );
    m_toolBar->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );

    KAction * addGroupAction = new KAction( KIcon("media-track-add-amarok" ), i18n( "Add Folder" ), this  );
    m_toolBar->addAction( addGroupAction );
    connect( addGroupAction, SIGNAL( triggered( bool ) ), BookmarkModel::instance(), SLOT( createNewGroup() ) );

    m_bookmarkView = new BookmarkTreeView( this );
    m_bookmarkView->setModel( BookmarkModel::instance() );
    connect( m_bookmarkView, SIGNAL( bookmarkSelected( AmarokUrl ) ), this, SLOT( slotBookmarkSelected( AmarokUrl ) ) );
    connect( m_bookmarkView, SIGNAL( showMenu( KMenu*, const QPointF& ) ), this, SIGNAL( showMenu( KMenu*, const QPointF& ) ) );

    m_currentBookmarkId = -1;

}

BookmarkManagerWidget::~BookmarkManagerWidget()
{
}


BookmarkTreeView * BookmarkManagerWidget::treeView()
{
    return m_bookmarkView;
}


#include "BookmarkManagerWidget.moc"



