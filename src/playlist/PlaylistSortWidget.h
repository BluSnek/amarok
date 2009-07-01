/****************************************************************************************
 * Copyright (c) 2009 Teo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#ifndef AMAROK_PLAYLISTSORTWIDGET_H
#define AMAROK_PLAYLISTSORTWIDGET_H

#include "playlist/proxymodels/SortProxy.h"

#include <KComboBox>

#include <QHBoxLayout>

namespace Playlist
{
/**
 * A ribbon interface that allows the user to define multiple sorting levels for the playlist.
 * @author To Mrnjavac <teo.mrnjavac@gmail.com>
 */
class SortWidget : public QWidget
{
    Q_OBJECT
    public:
        SortWidget( QWidget* parent = 0 );
    public slots:
        void pushLevel();
        void popLevel();

    private slots:
        void applySortingScheme();
    private:
        QHBoxLayout *m_comboLayout;
        QList< KComboBox * > m_comboList;
        QStringList m_sortableCategories;
};

}   //namespace Playlist

#endif  //AMAROK_PLAYLISTSORTWIDGET_H
