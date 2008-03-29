/******************************************************************************
 * copyright: (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>       *
 *            (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com> *
 *            (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>         *
 *   This program is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License version 2           *
 *   as published by the Free Software Foundation.                            *
 ******************************************************************************/




#define DEBUG_PREFIX "CollectionTreeItemModelBase"

#include "CollectionTreeItemModelBase.h"

#include "amarok.h"
#include "AmarokMimeData.h"
#include "Collection.h"
#include "CollectionManager.h"
#include "CollectionTreeItem.h"
#include "debug.h"
#include "expression.h"
#include "QueryMaker.h"

#include <KIcon>
#include <KIconLoader>
#include <KLocale>
#include <KStandardDirs>
#include <QPixmap>
#include <QTimer>

using namespace Meta;

inline uint qHash( const Meta::DataPtr &data )
{
    return qHash( data.data() );
}


CollectionTreeItemModelBase::CollectionTreeItemModelBase( )
    :QAbstractItemModel()
    , m_rootItem( 0 )
    , d( new Private )
    , m_animFrame( 0 )
    , m_loading1( QPixmap( KStandardDirs::locate("data", "amarok/images/loading1.png" ) ) )
    , m_loading2( QPixmap( KStandardDirs::locate("data", "amarok/images/loading2.png" ) ) )
    , m_currentAnimPixmap( m_loading1 )
{


    m_timeLine = new QTimeLine( 10000, this );
    m_timeLine->setFrameRange( 0, 20 );
    m_timeLine->setLoopCount ( 0 );
    connect( m_timeLine, SIGNAL( frameChanged( int ) ), this, SLOT( loadingAnimationTick() ) );


}


CollectionTreeItemModelBase::~CollectionTreeItemModelBase()
{
    delete m_rootItem;
    delete d;
}

Qt::ItemFlags CollectionTreeItemModelBase::flags(const QModelIndex & index) const
{
    if ( !index.isValid() )
        return Qt::ItemIsEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

QVariant
CollectionTreeItemModelBase::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        if (section == 0)
            return m_headerText;
    return QVariant();
}

QModelIndex
CollectionTreeItemModelBase::index(int row, int column, const QModelIndex & parent) const
{
    CollectionTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<CollectionTreeItem*>(parent.internalPointer());

   //if ( parentItem->childrenLoaded() )
   //{
        CollectionTreeItem *childItem = parentItem->child(row);
        if (childItem) {
            return createIndex(row, column, childItem);
        } else {
            return QModelIndex();
        }
    //}
    //else
    //   return QModelIndex();
}

QModelIndex
CollectionTreeItemModelBase::parent(const QModelIndex & index) const
{
     if (!index.isValid())
         return QModelIndex();

     CollectionTreeItem *childItem = static_cast<CollectionTreeItem*>(index.internalPointer());
     CollectionTreeItem *parentItem = childItem->parent();

     if ( (parentItem == m_rootItem) || !parentItem )
         return QModelIndex();

     return createIndex(parentItem->row(), 0, parentItem);
}

int
CollectionTreeItemModelBase::rowCount(const QModelIndex & parent) const
{

    CollectionTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<CollectionTreeItem*>(parent.internalPointer());

    if ( parentItem->childrenLoaded() )
        return parentItem->childCount();
    else
        return 0;

}

int CollectionTreeItemModelBase::columnCount(const QModelIndex & parent) const
{
    Q_UNUSED( parent )
    return 1;
}

QStringList
CollectionTreeItemModelBase::mimeTypes() const
{
    QStringList types;
    types << AmarokMimeData::TRACK_MIME;
    return types;
}

QMimeData * CollectionTreeItemModelBase::mimeData(const QModelIndexList & indices) const
{
    if ( indices.isEmpty() )
        return 0;

    Meta::TrackList tracks;
    QList<QueryMaker*> queries;

    foreach( const QModelIndex &index, indices ) {
        if (index.isValid()) {
            CollectionTreeItem *item = static_cast<CollectionTreeItem*>(index.internalPointer());
            if( item->allDescendentTracksLoaded() )
                tracks << item->descendentTracks();
            else
            {
                QueryMaker *qm = item->queryMaker();
                CollectionTreeItem *tmpItem = item;
                while ( tmpItem->isDataItem()  ) {
                    if ( tmpItem->data() )
                        qm->addMatch( tmpItem->data() );
                    else
                        qm->setAlbumQueryMode( QueryMaker::OnlyCompilations );
                    tmpItem = tmpItem->parent();
                }
                addFilters( qm );
                queries.append( qm );
            }
        }
    }

    AmarokMimeData *mimeData = new AmarokMimeData();
    mimeData->setTracks( tracks );
    mimeData->setQueryMakers( queries );
    mimeData->startQueries();
    return mimeData;
}

QPixmap
CollectionTreeItemModelBase::iconForLevel(int level) const
{
    QString icon;
        switch( m_levelType[level] ) {
        case CategoryId::Album :
            icon = "view-media-album-amarok";
            break;
        case CategoryId::Artist :
            icon = "view-media-artist-amarok";
            break;
        case CategoryId::Composer :
            icon = "view-media-artist-amarok";
            break;

        case CategoryId::Genre :
            icon = "kfm";
            break;

        case CategoryId::Year :
            icon = "clock";
            break;
    }
    return KIconLoader::global()->loadIcon( icon, KIconLoader::Toolbar, KIconLoader::SizeSmall );
}

void CollectionTreeItemModelBase::listForLevel(int level, QueryMaker * qm, CollectionTreeItem * parent) const
{
    //DEBUG_BLOCK
    if ( qm && parent ) {

        //this check should not hurt anyone... needs to check if single... needs it
        for( QMapIterator<QueryMaker*, CollectionTreeItem*> iter( d->m_childQueries ); iter.hasNext(); ) {
            if( iter.next().value() == parent )
                return;             //we are already querying for children of parent
        }
        if ( level > m_levelType.count() )
            return;
        if ( level == m_levelType.count() ) {
            qm->startTrackQuery();
        }
        else {
            switch( m_levelType[level] ) {
                case CategoryId::Album :
                    qm->startAlbumQuery();
                    //restrict query to normal albums if the previous level
                    //was the artist category. in that case we handle compilations below
                    if( level > 0 && m_levelType[level-1] == CategoryId::Artist )
                    {
                        qm->setAlbumQueryMode( QueryMaker::OnlyNormalAlbums );
                    }
                    break;
                case CategoryId::Artist :
                    qm->startArtistQuery();
                    //handle compilations only if the next level ist CategoryId::Album
                    if( level + 1 < m_levelType.count() && m_levelType[level+1] == CategoryId::Album )
                    {
                        handleCompilations( parent );
                        qm->setAlbumQueryMode( QueryMaker::OnlyNormalAlbums );
                    }
                    break;
                case CategoryId::Composer :
                    qm->startComposerQuery();
                    break;
                case CategoryId::Genre :
                    qm->startGenreQuery();
                    break;
                case CategoryId::Year :
                    qm->startYearQuery();
                    break;
                default : //TODO handle error condition. return tracks?
                    break;
            }
        }
        CollectionTreeItem *tmpItem = parent;
        while ( tmpItem->isDataItem()  ) {
            //ignore Various artists node (whichh will not have a data pointer
            if( tmpItem->data() )
            {
                qm->addMatch( tmpItem->data() );
            }
            tmpItem = tmpItem->parent();
        }
        addFilters( qm );
        qm->returnResultAsDataPtrs( true );
        connect( qm, SIGNAL( newResultReady( QString, Meta::DataList ) ), SLOT( newResultReady( QString, Meta::DataList ) ), Qt::QueuedConnection );
        connect( qm, SIGNAL( queryDone() ), SLOT( queryDone() ), Qt::QueuedConnection );
        d->m_childQueries.insert( qm, parent );
        qm->run();

       //start animation
       if ( ( m_timeLine->state() != QTimeLine::Running ) && ( parent != m_rootItem ) )
           m_timeLine->start();

    }
}


void
CollectionTreeItemModelBase::addFilters(QueryMaker * qm) const
{
    ParsedExpression parsed = ExpressionParser::parse ( m_currentFilter );
    foreach( or_list orList, parsed )
    {
        foreach ( expression_element elem, orList )
        {
            if ( elem.field.isEmpty() )
            {
                qm->beginOr();
                foreach ( int level, m_levelType )
                {
                    qint64 value;
                    switch ( level )
                    {
                        case CategoryId::Album:
                            value = QueryMaker::valAlbum;
                            break;
                        case CategoryId::Artist:
                            value = QueryMaker::valArtist;
                            break;
                        case CategoryId::Composer:
                            value = QueryMaker::valComposer;
                            break;
                        case CategoryId::Genre:
                            value = QueryMaker::valGenre;
                            break;
                        case CategoryId::Year:
                            value = QueryMaker::valYear;
                            break;
                        default:
                            value = -1;
                            break;
                    }
                    qm->addFilter ( value, elem.text, false, false );
                }
                qm->addFilter ( QueryMaker::valTitle, elem.text, false, false ); //always filter for track title too
                qm->endAndOr();
            }
            else
            {
                //get field values based on name
                qint64 value;
                QString lcField = elem.field.toLower();

                if ( lcField.compare( "album", Qt::CaseInsensitive ) == 0 || lcField.compare( i18n( "album" ), Qt::CaseInsensitive ) == 0 )
                {
                    value = QueryMaker::valAlbum;
                } 
                else if ( lcField.compare( "artist", Qt::CaseInsensitive ) == 0 || lcField.compare( i18n( "artist" ), Qt::CaseInsensitive ) == 0 )
                {
                    value = QueryMaker::valArtist;
                }
                else if ( lcField.compare( "genre", Qt::CaseInsensitive ) == 0 || lcField.compare( i18n( "genre" ), Qt::CaseInsensitive ) == 0)
                {
                    value = QueryMaker::valGenre;
                }
                else if ( lcField.compare( "composer", Qt::CaseInsensitive ) == 0|| lcField.compare( i18n( "composer" ), Qt::CaseInsensitive ) == 0 )
                {
                    value = QueryMaker::valComposer;
                }
                else if ( lcField.compare( "year", Qt::CaseInsensitive ) == 0 || lcField.compare( i18n( "year" ), Qt::CaseInsensitive ) == 0)
                {
                    value = QueryMaker::valYear;
                }
                else
                {
                    value = -1;
                }
                
                qm->addFilter ( value, elem.text, false, false );

            }
        }
    }
}

void
CollectionTreeItemModelBase::queryDone()
{
    QueryMaker *qm = static_cast<QueryMaker*>( sender() );
    CollectionTreeItem* item = d->m_childQueries.contains( qm ) ? d->m_childQueries.take( qm ) : d->m_compilationQueries.take( qm );

    //reset icon for this item
    if ( item != m_rootItem )
        emit ( dataChanged ( createIndex(item->row(), 0, item), createIndex(item->row(), 0, item) ) );

    //stop timer if there are no more animations active

    if (d->m_childQueries.count() == 0 /*&& d->m_compilationQueries.count() == 0 */ )
        m_timeLine->stop();
    qm->deleteLater();
}

void
CollectionTreeItemModelBase::newResultReady(const QString & collectionId, Meta::DataList data)
{
    Q_UNUSED( collectionId )
    if ( data.count() == 0 )
        return;
    //if we are expanding an item, we'll find the sender in m_childQueries
    //otherwise we are filtering all collections
    QueryMaker *qm = static_cast<QueryMaker*>( sender() );
    if ( d->m_childQueries.contains( qm ) ) {
        CollectionTreeItem *parent = d->m_childQueries.value( qm );
        QModelIndex parentIndex;
        if (parent == m_rootItem ) // will never happen in CollectionTreeItemModel
        {
            parentIndex = QModelIndex();
        }
        else
        {
            parentIndex = createIndex( parent->row(), 0, parent );
        }

        //add new rows after existing ones here (which means all artists nodes
        //will be inserted after the "Various Artists" node
        beginInsertRows( parentIndex, parent->childCount(), parent->childCount() + data.count()-1 );
        populateChildren( data, parent );
        endInsertRows();

        for( int count = parent->childCount(), i = 0; i < count; i++ )
        {
            CollectionTreeItem *item = parent->child( i );
            if ( m_expandedItems.contains( item->data() ) ) //item will always be a data item
            {
                listForLevel( item->level(), item->queryMaker(), item );
            }
        }

        if ( parent->isDataItem() )
        {
            if ( m_expandedItems.contains( parent->data() ) )
            {
                emit expandIndex( parentIndex );
            }
            else
                //simply insert the item, nothing will change if it is already in the set
                m_expandedItems.insert( parent->data() );
        }
        else
        {
            m_expandedCollections.insert( parent->parentCollection() );
        }
    }
    else if( d->m_compilationQueries.contains( qm ) )
    {
        CollectionTreeItem *parent = d->m_compilationQueries.value( qm );
        QModelIndex parentIndex;
        if (parent == m_rootItem ) // will never happen in CollectionTreeItemModel
        {
            parentIndex = QModelIndex();
        }
        else
        {
            parentIndex = createIndex( parent->row(), 0, parent );
        }
        //we only insert the "Various Artists" node
        beginInsertRows( parentIndex, 0, 0 );
        new CollectionTreeItem( data, parent );
        endInsertRows();
    }
}

void
CollectionTreeItemModelBase::populateChildren(const DataList & dataList, CollectionTreeItem * parent) const
{
    foreach( Meta::DataPtr data, dataList ) {
        new CollectionTreeItem( data, parent );
    }
    parent->setChildrenLoaded( true );
}

void
CollectionTreeItemModelBase::updateHeaderText()
{
    m_headerText.clear();
    for( int i=0; i< m_levelType.count(); ++i ) {
        m_headerText += nameForLevel( i ) + " / ";
    }
    m_headerText.chop( 3 );
}

QString
CollectionTreeItemModelBase::nameForLevel(int level) const
{
    switch( m_levelType[level] ) {
        case CategoryId::Album : return i18n( "Album" );
        case CategoryId::Artist : return i18n( "Artist" );
        case CategoryId::Composer : return i18n( "Composer" );
        case CategoryId::Genre : return i18n( "Genre" );
        case CategoryId::Year : return i18n( "Year" );
        default: return QString();
    }
}

void
CollectionTreeItemModelBase::handleCompilations( CollectionTreeItem *parent ) const
{
    //this method will be called when we retrieve a list of artists from the database.
    //we have to query for all compilations, and then add a "Various Artists" node if at least
    //one compilation exists
    QueryMaker *qm = parent->queryMaker();
    qm->setAlbumQueryMode( QueryMaker::OnlyCompilations );
    qm->startAlbumQuery();
    CollectionTreeItem *tmpItem = parent;
    while ( tmpItem->isDataItem()  ) {
        //ignore Various artists node (which will not have a data pointer)
        if( tmpItem->data() )
        {
            qm->addMatch( tmpItem->data() );
        }
        tmpItem = tmpItem->parent();
    }
    addFilters( qm );
    qm->returnResultAsDataPtrs( true );
    connect( qm, SIGNAL( newResultReady( QString, Meta::DataList ) ), SLOT( newResultReady( QString, Meta::DataList ) ), Qt::QueuedConnection );
    connect( qm, SIGNAL( queryDone() ), SLOT( queryDone() ), Qt::QueuedConnection );
    d->m_compilationQueries.insert( qm, parent );
    qm->run();
}

void CollectionTreeItemModelBase::loadingAnimationTick()
{
    if ( m_animFrame == 0 )
        m_currentAnimPixmap = m_loading2;
    else
        m_currentAnimPixmap = m_loading1;

    m_animFrame = 1 - m_animFrame;


    //trigger an update of all items being populated at the moment;
    QList<CollectionTreeItem* > items = d->m_childQueries.values();

    foreach ( CollectionTreeItem* item, items ) {
        emit ( dataChanged ( createIndex(item->row(), 0, item), createIndex(item->row(), 0, item) ) );
    }

}

void
CollectionTreeItemModelBase::setCurrentFilter( const QString &filter )
{
    m_currentFilter = filter;
}

void
CollectionTreeItemModelBase::slotFilter()
{
    filterChildren();
    reset();
    if ( !m_expandedCollections.isEmpty() )
    {
        foreach( Collection *expanded, m_expandedCollections )
        {
            CollectionTreeItem *expandedItem = d->m_collections.value( expanded->collectionId() ).second;
            if ( expandedItem == 0 ) {
                debug() << "ARRG! expandedItem is 0!!! id=" << expanded->collectionId() ;
            } else {
                emit expandIndex( createIndex( expandedItem->row(), 0, expandedItem ) );
            }
        }
    }
}

void
CollectionTreeItemModelBase::slotCollapsed( const QModelIndex &index )
{
    DEBUG_BLOCK
    if ( index.isValid() )      //probably unnecessary, but let's be safe
    {
        CollectionTreeItem *item = static_cast<CollectionTreeItem*>( index.internalPointer() );
        if ( item->isDataItem() )
        {
            m_expandedItems.remove( item->data() );
        }
        else
        {
            m_expandedCollections.remove( item->parentCollection() );
        }
    }
}

void CollectionTreeItemModelBase::update()
{
   reset();
}


#include "CollectionTreeItemModelBase.moc"

