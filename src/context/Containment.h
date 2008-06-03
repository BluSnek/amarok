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

#ifndef AMAROK_CONTAINMENT_H
#define AMAROK_CONTAINMENT_H

#include "amarok_export.h"

#include "plasma/containment.h"

#include <QAction>

namespace Context
{


class AMAROK_EXPORT Containment : public Plasma::Containment
{
    Q_OBJECT
public:
    explicit Containment(QGraphicsItem* parent = 0,
                         const QString& serviceId = QString(),
                         uint containmentId = 0);
    
    Containment(QObject* parent, const QVariantList& args);
    
//     ~Containment();
    
    virtual void saveToConfig( KConfig& conf ) = 0;
    virtual void loadConfig( KConfig& conf ) = 0;
    
    virtual void updateSize() = 0;
};

} // Context namespace
#endif
