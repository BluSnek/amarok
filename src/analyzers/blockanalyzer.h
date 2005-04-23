// Maintainer: Max Howell <mac.howell@methylblue.com>, (C) 2003-4
// Copyright:  See COPYING file that comes with this distribution
//

#ifndef BLOCKANALYZER_H
#define BLOCKANALYZER_H

#include "analyzerbase.h"
#include <qcolor.h>

/**
@author Max Howell
*/

class QResizeEvent;
class QMouseEvent;
class QPalette;

class BlockAnalyzer : public Analyzer::Base2D
{
public:
    BlockAnalyzer( QWidget* );
   ~BlockAnalyzer();

    static const uint HEIGHT      = 2;
    static const uint WIDTH       = 4;
    static const uint MIN_ROWS    = 3;   //arbituary
    static const uint MIN_COLUMNS = 32;  //arbituary
    static const uint MAX_COLUMNS = 256; //must be 2**n

protected:
    virtual void transform( Scope& );
    virtual void analyze( const Scope& );
    virtual void resizeEvent( QResizeEvent* );
    virtual void mousePressEvent( QMouseEvent* );
    virtual void paletteChange( const QPalette& );

    void drawBackground();
    void determineStep();

private:
    QPixmap* const bar() { return &m_barPixmap; }

    uint m_columns, m_rows;      //number of rows and columns of blocks
    uint m_y;                    //y-offset from top of widget
    QPixmap m_barPixmap;
    Scope m_scope;               //so we don't create a vector every frame
    std::vector<float> m_store;  //current bar heights
    std::vector<float> m_yscale;

    //FIXME why can't I namespace these? c++ issue?
    std::vector<QColor> m_fade_color;
    std::vector<uint>   m_fade_pos;
    std::vector<int>    m_fade_intensity;

    float m_step; //rows to fall per frame

    QColor m_topBarColor;
};

#endif
