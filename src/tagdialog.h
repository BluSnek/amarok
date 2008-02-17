// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Pierpaolo Di Panfilo <pippo_dp@libero.it>
// (c) 2005 Alexandre Pereira de Oliveira <aleprj@gmail.com>
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_TAGDIALOG_H
#define AMAROK_TAGDIALOG_H

#include "config-amarok.h"

#include "ktrm.h"
#include "playlist/PlaylistItem.h"

#include "meta/Meta.h"

#include <khtml_part.h>
#include <KDialog>

#include <QDateTime>
#include <QLabel>
#include <QListIterator>
#include <QMap>
#include <QVariant>
#include <QtGui/QWidget>


namespace TagLib {
    namespace ID3v2 {
        class Tag;
    }
}

namespace Ui
{
    class TagDialogBase;
}

class QueryMaker;

class TagDialog : public KDialog
{
    Q_OBJECT

    public:

        enum Changes { NOCHANGE=0, SCORECHANGED=1, TAGSCHANGED=2, LYRICSCHANGED=4, RATINGCHANGED=8, LABELSCHANGED=16 };
        enum Tabs { SUMMARYTAB, TAGSTAB, LYRICSTAB, STATSTAB, LABELSTAB };

        explicit TagDialog( const Meta::TrackList &tracks, QWidget *parent = 0 );
        explicit TagDialog( Meta::TrackPtr track, QWidget *parent = 0 );
        explicit TagDialog( QueryMaker *qm );
        ~TagDialog();

        void setTab( int id );

        friend class TagSelect;

    signals:
        void lyricsChanged( const QString& );

    public slots:
        void openUrlRequest(const KUrl &url );

    private slots:
        void accept();
        void cancelPressed();
        void openPressed();
        void previousTrack();
        void nextTrack();
        void perTrack();
        void checkModified();

        void loadCover();

        void musicbrainzQuery();
        void guessFromFilename();
        void setFileNameSchemes();
        void queryDone( KTRMResultList results, QString error );
        void fillSelected( KTRMResult selected );
        void resetMusicbrainz();

        void resultReady( const QString &collectionId, const Meta::TrackList &tracks );
        void queryDone();

    private:
        void init();
        void readTags();
        void readMultipleTracks();
        void setMultipleTracksMode();
        void setSingleTrackMode();
        void enableItems();
        bool hasChanged();
        int changes();
        void storeTags();
        void storeTags( const Meta::TrackPtr &track );
        void storeTags( const Meta::TrackPtr &track, int changes, const QVariantMap &data );
        void storeLabels( const Meta::TrackPtr &track, const QStringList &labels );
        void loadTags( const Meta::TrackPtr &track );
        void loadLyrics( const Meta::TrackPtr &track );
        void loadLabels( const Meta::TrackPtr &track );
        QVariantMap dataForTrack( const Meta::TrackPtr &track );
        double scoreForTrack( const Meta::TrackPtr &track );
        int ratingForTrack( const Meta::TrackPtr &track );
        QString lyricsForTrack( const Meta::TrackPtr &track );
        QStringList labelsForTrack( const Meta::TrackPtr &track );
        QStringList getCommonLabels();
        void saveTags();
        const QString unknownSafe( QString );
        const QStringList statisticsData();
        void applyToAllTracks();

        const QStringList filenameSchemes();

        QStringList labelListFromText( const QString &text );
        void generateDeltaForLabelList( const QStringList &list );
        QString generateHTML( const QStringList &labels );

        QString m_lyrics;
        bool m_perTrack;
        QMap<Meta::TrackPtr, QVariantMap > storedTags;
        QMap<Meta::TrackPtr, double> storedScores;
        QMap<Meta::TrackPtr, int> storedRatings;
        QMap<Meta::TrackPtr, QString> storedLyrics;
        QMap<Meta::TrackPtr, QStringList> newLabels;
        QMap<Meta::TrackPtr, QStringList> originalLabels;
        QString m_buttonMbText;
        QString m_path;
        QString m_currentCover;
        QStringList m_labels;
        QStringList m_addedLabels;
        QStringList m_removedLabels;
        QString m_commaSeparatedLabels;
        KHTMLPart *m_labelCloud;
        //HTMLView *m_labelCloud;

        //2.0 stuff
        Meta::TrackList m_tracks;
        Meta::TrackPtr m_currentTrack;
        QListIterator<Meta::TrackPtr > m_trackIterator;
        QVariantMap m_currentData;
        QueryMaker *m_queryMaker;

        Ui::TagDialogBase *ui;
};


#endif /*AMAROK_TAGDIALOG_H*/

