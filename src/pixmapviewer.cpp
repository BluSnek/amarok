/***************************************************************************
 *   Copyright (C) 2005 Eyal Lotem <eyal.lotem@gmail.com>                  *
 *   Copyright (C) 2005 Alexandre Oliveira <aleprj@gmail.com>              *
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
#include "pixmapviewer.h"

#include <KApplication>

#include <QDesktopWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>


PixmapViewer::PixmapViewer(QWidget *widget, const QPixmap &pixmap)
    : Q3ScrollView(widget, 0, Qt::WNoAutoErase)
    , m_isDragging(false)
    , m_pixmap(pixmap)
{
    resizeContents( m_pixmap.width(), m_pixmap.height() );
}

void PixmapViewer::drawContents( QPainter * p, int clipx, int clipy, int clipw, int cliph ) {
    p->drawPixmap(QPoint(clipx, clipy),
                  m_pixmap,
                  QRect(clipx, clipy, clipw, cliph));
}

void PixmapViewer::contentsMousePressEvent(QMouseEvent *event) {
    if(Qt::LeftButton == event->button()) {
        m_currentPos = event->globalPos();
        m_isDragging = true;
    }
}

void PixmapViewer::contentsMouseReleaseEvent(QMouseEvent *event) {
    if(Qt::LeftButton == event->button()) {
        m_currentPos = event->globalPos();
        m_isDragging = false;
    }
}

void PixmapViewer::contentsMouseMoveEvent(QMouseEvent *event) {
    if(m_isDragging) {
        QPoint delta = m_currentPos - event->globalPos();
        scrollBy(delta.x(), delta.y());
        m_currentPos = event->globalPos();
    }
}

QSize PixmapViewer::maximalSize() {
    return m_pixmap.size().boundedTo( KApplication::desktop()->size() ) + size() - viewport()->size();
}

#include "pixmapviewer.moc"
