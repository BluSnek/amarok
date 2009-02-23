

#include "Token.h"
#include "DragStack.h"

#include <QDropEvent>

#include <QtDebug>

/** TokenDragger - eventfilter that drags a token, designed to be a child of DragStack
This is necessary, as if DragStack would QDrag::exec() itself, the eventFilter would be blocked
and thus not be able to handle other events for the parenting widget, like e.g. dragEnter... */

class TokenDragger : public QObject
{
public:
    TokenDragger( const QString &mimeType, DragStack *parent ) : QObject(parent), m_mimeType( mimeType )
    {}
protected:
    bool eventFilter( QObject *o, QEvent *e )
    {
        if ( e->type() == QEvent::MouseMove )
        {
            if ( static_cast<QMouseEvent*>(e)->buttons() & Qt::LeftButton )
                return drag( qobject_cast<Token*>(o) );
        }
        else if ( e->type() == QEvent::MouseButtonPress )
        {
            if ( static_cast<QMouseEvent*>(e)->buttons() & Qt::LeftButton )
            {
                setCursor( qobject_cast<QWidget*>(o), Qt::ClosedHandCursor );
//                 m_startPos = me->pos(); // not sure whether i like this...
//             else if ( event->button() == Qt::MidButton ) // TODO: really kick item on mmbc?
            }
            return false;
        }
        else if ( e->type() == QEvent::MouseButtonRelease )
        {
            if ( static_cast<QMouseEvent*>(e)->buttons() & Qt::LeftButton )
                setCursor( qobject_cast<QWidget*>(o), Qt::OpenHandCursor );
            return false;
        }
        else if ( e->type() == QEvent::FocusIn )
            emit static_cast<DragStack*>( parent() )->focussed( qobject_cast<QWidget*>(o) );
        else if ( e->type() == QEvent::Hide )
        {
            setCursor( qobject_cast<QWidget*>(o), Qt::OpenHandCursor );
            return false;
        }
        return false;
    }

private:
    bool drag( Token *token )
    {
        if ( !token )
            return false;

        bool ret = false;
        bool stacked = token->parentWidget() && qobject_cast<DragStack*>( token->parentWidget()->layout() );
        if (stacked)
            token->hide();
        
        QPixmap pixmap( token->size() );
        token->render( &pixmap );
        QDrag *drag = new QDrag( token );
        QMimeData *data = new QMimeData;
        
        QByteArray itemData;
        QDataStream dataStream( &itemData, QIODevice::WriteOnly );
//         dataStream << child->name() << child->iconName() << child->value();
        
        data->setData( m_mimeType, itemData );
        drag->setMimeData( data );
        drag->setPixmap( pixmap );
        drag->setHotSpot ( pixmap.rect().center() );
        
        Qt::DropAction dropAction = drag->exec( Qt::CopyAction | Qt::MoveAction, Qt::CopyAction );
        
        if ( stacked )
        {
            if ( dropAction != Qt::MoveAction && dropAction != Qt::CopyAction ) // dragged out
            {
                // TODO: nice poof animation? ;-)
                delete token;
                ret = true; // THIS IS IMPORTANT
            }
            // anyway, tell daddy to wipe empty rows NOW
            static_cast<DragStack*>(parent())->deleteEmptyRows();
        }
        return ret;
    }
    inline void setCursor( QWidget *w, Qt::CursorShape shape )
    {
        if ( !w )
            return;
        w->setCursor( shape );
    }
private:
    QString m_mimeType;
    QPoint m_startPos;
};


DragStack::DragStack( const QString &mimeType, QWidget *parent ) : QVBoxLayout( parent ),
m_tokenDragger( new TokenDragger( mimeType, this ) ),
m_tokenFactory( new TokenFactory() )
{
    m_mimeType = mimeType;
    m_limits[0] = m_limits[1] = 0;
    // let daddy widget be droppable... ;)
    parent->setAcceptDrops(true);
    // ...and handle drop events for him
    parent->removeEventFilter( this );
    parent->installEventFilter( this );
    
    // visual, maybe there should be spacing? however, frames etc. can have contentmargin.
    setSpacing( 0 );
    // top-align content
    addItem( new QSpacerItem( 1, 1, QSizePolicy::Expanding, QSizePolicy::MinimumExpanding ) );
}

bool
DragStack::accept( QDropEvent *de )
{
    if ( !de->mimeData()->hasFormat( m_mimeType ) )
    {
        de->ignore();
        return false;
    }
    
    if ( de->source() && parentWidget() && de->source()->parentWidget() == parentWidget() )
    {   // move
        de->setDropAction(Qt::MoveAction);
        de->accept();
    }
    else
        de->acceptProposedAction();
    return true;
}

QHBoxLayout *
DragStack::appendRow()
{
    QHBoxLayout *box = new QHBoxLayout;
    box->setSpacing( 0 );
    box->addStretch();
    insertLayout( QVBoxLayout::count() - 1, box ); // last item is a spacer
    return box;
}

QWidget *
DragStack::childAt( const QPoint &pos ) const
{
    for ( int row = 0; row < QVBoxLayout::count(); ++row )
        if ( QHBoxLayout *rowBox = qobject_cast<QHBoxLayout*>( itemAt( row )->layout() ) )
            for ( int col = 0; col < rowBox->count(); ++col )
                if ( QWidget *kid = rowBox->itemAt( col )->widget() )
                if ( kid->geometry().contains( pos ) )
                    return kid;
    return NULL;
}

void
DragStack::clear()
{
    QLayoutItem *row, *col;
    while( ( row = takeAt( 0 ) ) )
    {
        if ( QLayout *layout = row->layout() )
        {
            while( ( col = layout->takeAt( 0 ) ) )
            {
                delete col->widget();
                delete col;
            }
        }
        delete row;
    }
    //readd our spacer
    addItem( new QSpacerItem( 1, 1, QSizePolicy::Expanding, QSizePolicy::MinimumExpanding ) );
    emit changed();
}

int
DragStack::count( int row ) const
{
    int lower = 0, upper = QVBoxLayout::count() - 1;
    if ( row > -1 && row < QVBoxLayout::count() - 1 )
    {
        lower = row;
        upper = row + 1;
    }
    
    int c = 0;
    for ( row = lower; row < upper; ++row )
        if ( QHBoxLayout *rowBox = qobject_cast<QHBoxLayout*>( itemAt( row )->layout() ) )
            c += rowBox->count() - 1;
    return c;
}

void
DragStack::deleteEmptyRows()
{
    QBoxLayout *box = 0;
    for ( int row = 0; row < QVBoxLayout::count(); )
    {
        box = qobject_cast<QBoxLayout*>( itemAt( row )->layout() );
        if ( box && box->count() < 2 ) // sic! last is spacer
        {
            removeItem( box );
            delete box;
        }
        else
            ++row;
    }
}

QList< Token *>
DragStack::drags( int row )
{
    int lower = 0, upper = QVBoxLayout::count() - 1;
    if ( row > -1 && row < QVBoxLayout::count() - 1 )
    {
        lower = row;
        upper = row + 1;
    }
    
    QList< Token *> list;
    Token *token;
    for ( row = lower; row < upper; ++row )
        if ( QHBoxLayout *rowBox = qobject_cast<QHBoxLayout*>( itemAt( row )->layout() ) )
        {
            for ( int col = 0; col < rowBox->count() - 1; ++col )
                if ( ( token = qobject_cast<Token*>( rowBox->itemAt( col )->widget() ) ) )
                    list << token;
        }
        
        return list;
}

void
DragStack::drop( Token *token, const QPoint &pos )
{
    if ( !token )
        return;
    
    // unlayout in case of move
    if ( QBoxLayout *box = rowBox( token ) )
        box->removeWidget( token );
    token->setParent( parentWidget() );

    QBoxLayout *box = 0;
    if ( Token *brother = qobject_cast<Token*>( childAt( pos ) ) )
    {   // we hit a sibling, -> prepend
        QPoint idx;
        box = rowBox( brother, &idx );
        if ( pos.x() > brother->geometry().x() + 2*brother->width()/3 )
            box->insertWidget( idx.x() + 1, token );
        else
            box->insertWidget( idx.x(), token );
    }
    else
    {
        if ( rowLimit() && QVBoxLayout::count() > (int)rowLimit() ) // we usually don't want more rows
            box = qobject_cast<QBoxLayout*>( itemAt( QVBoxLayout::count() - 2 )->layout() );
        
        if ( !box )
        {
            box = rowBox( pos ); // maybe this is on an existing row
            if ( !box )
                box = appendRow();
        }
        int idx = ( box->count() > 1 && box->itemAt(0)->widget() &&
                    pos.x() < box->itemAt(0)->widget()->geometry().x() ) ? 0 : box->count() - 1;
        box->insertWidget( idx, token ); // append to existing row
    }
    token->show();
    emit changed();
}

bool
DragStack::eventFilter( QObject *o, QEvent *ev )
{

    if ( ev->type() == QEvent::DragMove ||
         ev->type() == QEvent::DragEnter )
    {
        accept( static_cast<QDropEvent*>( ev ) );
        return false; // TODO: return accept boolean ?
    }

//     if ( ev->type() == QEvent::DragLeave )
    if ( ev->type() == QEvent::Drop )
    {
        QDropEvent *de = static_cast<QDropEvent*>( ev );
        if ( accept( de ) )
        {
            Token *token = qobject_cast<Token*>( de->source() );
            if ( !token )
            {
                QByteArray itemData = de->mimeData()->data( m_mimeType );
                QDataStream dataStream(&itemData, QIODevice::ReadOnly);
                
                QString tokenName;
                QString tokenIconName;
                int tokenValue;
                dataStream >> tokenName;
                dataStream >> tokenIconName;
                dataStream >> tokenValue;

                token = m_tokenFactory->createToken( tokenName, tokenIconName, tokenValue, parentWidget() );
                token->removeEventFilter( m_tokenDragger );
                token->installEventFilter( m_tokenDragger );
                token->setCursor( Qt::OpenHandCursor );
            }
            drop( token, de->pos() );
        }
        return false;
    }
    return false;
}

void
DragStack::insertToken( Token *token, int row, int col )
{
    QBoxLayout *box = 0;
    if ( row > QVBoxLayout::count() - 2 )
        box = appendRow();
    else
        box = qobject_cast<QBoxLayout*>( itemAt( row )->layout() );
    token->setParent( parentWidget() );
    if ( col < 0 || col > box->count() - 2 )
        col = box->count() - 1;
    box->insertWidget( col, token );
    token->removeEventFilter( m_tokenDragger );
    token->installEventFilter( m_tokenDragger );
    token->setCursor( Qt::OpenHandCursor );
    emit changed();
}

int
DragStack::row( Token *token ) const
{
    for ( int row = 0; row < QVBoxLayout::count(); ++row )
    {
        QBoxLayout *box = qobject_cast<QBoxLayout*>( itemAt( row )->layout() );
        if ( box && ( box->indexOf( token ) ) > -1 )
            return row;
    }
    return -1;
}

QBoxLayout *
DragStack::rowBox( QWidget *w, QPoint *idx ) const
{
    QBoxLayout *box = 0;
    int col;
    for ( int row = 0; row < QVBoxLayout::count(); ++row )
    {
        box = qobject_cast<QBoxLayout*>( itemAt( row )->layout() );
        if ( box && ( col = box->indexOf( w ) ) > -1 )
        {
            if ( idx )
            {
                idx->setX( col );
                idx->setY( row );
            }
            return box;
        }
    }
    return NULL;
}

QBoxLayout *
DragStack::rowBox( const QPoint &pt ) const
{
    QBoxLayout *box = 0;
    for ( int row = 0; row < QVBoxLayout::count(); ++row )
    {
        box = qobject_cast<QBoxLayout*>( itemAt( row )->layout() );
        if ( !box )
            continue;
        for ( int col = 0; col < box->count(); ++col )
        {
            if ( QWidget *w = box->itemAt( col )->widget() )
            {
                const QRect &geo = w->geometry();
                if ( geo.y() <= pt.y() && geo.bottom() >= pt.y() )
                    return box;
                break; // yes - all widgets are supposed of equal height. we checked on, we checked all
            }
        }
    }
    return NULL;
}

void
DragStack::setCustomTokenFactory( TokenFactory * factory )
{
    delete m_tokenFactory;
    m_tokenFactory = factory;
}

