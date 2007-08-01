/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_CONTEXT_SCENE_H
#define AMAROK_CONTEXT_SCENE_H

#include "amarok_export.h"
#include "Applet.h"
#include "Context.h"
#include <plasma/corona.h>

namespace Context
{

class AMAROK_EXPORT ContextScene : public Plasma::Corona
{
    Q_OBJECT
public:
    explicit ContextScene(QObject * parent = 0);
    explicit ContextScene(const QRectF & sceneRect, QObject * parent = 0);
    explicit ContextScene(qreal x, qreal y, qreal width, qreal height, QObject * parent = 0);
    ~ContextScene();
    
public slots:
    
    Applet* addApplet(const QString& name, const QStringList& args = QStringList());
    
    void clear();
    void clear( const ContextState& state );
    
protected:
    
    void launchExplorer() {};
    /*void dragEnterEvent(QGraphicsSceneDragDropEvent* event);
    void dropEvent(QGraphicsSceneDragDropEvent* event);
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *contextMenuEvent); */
    
private:
    typedef QPointer<Applet> AppletPointer;
    QList<AppletPointer> m_loaded;
};

} // Context namespace

#endif
