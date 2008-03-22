/***************************************************************************
                       amarokslider.h  -  description
                          -------------------
 begin                : Dec 15 2003
 copyright            : (C) 2003 by Mark Kretschmann
 email                : markey@web.de
 copyright            : (C) 2005 by Gábor Lehel
 email                : illissius@gmail.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROKSLIDER_H
#define AMAROKSLIDER_H

#include <QPixmap>
#include <QSlider>
#include <QVector>

class QPalette;
class QSvgRenderer;
class QTimer;

namespace Amarok
{
    class Slider : public QSlider
    {
        Q_OBJECT

        public:
            Slider( Qt::Orientation, QWidget*, uint max = 0 );

            virtual void setValue( int );

            //WARNING non-virtual - and thus only really intended for internal use
            //this is a major flaw in the class presently, however it suits our
            //current needs fine
            int value() const { return adjustValue( QSlider::value() ); }

        signals:
            //we emit this when the user has specifically changed the slider
            //so connect to it if valueChanged() is too generic
            //Qt also emits valueChanged( int )
            void sliderReleased( int );

        protected:
            virtual void wheelEvent( QWheelEvent* );
            virtual void mouseMoveEvent( QMouseEvent* );
            virtual void mouseReleaseEvent( QMouseEvent* );
            virtual void mousePressEvent( QMouseEvent* );
            virtual void slideEvent( QMouseEvent* );

            bool m_sliding;

            /// we flip the value for vertical sliders
            int adjustValue( int v ) const
            {
               int mp = (minimum() + maximum()) / 2;
               return orientation() == Qt::Vertical ? mp - (v - mp) : v;
            }

        private:
            bool m_outside;
            int  m_prevValue;

            Q_DISABLE_COPY( Slider );

    };

    class VolumeSlider: public Slider
    {
        Q_OBJECT

        public:
            explicit VolumeSlider( QWidget *parent, uint max = 0 );

        protected:
            virtual void paintEvent( QPaintEvent* );

            virtual void enterEvent( QEvent* );
            virtual void leaveEvent( QEvent* );
            virtual void paletteChange( const QPalette& );
            virtual void slideEvent( QMouseEvent* );
            virtual void mousePressEvent( QMouseEvent* );
            virtual void contextMenuEvent( QContextMenuEvent* );
            virtual void wheelEvent( QWheelEvent *e );
            virtual void resizeEvent(QResizeEvent * event);

        private slots:
            virtual void slotAnimTimer();

        private:
            //void generateGradient();

            Q_DISABLE_COPY( VolumeSlider );

            ////////////////////////////////////////////////////////////////
            static const int ANIM_INTERVAL = 18;
            static const int ANIM_MAX = 18;

            bool    m_animEnter;
            int     m_animCount;
            QTimer* m_animTimer;


            int m_iconHeight;
            int m_iconWidth;
            int m_textWidth;
            int m_sliderWidth;
            int m_sliderHeight;
            int m_sliderX;

            int m_margin;

            //QPixmap m_pixmapInset;
            //QPixmap m_pixmapGradient;

            QVector<QPixmap> m_handlePixmaps;
            QSvgRenderer * m_svgRenderer;
    };

    class TimeSlider : public Amarok::Slider
    {
        Q_OBJECT;

        public:
            TimeSlider( QWidget *parent );

        protected:
            virtual void paintEvent( QPaintEvent* );

            virtual void paletteChange( const QPalette& );
            virtual void resizeEvent(QResizeEvent * event);

        private:
            Q_DISABLE_COPY( TimeSlider );
            ////////////////////////////////////////////////////////////////

            int m_sliderHeight;

            QSvgRenderer * m_svgRenderer;
    };

}

#endif
