/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef LASTFMREADLABELCAPABILITY_H
#define LASTFMREADLABELCAPABILITY_H

#include "meta/Capability.h"
#include "meta/capabilities/ReadLabelCapability.h"
class QNetworkReply;

namespace Meta
{
    class Track;

class LastfmReadLabelCapability : public ReadLabelCapability
{
    Q_OBJECT
    public:
        LastfmReadLabelCapability( Meta::Track *track );
        virtual void fetchLabels();
        virtual void fetchGlobalLabels();
        virtual QStringList labels();

    private:
        QStringList m_labels;
        Meta::TrackPtr m_track;
        QNetworkReply *m_job;

    private slots:
        void onTagsFetched();
};
}

#endif // LASTFMREADLABELCAPABILITY_H
