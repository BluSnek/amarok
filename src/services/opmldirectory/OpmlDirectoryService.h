/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef OPMLDIRECTORYSERVICE_H
#define OPMLDIRECTORYSERVICE_H


#include "../ServiceBase.h"
#include "OpmlDirectoryDatabaseHandler.h"
#include "ServiceSqlCollection.h"

#include "core/support/Amarok.h"
#include <kio/job.h>
#include <kio/jobclasses.h>

class OpmlOutline;
class OpmlDirectoryModel;

class OpmlDirectoryServiceFactory: public ServiceFactory
{
    Q_OBJECT

    public:
        OpmlDirectoryServiceFactory( QObject *parent, const QVariantList &args );
        virtual ~OpmlDirectoryServiceFactory() {}

        virtual void init();
        virtual QString name();
        virtual KConfigGroup config();
};

/**
A service for displaying, previewing and downloading music from OpmlDirectory.com

	@author 
*/
class OpmlDirectoryService : public ServiceBase
{
    Q_OBJECT
    public:
        OpmlDirectoryService( OpmlDirectoryServiceFactory* parent, const QString &name,
                              const QString &prettyName );

        ~OpmlDirectoryService();

        void polish();

        virtual Collections::Collection * collection() { return 0; }

    public slots:
        // filter slots
        void slotSetFilterTimeout();
        void slotFilterNow();
        void setFocus();

    private slots:
        void subscribe();
        void slotSelectionChanged( const QItemSelection &, const QItemSelection & );

    private:
        OpmlDirectoryModel *m_opmlModel;
        QSortFilterProxyModel *m_proxyModel;

        QPushButton *m_addOpmlButton;
        QPushButton *m_subscribeButton;

        int m_currentCategoryId;

        int m_numberOfFeeds;
        int m_numberOfCategories;
};

#endif
