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

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {MAX_COLOR = 1001};

enum {npalettes = 265};
extern int knownColorScheme(const char*);
extern char *color_palettes[npalettes][2];
  /* return a list of rgb in hex form: "#ff0000,#00ff00,..." */
extern char *color_palettes_get(char *color_palette_name);
extern int color_palettes_Q(const char *color_palette_name);

extern float palette_pastel[1001][3];
extern float palette_blue_to_yellow[1001][3];
extern float palette_grey_to_red[1001][3];
extern float palette_white_to_red[1001][3];
extern float palette_grey[1001][3];
extern float palette_primary[1001][3];
extern float palette_sequential_singlehue_red[1001][3];
extern float palette_sequential_singlehue_red_lighter[1001][3];
extern float palette_adam_blend[1001][3];
extern float palette_adam[11][3];

#ifdef __cplusplus
}
#endif
