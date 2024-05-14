/*************************************************************************
 * Copyright (c) 2014 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#pragma once

struct rgb_struct {
  double r, g, b;/* 0 to 255 */
};
typedef struct rgb_struct color_rgb;

struct xyz_struct {
  double x, y, z;
};
typedef struct xyz_struct color_xyz;

struct lab_struct {
  signed char l, a, b;/* l: 0 to 100, a,b: -128 tp 128 */
};
typedef struct lab_struct color_lab;

color_xyz RGB2XYZ(color_rgb color);
color_rgb XYZ2RGB(color_xyz color);
color_lab RGB2LAB(color_rgb color);
void LAB2RGB_real_01(double *color);  /* convert an array[3] of LAB colors to RGB between 0 to 1, in place */
color_rgb LAB2RGB(color_lab color);
color_rgb color_rgb_init(double r, double g, double b);
color_xyz color_xyz_init(double x, double y, double z);
color_lab color_lab_init(double l, double a, double b);
QuadTree lab_gamut_quadtree(const char *lightness, int max_qtree_level); /* construct a quadtree of the LAB gamut points */
double *lab_gamut(const char *lightness, int *n);  /* give a list of n points  in the file defining the LAB color gamut */

/** derive around maxpoints from a color list
 *
 * \param color_list List of the form "#ff0000,#00ff00,..."
 * \param maxpoints Maximum number of points to return
 * \return An array of size [maxpoints*3] of the form {{l,a,b},...}
 */
double *color_blend_rgb2lab(char *color_list, const int maxpoints);
