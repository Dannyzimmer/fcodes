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

#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <gvc/gvplugin_loadimage.h>

#ifdef HAVE_PANGOCAIRO
#ifdef HAVE_RSVG
#include <librsvg/rsvg.h>
#ifndef RSVG_CAIRO_H
#include <librsvg/rsvg-cairo.h>
#endif
#include <cairo-svg.h>

typedef enum {
    FORMAT_SVG_CAIRO,
} format_type;


static void gvloadimage_rsvg_free(usershape_t *us)
{
    rsvg_handle_close(us->data, NULL);
}

static RsvgHandle* gvloadimage_rsvg_load(GVJ_t * job, usershape_t *us)
{
    RsvgHandle* rsvgh = NULL;
    guchar *fileBuf = NULL;
    GError *err = NULL;
    size_t fileSize;

    int fd;
    struct stat stbuf;

    assert(job);
    assert(us);
    assert(us->name);

    if (us->data) {
        if (us->datafree == gvloadimage_rsvg_free)
             rsvgh = us->data; /* use cached data */
        else {
             us->datafree(us);        /* free incompatible cache data */
             us->data = NULL;
        }

    }

    if (!rsvgh) { /* read file into cache */
	if (!gvusershape_file_access(us))
	    return NULL;
        switch (us->type) {
            case FT_SVG:
      		rsvgh = rsvg_handle_new();
		
		if (rsvgh == NULL) {
			fprintf(stderr, "rsvg_handle_new_from_file returned an error: %s\n", err->message);
			return NULL;
		} 

	 	fd = fileno(us->f);
		fstat(fd, &stbuf);
  		fileSize = (size_t)stbuf.st_size;	

		fileBuf = calloc(fileSize + 1, sizeof(guchar));

		if (fileBuf == NULL) {
			g_object_unref(rsvgh);
			return NULL;
		}
	
		rewind(us->f);

		if (fread(fileBuf, 1, fileSize, us->f) < fileSize) {
			free(fileBuf);
			g_object_unref(rsvgh);
			return NULL;
		}

		if (!rsvg_handle_write(rsvgh, fileBuf, (gsize)fileSize, &err)) {
			fprintf(stderr, "rsvg_handle_write returned an error: %s\n", err->message);
			free(fileBuf);
			g_object_unref(rsvgh);
			return NULL;
		} 

		free(fileBuf);

		rsvg_handle_close(rsvgh, &err);
		rsvg_handle_set_dpi(rsvgh, POINTS_PER_INCH);

                break;
            default:
                rsvgh = NULL;
        }

        if (rsvgh) {
            us->data = rsvgh;
            us->datafree = gvloadimage_rsvg_free;
        }

	gvusershape_file_release(us);
    }

    return rsvgh;
}

static void gvloadimage_rsvg_cairo(GVJ_t * job, usershape_t *us, boxf b, bool filled)
{
    (void)filled;

    RsvgHandle* rsvgh = gvloadimage_rsvg_load(job, us);

    cairo_t *cr = job->context; /* target context */
    cairo_surface_t *surface;	 /* source surface */

    if (rsvgh) {
        cairo_save(cr);

       	surface = cairo_svg_surface_create(NULL, us->w, us->h); 

	cairo_surface_reference(surface);

        cairo_set_source_surface(cr, surface, 0, 0);
	cairo_translate(cr, b.LL.x, -b.UR.y);
	cairo_scale(cr, (b.UR.x - b.LL.x) / us->w, (b.UR.y - b.LL.y) / us->h);
	rsvg_handle_render_cairo(rsvgh, cr);

        cairo_paint (cr);
        cairo_restore(cr);
    }
}

static gvloadimage_engine_t engine_cairo = {
    gvloadimage_rsvg_cairo
};
#endif
#endif

gvplugin_installed_t gvloadimage_rsvg_types[] = {
#ifdef HAVE_PANGOCAIRO
#ifdef HAVE_RSVG
    {FORMAT_SVG_CAIRO, "svg:cairo", 1, &engine_cairo, NULL},
#endif
#endif
    {0, NULL, 0, NULL, NULL}
};
