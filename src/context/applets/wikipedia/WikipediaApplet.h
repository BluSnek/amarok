/***************************************************************************
 * copyright      : (C) 2007 Leo Franchi <lfranchi@gmail.com>              *
 * copyright      : (C) 2009 Simon Esneault <simon.esneault@gmail.com>     *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef WIKIPEDIA_APPLET_H
#define WIKIPEDIA_APPLET_H

#include "context/Applet.h"
#include "context/DataEngine.h"
#include "context/Svg.h"

#include <ktemporaryfile.h>
#include <plasma/framesvg.h>

#include <QGraphicsProxyWidget>
#include <qwebview.h>

#include <ui_wikipediaSettings.h>

class QAction;
class QGraphicsSimpleTextItem;
class QGraphicsTextItem;
class KDialog;
class KConfigDialog;

namespace Plasma
{
    class WebView;
    class IconWidget;
}

class WikipediaApplet : public Context::Applet
{
    Q_OBJECT
public:
    WikipediaApplet( QObject* parent, const QVariantList& args );
    ~WikipediaApplet();

    void init();
    void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem* option, const QRect& contentsRect );

    void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );

    bool hasHeightForWidth() const;
    qreal heightForWidth( qreal width ) const;

    virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF & constraint) const;

protected:
    void createConfigurationInterface(KConfigDialog *parent);


    
public slots:
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data& data );

private:
    Plasma::IconWidget * addAction( QAction *action );

    qreal m_aspectRatio;
    qreal m_headerAspectRatio;
    QSizeF m_size;

    QGraphicsSimpleTextItem* m_wikipediaLabel;

    Plasma::WebView * m_webView;

    QString m_label;
    QString m_title;

    Plasma::IconWidget *m_backwardIcon;
    Plasma::IconWidget *m_forwardIcon;
    Plasma::IconWidget *m_artistIcon;
    Plasma::IconWidget *m_albumIcon;
    Plasma::IconWidget *m_trackIcon;
    Plasma::IconWidget *m_settingsIcon;
    Plasma::IconWidget *m_reloadIcon;

    Ui::wikipediaSettings ui_Settings;
    
    
    KTemporaryFile* m_css;

    QList <QString> m_histoBack;
    QList <QString> m_histoFor;
    QString m_current;

    QString m_wikiPreferredLang;

    bool m_gotMessage;
    
private slots:
    void connectSource( const QString &source );
    void linkClicked( const QUrl &url );
    
    void goBackward();    
    void goForward();
    void gotoArtist();
    void gotoAlbum();
    void gotoTrack();

    void switchLang();
    void switchToLang(QString lang);
    void reloadWikipedia();
    
    void paletteChanged( const QPalette & palette );

};

K_EXPORT_AMAROK_APPLET( wikipedia, WikipediaApplet )

#endif
