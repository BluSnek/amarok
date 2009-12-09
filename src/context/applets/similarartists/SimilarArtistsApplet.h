/****************************************************************************************
 * Copyright (c) 2009 Joffrey Clavel <jclavel@clabert.info>                             *
 * Copyright (c) 2009 Oleksandr Khayrullin <saniokh@gmail.com>                          *
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

#ifndef SIMILAR_ARTISTS_APPLET_H
#define SIMILAR_ARTISTS_APPLET_H


#include "./ArtistWidget.h"

//Amarok
#include "context/Applet.h"
#include "context/DataEngine.h"
#include "EngineObserver.h"

#include <ui_similarArtistsSettings.h>


class QAction;
class TextScrollingWidget;
class KConfigDialog;
class QLabel;
class QGraphicsScene;
class QGraphicsGridLayout;

namespace Plasma
{    
    class IconWidget;
}


 /**
  * SimilarArtists will display similar artists from the Internet, relative to the current playing artist.
  * @author Joffrey Clavel
  * @author Oleksandr Khayrullin
  * @version 0.1
  */
class SimilarArtistsApplet : public Context::Applet, public EngineObserver
{
    Q_OBJECT

public:
    SimilarArtistsApplet( QObject* parent, const QVariantList& args );
    
    void init();

    void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem* option, const QRect& contentsRect );
    void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );

    bool hasHeightForWidth() const;
    qreal heightForWidth( qreal width ) const;

    // inherited from EngineObserver
    virtual void engineNewTrackPlaying();
    virtual void enginePlaybackEnded( qint64 finalPosition, qint64 trackLength, PlaybackEndedReason reason );

protected:
    void createConfigurationInterface(KConfigDialog *parent);

public slots:
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data& data );

private:
    qreal m_aspectRatio;
    qreal m_headerAspectRatio;
    QSizeF m_size;

    /**
     * Layout for the formatting of the applet contents
     */
    QGraphicsGridLayout *m_layout;
    QGraphicsGridLayout *m_tmp;

    QGraphicsScene *m_scene;


    /**
     * Indicates if a track is playing.
     */
    bool  m_stoppedState;
        

    /**
     * Title of the applet (in the top bar)
     */
    TextScrollingWidget *m_headerLabel;

    //Icons on the title right
    Plasma::IconWidget *m_settingsIcon;
    Ui::similarArtistsSettings ui_Settings;

    //elements of the applet
    QList<ArtistWidget*> m_artists;

    int m_maxArtists;
    int m_temp_maxArtists;

private slots:
    void connectSource( const QString &source );

    /**
     * Show the settings windows
     */
    void configure();
    void changeMaxArtists(int value);
    void saveMaxArtists();
    void saveSettings();

};

K_EXPORT_AMAROK_APPLET( similarArtists, SimilarArtistsApplet )

#endif // SIMILARARTISTSAPPLET_H