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
#include <cgraph/agxbuf.h>
#include <stdbool.h>
#include <stdlib.h>

#include <gvc/gvplugin_loadimage.h>
#include <gvc/gvio.h>

#ifdef HAVE_PANGOCAIRO
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkcairo.h>

#ifdef _MSC_VER //*dependencies
    #pragma comment( lib, "gvc.lib" )
    #pragma comment( lib, "glib-2.0.lib" )
    #pragma comment( lib, "cairo.lib" )
    #pragma comment( lib, "gobject-2.0.lib" )
    #pragma comment( lib, "graph.lib" )
    #pragma comment( lib, "gdk-pixbuf.lib" )
#endif

typedef enum {
    FORMAT_BMP_CAIRO,
    FORMAT_JPEG_CAIRO,
    FORMAT_PNG_CAIRO,
    FORMAT_ICO_CAIRO,
} format_type;

static void gdk_set_mimedata_from_file (cairo_surface_t *image, const char *mime_type, const char *file)
{
    FILE *fp;
    unsigned char *data = NULL;
    long len;
    const char *id_prefix = "gvloadimage_gdk-";

    fp = fopen (file, "rb");
    if (fp == NULL)
        return;
    fseek (fp, 0, SEEK_END);
    len = ftell(fp);
    fseek (fp, 0, SEEK_SET);
    if (len > 0)
        data = malloc ((size_t)len);
    if (data) {
        if (fread(data, (size_t)len, 1, fp) != 1) {
            free (data);
            data = NULL;
        }
    }
    fclose(fp);

    if (data) {
        cairo_surface_set_mime_data (image, mime_type, data, (unsigned long)len, free, data);
        agxbuf id = {0};
        agxbprint(&id, "%s%s", id_prefix, file);
        char *unique_id = agxbdisown(&id);
        cairo_surface_set_mime_data(image, CAIRO_MIME_TYPE_UNIQUE_ID,
                                    (unsigned char*)unique_id,
                                    strlen(unique_id), free, unique_id);
    }
}

static void gdk_set_mimedata(cairo_surface_t *image, usershape_t *us)
{
    switch (us->type) {
        case FT_PNG:
            gdk_set_mimedata_from_file (image, CAIRO_MIME_TYPE_PNG, us->name);
            break;
        case FT_JPEG:
            gdk_set_mimedata_from_file (image, CAIRO_MIME_TYPE_JPEG, us->name);
            break;
        default:
            break;
    }
}

static void gdk_freeimage(usershape_t *us)
{
    cairo_surface_destroy(us->data);
}

static cairo_surface_t* gdk_loadimage(GVJ_t * job, usershape_t *us)
{
    cairo_t *cr = job->context; /* target context */
    GdkPixbuf *image = NULL;
    cairo_surface_t *cairo_image = NULL;
    cairo_pattern_t *pattern;

    assert(job);
    assert(us);
    assert(us->name);

    if (us->data) {
        if (us->datafree == gdk_freeimage) {
	    cairo_image = cairo_surface_reference(us->data); /* use cached data */
	} else {
	    us->datafree(us);        /* free incompatible cache data */
	    us->datafree = NULL;
	    us->data = NULL;
        }
    }
    if (!cairo_image) { /* read file into cache */
        if (!gvusershape_file_access(us))
            return NULL;
        switch (us->type) {
            case FT_PNG:
            case FT_JPEG:
            case FT_BMP:
            case FT_ICO:
            case FT_TIFF:
                // FIXME - should be using a stream reader
                image = gdk_pixbuf_new_from_file(us->name, NULL);
                break;
            default:
                image = NULL;
        }
        if (image) {
            cairo_save (cr);
            gdk_cairo_set_source_pixbuf (cr, image, 0, 0);
            pattern = cairo_get_source (cr);
            assert(cairo_pattern_get_type (pattern) == CAIRO_PATTERN_TYPE_SURFACE);
            cairo_pattern_get_surface (pattern, &cairo_image);
            cairo_image = cairo_surface_reference (cairo_image);
            cairo_restore (cr);
            gdk_set_mimedata (cairo_image, us);
            us->data = cairo_surface_reference(cairo_image);
            us->datafree = gdk_freeimage;
        }
        gvusershape_file_release(us);
    }
    return cairo_image;
}

static void gdk_loadimage_cairo(GVJ_t * job, usershape_t *us, boxf b, bool filled)
{
    (void)filled;

    cairo_t *cr = job->context; /* target context */
    cairo_surface_t *image;

    image = gdk_loadimage(job, us);
    if (image) {
        cairo_save(cr);
	cairo_translate(cr, b.LL.x, -b.UR.y);
	cairo_scale(cr, (b.UR.x - b.LL.x)/(us->w), (b.UR.y - b.LL.y)/(us->h)); 
        cairo_set_source_surface (cr, image, 0, 0);
        cairo_paint (cr);
        cairo_restore(cr);
        cairo_surface_destroy (image);
    }
}

static gvloadimage_engine_t engine_gdk = {
    gdk_loadimage_cairo
};

#endif

gvplugin_installed_t gvloadimage_gdk_types[] = {
#ifdef HAVE_PANGOCAIRO
    {FORMAT_BMP_CAIRO,  "bmp:cairo", 1, &engine_gdk, NULL},
    {FORMAT_JPEG_CAIRO, "jpe:cairo", 2, &engine_gdk, NULL},
    {FORMAT_JPEG_CAIRO, "jpg:cairo", 2, &engine_gdk, NULL},
    {FORMAT_JPEG_CAIRO, "jpeg:cairo", 2, &engine_gdk, NULL},
    {FORMAT_PNG_CAIRO,  "png:cairo", -1, &engine_gdk, NULL},
    {FORMAT_ICO_CAIRO,  "ico:cairo", 1, &engine_gdk, NULL},
#endif
    {0, NULL, 0, NULL, NULL}
};
