/***************************************************************************
 *   Copyright (C) 2005 by Max Howell <max.howell@methylblue.com>          *
 *             (C) 2004 Frederik Holljen <fh@ez.no>                        *
 *             (C) 2005 Gábor Lehel <illissius@gmail.com>                  *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/

/** WARNING this is not meant for use outside this unit! */

#ifndef AMAROK_SELECTLABEL_H
#define AMAROK_SELECTLABEL_H

#include "ActionClasses.h"
#include "amarok.h"
#include "amarokconfig.h"
#include "overlayWidget.h"
#include "popupMessage.h"

#include <QIcon>
#include <QLabel>
#include <QTimer>
#include <QToolTip>
//Added by qt3to4:
#include <QPixmap>
#include <QEvent>
#include <QMouseEvent>
#include <QStyle>

#include <kactioncollection.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kpassivepopup.h>



class SelectLabel : public QLabel
{
    Q_OBJECT

    Amarok::SelectAction const*const m_action;

    signals:
        void activated( int );

    public:
        SelectLabel( Amarok::SelectAction const*const action, QWidget *parent )
                : QLabel( parent )
                , m_action( action )
                , m_tooltip( 0 )
                , m_tooltipShowing( false )
                , m_tooltipHidden( false )
        {
            connect( this,   SIGNAL( activated( int ) ), action, SLOT( setCurrentItem( int ) ) );
            connect( action, SIGNAL( activated( int ) ), this,   SLOT( setCurrentItem( int ) ) );
            connect( action, SIGNAL( enabled( bool )  ), this,   SLOT( setEnabled( bool )    ) );

            setCurrentItem( currentItem() );
        }

        inline int currentItem() const { return m_action->currentItem(); }
        inline bool isEnabled()  const { return m_action->isEnabled();   }

    protected:
        void mousePressEvent( QMouseEvent* )
        {
            bool shown = m_tooltipShowing;
            hideToolTip();
            int n = currentItem();
            do //TODO doesn't handle all of them being disabled, but we don't do that anyways.
            {
                n = ( int( n ) == m_action->items().count() - 1 ) ? 0 : n + 1;
            } while ( !m_action->menu()->isItemEnabled( n ) );
            if( isEnabled() )
            {
                setCurrentItem( n );
                emit activated( n );
                if( shown )
                {
                    m_tooltipHidden = false;
                    showToolTip();
                }
            }
        }

        void enterEvent( QEvent* )
        {
            //Show the tooltip after 1/2 second
            m_tooltipHidden = false;
            QTimer::singleShot( 500, this, SLOT(aboutToShow()) );
        }

        void leaveEvent( QEvent* )
        {
            hideToolTip();
        }

    public slots:
        void setCurrentItem( int )
        {
            if( isEnabled() && !m_action->currentIcon().isNull() )
                setPixmap( SmallIcon( m_action->currentIcon() ) );
        }

        void setEnabled( bool /*on*/ )
        {
            if( !m_action->currentIcon().isNull() )
                setPixmap( KIcon( m_action->currentIcon() ).pixmap( style()->pixelMetric(QStyle::PM_SmallIconSize), QIcon::Disabled ) );
        }

    private slots:
        void aboutToShow()
        {
            if( testAttribute(Qt::WA_UnderMouse) && !m_tooltipHidden )
                showToolTip();
        }

    private:
        void showToolTip()
        {
            if( m_tooltipShowing )
                return;

            m_tooltipShowing = true;

            QString tip = i18n("%1: %2", m_action->text().remove( '&' ),  m_action->currentText().remove( '&' ) );

            if( !isEnabled() )
                tip += i18n("&nbsp;<br/>&nbsp;<i>Disabled</i>");
            else if( AmarokConfig::favorTracks() &&
                     m_action == Amarok::actionCollection()->action( "random_mode" ) ) //hack?
            {
                KSelectAction *a = static_cast<KSelectAction*>( Amarok::actionCollection()->action( "favor_tracks" ) );
                tip += QString("<br><br>") + i18n("%1: %2", a->text().remove( '&' ), a->currentText().remove( '&' ) );
            }

            const QPixmap pix = KIcon( m_action->currentIcon() )
                                .pixmap( style()->pixelMetric(QStyle::PM_SmallIconSize), m_action->isEnabled()
                                                          ? QIcon::Normal
                                                          : QIcon::Disabled );
            m_tooltip = new KPassivePopup( parentWidget()->parentWidget() );
            KHBox *labBox = new KHBox( m_tooltip );
            QLabel *lab = new QLabel( labBox );
            lab->setPixmap( pix );
            QLabel *lab1 = new QLabel( labBox );
            lab1->setText( tip );
            m_tooltip->setView( labBox );
            m_tooltip->show( mapToGlobal( QPoint(parentWidget()->contentsRect().topLeft().x() - 70,
                             parentWidget()->contentsRect().topLeft().y() - 80 ) ) );
        }

        void hideToolTip()
        {
            m_tooltipHidden = true;
            if( m_tooltipShowing )
            {
                m_tooltip->hide();
                m_tooltipShowing = false;
            }
        }

        KPassivePopup      *m_tooltip;
        bool               m_tooltipShowing;
        bool               m_tooltipHidden;

};

#endif
