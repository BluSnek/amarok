/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef PLAYLISTBROWSERNSPLAYLISTBROWSER_H
#define PLAYLISTBROWSERNSPLAYLISTBROWSER_H

#include <KVBox>

#include <QString>

class QToolBox;
class PodcastCollection;

namespace PlaylistBrowserNS {

class PodcastCategory;

/**
	@author Bart Cerneels
*/
class PlaylistBrowser : public KVBox
{
public:
    PlaylistBrowser( const char *name, QWidget *parent );

    ~PlaylistBrowser();

public slots:
    void addCategory( int category );

private:
    QWidget * loadPodcastCategory();
    QWidget * loadDynamicCategory();

    QToolBox *m_toolBox;
    PodcastCollection *m_localPodcasts;
    PodcastCategory * m_podcastCategory;
};

}

#endif
