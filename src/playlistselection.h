/***************************************************************************
 *   Copyright (C) 2005 Ian Monroe <ian@monroe.nu>                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef PLAYLISTSELECTION_H
#define PLAYLISTSELECTION_H

#include <klistview.h>
#include <qlistview.h>

class NewDynamic;
class KDialogBase;
class PartyEntry;

//this is a widget used in newdynamic.ui
class PlaylistSelection : public KListView
{
    Q_OBJECT

    public:
        PlaylistSelection( QWidget* parent, char* name );

    private:
        void loadChildren( QListViewItem* browserParent, QListViewItem* selectionParent );
};

namespace ConfigDynamic
{
    void addDynamic( NewDynamic* dialog );
    void dynamicDialog( QWidget* parent );
    void editDynamicPlaylist( QWidget* parent, PartyEntry* entry );

    KDialogBase* basicDialog( QWidget* parent );
    PartyEntry*  loadPartyEntry( NewDynamic* dialog );

};

class SelectionListItem : public QCheckListItem
{
    public:
        SelectionListItem( QListViewItem  * parent, const QString& text, QListViewItem* browserEquivalent );
        SelectionListItem( QCheckListItem * parent, const QString& text, QListViewItem* browserEquivalent );

    protected:
        virtual void stateChange(bool);

    private:
        QListViewItem* m_browserEquivalent;
};
#endif
