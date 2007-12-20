// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2006 Paul Cifarelli <paul@cifarelli.net>
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_GST_STREAMSRC_H
#define AMAROK_GST_STREAMSRC_H

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_STREAMSRC \
  (gst_streamsrc_get_type())
#define GST_STREAMSRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_STREAMSRC,GstStreamSrc))
#define GST_STREAMSRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_STREAMSRC,GstStreamSrcClass))
#define GST_IS_STREAMSRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_STREAMSRC))
#define GST_IS_STREAMSRC_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_STREAMSRC))


GType gst_streamsrc_get_type();

typedef struct _GstStreamSrc GstStreamSrc;
typedef struct _GstStreamSrcClass GstStreamSrcClass;

struct _GstStreamSrc
{
    GstElement element;
    /* pads */
    GstPad *srcpad;

    bool stopped;
    long curoffset;

    // Properties
    glong blocksize; /* Bytes per read */
    guint64 timeout;  /* Read timeout, in nanoseconds */
    guint buffer_min; /* Minimum buffer fill */
    guint buffer_resume; /*Resume KIO transfer at this point*/

    // Pointers to member variables of GstEngine
    char* m_buf;
    int* m_bufIndex;
    bool* m_bufStop;
    bool* m_buffering;
};

struct _GstStreamSrcClass
{
    GstElementClass parent_class;

    /* signals */
    void ( *timeout ) ( GstElement *element );
    void ( *kio_resume ) ( GstElement *element );
};


GstStreamSrc* gst_streamsrc_new ( char* buf, int* index, bool* stop, bool* buffering );
void gst_streamsrc_set_property( GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec );
void gst_streamsrc_get_property( GObject * object, guint prop_id, GValue * value, GParamSpec * pspec );
GstStateChangeReturn gst_streamsrc_change_state(GstElement* element, GstStateChange trans );
GstFlowReturn gst_streamsrc_get( GstPad * pad, guint64 offset, guint length, GstBuffer **buffer );
void gst_streamsrc_dispose( GObject *object );


G_END_DECLS

#endif /* AMAROK_GST_STREAMSRC_H */



