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

#include "ServiceInfo.h"

#include "amarok.h"
#include "debug.h"
#include "context/Svg.h"

#include <QPainter>
#include <QBrush>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>

ServiceInfo::ServiceInfo( QObject* parent, const QStringList& args )
    : Plasma::Applet( parent, args )
    , m_config( 0 )
    , m_configLayout( 0 )
    , m_width( 0 )
    , m_aspectRatio( 0.0 )
    , m_size( QSizeF() )

{
    DEBUG_BLOCK
        
    setHasConfigurationInterface( true );
    setDrawStandardBackground( false );
    
    dataEngine( "amarok-service" )->connectSource( "service", this );
    
    m_theme = new Context::Svg( "widgets/amarok-serviceinfo", this );
    m_theme->setContentType( Context::Svg::SingleImage );
    m_width = globalConfig().readEntry( "width", 500 );
    
    m_serviceName = new QGraphicsSimpleTextItem( this );
  
    
    m_serviceName->setBrush( QBrush( Qt::white ) );
   
    // get natural aspect ratio, so we can keep it on resize
    m_theme->resize();
    m_aspectRatio = (qreal)m_theme->size().height() / (qreal)m_theme->size().width();
    resize( m_width, m_aspectRatio ); 
    
    constraintsUpdated();
}

ServiceInfo::~ServiceInfo()
{
    DEBUG_BLOCK
}

void ServiceInfo::setRect( const QRectF& rect )
{
    DEBUG_BLOCK
    setPos( rect.topLeft() );
    resize( rect.width(), m_aspectRatio );
}

QSizeF ServiceInfo::contentSize() const
{
    return m_size;
}

void ServiceInfo::constraintsUpdated()
{
    prepareGeometryChange();

    m_serviceName->setPos( m_theme->elementRect( "track" ).topLeft() );

}

void ServiceInfo::updated( const QString& name, const Plasma::DataEngine::Data& data )
{
    DEBUG_BLOCK
    Q_UNUSED( name );
    
    if( data.size() == 0 ) return;
    
    kDebug() << "got data from engine: " << data[ "service_name" ].toString();
    m_serviceName->setText( data[ "service_name" ].toString() );
    
}

void ServiceInfo::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option );
        
    p->save();
    m_theme->paint( p, contentsRect, "background" );
    p->restore();
        
        
    m_serviceName->setPos( m_theme->elementRect( "track" ).topLeft() );

    
}

void ServiceInfo::showConfigurationInterface()
{

}

void ServiceInfo::configAccepted() // SLOT
{

}

void ServiceInfo::resize( qreal newWidth, qreal aspectRatio )
{
    qreal height = aspectRatio * newWidth;
    m_size.setWidth( newWidth );
    m_size.setHeight( height );
    
    m_theme->resize( m_size );
    kDebug() << "set new size: " << m_size;
    constraintsUpdated();
}

#include "ServiceInfo.moc"
