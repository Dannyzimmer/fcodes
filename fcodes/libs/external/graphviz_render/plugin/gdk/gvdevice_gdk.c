/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include "config.h"
#include <assert.h>
#include <cgraph/unreachable.h>
#include <gvc/gvplugin_device.h>
#include <gvc/gvio.h>
#include <limits.h>
#ifdef HAVE_PANGOCAIRO
#include <gdk-pixbuf/gdk-pixbuf.h>

typedef enum {
	FORMAT_BMP,
	FORMAT_ICO,
	FORMAT_JPEG,
	FORMAT_PNG,
	FORMAT_TIFF,
    } format_type;

/* 
 * Does an in-place conversion of a CAIRO ARGB32 image to GDK RGBA
 */
static void
argb2rgba ( unsigned int width, unsigned int height, char *data)
{
/* define indexes to color bytes in each format */
#define Ba 0
#define Ga 1
#define Ra 2
#define Aa 3

#define Rb 0
#define Gb 1
#define Bb 2
#define Ab 3

    unsigned int x, y;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            /* swap red and blue */
            char r = data[Ra];
            data[Bb] = data[Ba];
	    data[Rb] = r;
            data += 4;
        }
    }
}

static gboolean
writer ( const gchar *buf, gsize count, GError **error, gpointer data)
{
  (void)error;
  return count == gvwrite(data, buf, count);
}

static void gdk_format(GVJ_t * job)
{
    char *format_str = "";
    GdkPixbuf *pixbuf;

    switch (job->device.id) {
    case FORMAT_BMP:
	format_str = "bmp";
	break;
    case FORMAT_ICO:
	format_str = "ico";
	break;
    case FORMAT_JPEG:
	format_str = "jpeg";
	break;
    case FORMAT_PNG:
	format_str = "png";
	break;
    case FORMAT_TIFF:
	format_str = "tiff";
	break;
    default:
	UNREACHABLE();
    }

    argb2rgba(job->width, job->height, job->imagedata);

    assert(job->width <= (unsigned)INT_MAX / 4 && "width out of range");
    assert(job->height <= (unsigned)INT_MAX && "height out of range");

    pixbuf = gdk_pixbuf_new_from_data(
                (unsigned char*)(job->imagedata), // data
                GDK_COLORSPACE_RGB,     // colorspace
                TRUE,                   // has_alpha
                8,                      // bits_per_sample
                (int)job->width,        // width
                (int)job->height,       // height
                4 * (int)job->width,    // rowstride
                NULL,                   // destroy_fn
                NULL                    // destroy_fn_data
               );

    gdk_pixbuf_save_to_callback(pixbuf, writer, job, format_str, NULL, NULL);

    g_object_unref(pixbuf);
}

static gvdevice_engine_t gdk_engine = {
    NULL,		/* gdk_initialize */
    gdk_format,
    NULL,		/* gdk_finalize */
};

static gvdevice_features_t device_features_gdk = {
    GVDEVICE_BINARY_FORMAT
      | GVDEVICE_DOES_TRUECOLOR,/* flags */
    {0.,0.},                    /* default margin - points */
    {0.,0.},                    /* default page width, height - points */
    {96.,96.},                  /* dpi */
};
#endif

gvplugin_installed_t gvdevice_gdk_types[] = {
#ifdef HAVE_PANGOCAIRO
    {FORMAT_BMP, "bmp:cairo", 6, &gdk_engine, &device_features_gdk},
    {FORMAT_ICO, "ico:cairo", 6, &gdk_engine, &device_features_gdk},
    {FORMAT_JPEG, "jpe:cairo", 6, &gdk_engine, &device_features_gdk},
    {FORMAT_JPEG, "jpeg:cairo", 6, &gdk_engine, &device_features_gdk},
    {FORMAT_JPEG, "jpg:cairo", 6, &gdk_engine, &device_features_gdk},
    {FORMAT_PNG, "png:cairo", 6, &gdk_engine, &device_features_gdk},
    {FORMAT_TIFF, "tif:cairo", 6, &gdk_engine, &device_features_gdk},
    {FORMAT_TIFF, "tiff:cairo", 6, &gdk_engine, &device_features_gdk},
#endif
    {0, NULL, 0, NULL, NULL}
};
