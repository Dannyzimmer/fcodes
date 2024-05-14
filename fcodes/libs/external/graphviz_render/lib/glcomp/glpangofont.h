/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#pragma once

#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include <png.h>

#ifdef __cplusplus
extern "C" {
#endif

//creates a font file with given name and font description
    unsigned char *glCompCreatePangoTexture(char *fontdescription,
					int fontsize, char *txt,
					cairo_surface_t * surface, int *w,
					int *h);

#ifdef __cplusplus
}
#endif
