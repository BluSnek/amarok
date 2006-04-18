//
// C++ Interface: mediumpluginmanager
//
// Description:
//
//
// Author: Jeff Mitchell <kde-dev@emailgoeshere.com>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef MEDIUMPLUGINMANAGER_H
#define MEDIUMPLUGINMANAGER_H

#include "amarok.h"
#include "hintlineedit.h"
#include "plugin/pluginconfig.h"
#include "pluginmanager.h"

#include <qlayout.h>
#include <qmap.h>

#include <kconfig.h>
#include <kdialogbase.h>
#include <klocale.h>

class QGroupBox;
class QLabel;
class QSignalMapper;
class QVBox;
class KComboBox;
class KLineEdit;
class Medium;
class MediumPluginManager;
class MediumPluginDetailView;

typedef QMap<Medium*, KComboBox*> ComboMap;
typedef QMap<Medium*, QString> OldPluginMap;
typedef QMap<int, Medium*> ButtonMap;
typedef QMap<int, QHBox*> HBoxMap;
typedef QMap<QString, Medium*> DeletedMap;
typedef QMap<QString, Medium*> NewDeviceMap;
typedef QMap<QString, QString> PluginStringMap;

/**
	@author Jeff Mitchell <kde-dev@emailgoeshere.com>
*/
class MediumPluginManager : public KDialogBase
{
    Q_OBJECT

    public:
        MediumPluginManager();
        ~MediumPluginManager();
        QString getPluginName( const QString name ) { return m_dmap[name]; }

    signals:
        void selectedPlugin( const Medium*, const QString );

    private slots:
        void slotOk();
        void infoRequested( int buttonId );
        void configureDevice( int buttonId );
        void deleteDevice( int buttonId );
        void reDetectDevices();
        void newDevice();

    private:

        void detectDevices();

        ComboMap m_cmap;
        OldPluginMap m_omap;
        ButtonMap m_bmap;
        HBoxMap m_hmap;
        PluginStringMap m_dmap;
        DeletedMap m_deletedMap;
        NewDeviceMap m_newDevMap;
        QSignalMapper *m_siginfomap;
        QSignalMapper *m_sigdelmap;
        QSignalMapper *m_sigconfmap;

        QVBox *m_devicesBox;
        QHBox *m_hbox;
        QGroupBox *m_location;
        KPushButton *m_configurebutton;
        KPushButton *m_deletebutton;
        int m_buttonnum;
        bool m_redetect;

        KTrader::OfferList m_offers;
        KTrader::OfferList::ConstIterator m_offersEnd;
        KTrader::OfferList::ConstIterator m_plugit;
};

class MediumPluginDetailView : public KDialogBase
{
    Q_OBJECT

    public:
        MediumPluginDetailView( const Medium* medium );
        ~MediumPluginDetailView();
};

class ManualDeviceAdder : public KDialogBase
{
    Q_OBJECT

    public:
        ManualDeviceAdder( MediumPluginManager* mdm );
        ~ManualDeviceAdder();
        bool successful() const { return m_successful; }
        Medium* getMedium();
        QString getPlugin() const { return m_selectedPlugin; }

    private slots:
        void slotCancel();
        void slotOk();
        void comboChanged( const QString & );

    private:
        MediumPluginManager* m_mpm;
        bool m_successful;
        QString m_comboOldText;
        QString m_selectedPlugin;

        KComboBox* m_mdaCombo;
        HintLineEdit* m_mdaName;
        HintLineEdit* m_mdaMountPoint;

        KTrader::OfferList m_mdaOffers;
        KTrader::OfferList::ConstIterator m_mdaOffersEnd;
        KTrader::OfferList::ConstIterator m_mdaPlugit;
};

#endif

