/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <glcomp/glcompfont.h>
#include <glcomp/glcomptextpng.h>
#include <gtk/gtk.h>
#include <png.h>

unsigned char *glCompLoadPng (char *filename, int *imageWidth, int *imageHeight)
{
    cairo_surface_t *surface = cairo_image_surface_create_from_png(filename);
    if (!surface) return 0;
    *imageWidth = cairo_image_surface_get_width(surface);
    *imageHeight = cairo_image_surface_get_height(surface);
    return cairo_image_surface_get_data(surface);
}
