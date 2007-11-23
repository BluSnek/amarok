// (c) 2005 Ian Monroe <ian@monroe.nu>
// See COPYING file for licensing information.

#define DEBUG_PREFIX "RefreshImages"

#include "refreshimages.h"

#include "amarok.h"
#include "collectiondb.h"
#include "debug.h"
#include "StatusBar.h"

#include <KIO/Job>
#include <kio/jobclasses.h>
#include <KIO/Scheduler>
#include <KLocale>

#include <q3valuelist.h>
#include <QDomDocument>
#include <QDomNode>
#include <QImage>
#include <QMap>
#include <QObject>
#include <QStringList>
#include <QVariant>



RefreshImages::RefreshImages()
{
    //"SELECT asin, locale, filename FROM amazon WHERE refetchdate > %1 ;"
    const QStringList staleImages = CollectionDB::instance()->staleImages();
    QStringList::ConstIterator it = staleImages.begin();
    QStringList::ConstIterator end = staleImages.end();

    while( it != end )
    {
        QString asin=*it;
        it++;
        QString locale = *it;
        it++;
        QString md5sum = *it;
        if ( asin.isEmpty() || locale.isEmpty() || md5sum.isEmpty() )
        {
            //somehow we have entries without ASIN
            if ( !md5sum.isEmpty() ) //I've never seen this, just to be sure
                CollectionDB::instance()->removeInvalidAmazonInfo(md5sum);
            it++;
            if ( it==end )
                deleteLater();

            continue;
        }

        QString url =
            QString("http://webservices.amazon.%1/onca/xml?Service=AWSECommerceService&SubscriptionId=%2&Operation=ItemLookup&ItemId=%3&ResponseGroup=Small,Images")
             .arg(localeToTLD(locale))
             .arg("0RQSQ0B8CRY7VX2VF3G2") //Ian Monroe
             .arg(asin);

        debug() << url;

        KIO::TransferJob* job = KIO::storedGet( url, KIO::NoReload, KIO::HideProgressInfo );
        KIO::Scheduler::scheduleJob( job );

        //Amarok::StatusBar::instance()->newProgressOperation( job );
        job->setObjectName( md5sum );
        it++; //iterate to the next set

        m_jobInfo[md5sum] = JobInfo( asin, locale, it == end );
        connect( job, SIGNAL( result( KIO::Job* ) ), SLOT( finishedXmlFetch( KIO::Job* ) ) );
    }
}

void
RefreshImages::finishedXmlFetch( KIO::Job* xmlJob ) //SLOT
{
    if ( xmlJob->error() )
    {
        Amarok::StatusBar::instance()->shortMessage( i18n( "There was an error communicating with Amazon." ) );
        if ( m_jobInfo[ xmlJob->objectName() ].m_last )
            deleteLater();

        return;
    }

    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( xmlJob );
    const QString xml = QString::fromUtf8( storedJob->data().data(), storedJob->data().size() );

    QDomDocument doc;
    if ( !doc.setContent( xml ) )
      return;

    QStringList imageSizes;
    imageSizes << "LargeImage" << "MediumImage" << "SmallImage";
    QString imageUrl;
    foreach( const QString &size, imageSizes )
    {
        QDomNode imageNode = doc.documentElement()
            .namedItem( "Items" )
            .namedItem( "Item" )
            .namedItem( size );
        if ( !imageNode.isNull() )
        {
            imageUrl = imageNode.namedItem( "URL" ).firstChild().toText().data();
            if( !imageUrl.isEmpty() )
                break;
        }
    }
    debug() << imageUrl;
    KUrl testUrl( imageUrl );
    if( !testUrl.isValid() ) //KIO crashs on empty strings!!!
    {
        //Amazon sometimes takes down covers
        CollectionDB::instance()->removeInvalidAmazonInfo(xmlJob->objectName());
        return;
    }

    KIO::TransferJob* imageJob = KIO::storedGet( imageUrl, KIO::NoReload, KIO::HideProgressInfo );
    KIO::Scheduler::scheduleJob(imageJob);
    //Amarok::StatusBar::instance()->newProgressOperation( imageJob );
    imageJob->setObjectName(xmlJob->objectName());
    //get the URL of the detail page
    m_jobInfo[xmlJob->objectName()].m_detailUrl = doc.documentElement()
       .namedItem( "Items" )
       .namedItem( "Item" )
       .namedItem( "DetailPageURL" ).firstChild().toText().data();
    connect( imageJob, SIGNAL( result(KIO::Job*) ), SLOT( finishedImageFetch(KIO::Job*) ) );
}

void RefreshImages::finishedImageFetch(KIO::Job* imageJob)
{
   if( imageJob->error() ) {
        Amarok::StatusBar::instance()->shortMessage(i18n("There was an error communicating with Amazon."));
        if(m_jobInfo[imageJob->objectName()].m_last)
            deleteLater();

        return;
    }
    QImage img;
    img.loadFromData(static_cast<KIO::StoredTransferJob*>(imageJob)->data());
    img.setText( "amazon-url", 0, m_jobInfo[imageJob->objectName()].m_detailUrl);
    img.save( Amarok::saveLocation("albumcovers/large/") + imageJob->objectName(), "PNG");

    CollectionDB::instance()->newAmazonReloadDate( m_jobInfo[imageJob->objectName()].m_asin
        , m_jobInfo[imageJob->objectName()].m_locale
        , imageJob->objectName());

    if(m_jobInfo[imageJob->objectName()].m_last)
        deleteLater();
}

QString RefreshImages::localeToTLD(const QString& locale)
{
    if(locale=="us")
        return "com";
    else if(locale=="jp")
        return "co.jp";
    else if(locale=="uk")
        return "co.uk";
    else
        return locale;
}

#include "refreshimages.moc"
