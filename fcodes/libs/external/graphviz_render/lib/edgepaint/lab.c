/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <cgraph/alloc.h>
#include <sparse/general.h>
#include <sparse/QuadTree.h>
#include <edgepaint/lab.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sparse/color_palette.h>
#include <edgepaint/lab_gamut.h>

color_rgb color_rgb_init(double r, double g, double b){
  color_rgb rgb;
  rgb.r = r; rgb.g = g; rgb.b = b;
  return rgb;
}

color_xyz color_xyz_init(double x, double y, double z){
  color_xyz xyz;
  xyz.x = x; xyz.y = y; xyz.z = z;
  return xyz;
}


color_lab color_lab_init(double l, double a, double b){
  color_lab lab;
  lab.l = l; lab.a = a; lab.b = b;
  return lab;
}

double XYZEpsilon = 216./24389.;
double XYZKappa = 24389./27.;

static double PivotXYZ(double n){
  if (n > XYZEpsilon) return pow(n, 1/3.);
  return (XYZKappa*n + 16)/116;
}

static double PivotRgb(double n){
  if (n > 0.04045) return 100*pow((n + 0.055)/1.055, 2.4);
  return 100*n/12.92;
}

color_xyz RGB2XYZ(color_rgb color){
  double r = PivotRgb(color.r/255.0);
  double g = PivotRgb(color.g/255.0);
  double b = PivotRgb(color.b/255.0);
  return color_xyz_init(r*0.4124 + g*0.3576 + b*0.1805, r*0.2126 + g*0.7152 + b*0.0722, r*0.0193 + g*0.1192 + b*0.9505);
}

color_lab RGB2LAB(color_rgb color){
  color_xyz white = color_xyz_init(95.047, 100.000, 108.883);
  color_xyz xyz = RGB2XYZ(color);
  double x = PivotXYZ(xyz.x/white.x);
  double y = PivotXYZ(xyz.y/white.y);
  double z = PivotXYZ(xyz.z/white.z);
  double L = MAX(0, 116*y - 16);
  double A = 500*(x - y);
  double B = 200*(y - z);
  return color_lab_init(L, A, B);
}

void LAB2RGB_real_01(double *color){
  /* convert an array[3] of LAB colors to RGB between 0 to 1, in place */
  color_rgb rgb;
  color_lab lab;

  lab.l = color[0];
  lab.a = color[1];
  lab.b = color[2];
  rgb = LAB2RGB(lab);
  color[0] = rgb.r/255;
  color[1] = rgb.g/255;
  color[2] = rgb.b/255;
}
color_rgb LAB2RGB(color_lab color){
  double y = (color.l + 16.0)/116.0;
  double x = color.a/500.0 + y;
  double  z = y - color.b/200.0;
  color_xyz white = color_xyz_init(95.047, 100.000, 108.883), xyz;
  double t1, t2, t3;
  if(pow(x, 3.) > XYZEpsilon){
    t1 = pow(x, 3.);
  } else {
    t1 = (x - 16.0/116.0)/7.787;
  }
  if (color.l > (XYZKappa*XYZEpsilon)){
    t2 = pow(((color.l + 16.0)/116.0), 3.);
  } else {
    t2 = color.l/XYZKappa;
  }
  if (pow(z, 3.) > XYZEpsilon){
    t3 = pow(z, 3.);
  } else {
    t3 = (z - 16.0/116.0)/7.787;
  }
  xyz = color_xyz_init(white.x*t1, white.y*t2, white.z*t3);
  return XYZ2RGB(xyz);
}

color_rgb XYZ2RGB(color_xyz color){
  double x = color.x/100.0;
  double y = color.y/100.0;
  double z = color.z/100.0;
  double r = x*3.2406 + y*(-1.5372) + z*(-0.4986);
  double g = x*(-0.9689) + y*1.8758 + z*0.0415;
  double b = x*0.0557 + y*(-0.2040) + z*1.0570;
  if (r > 0.0031308){
    r = 1.055*pow(r, 1/2.4) - 0.055;
  } else {
    r = 12.92*r;
  }
  if (g > 0.0031308) {
    g = 1.055*pow(g, 1/2.4) - 0.055;
  } else {
    g = 12.92*g;
  }
  if (b > 0.0031308){
    b = 1.055*pow(b, 1/2.4) - 0.055;
  } else {
    b = 12.92*b;
  }
  r = MAX(0, r);
  r = MIN(255, r*255);
  g = MAX(0, g);
  g = MIN(255, g*255);
  b = MAX(0, b);
  b = MIN(255, b*255);

  return color_rgb_init(r, g, b);
}

double *lab_gamut(const char *lightness, int *n){
  /* give a list of n points  in the file defining the LAB color gamut.
     lightness is a string of the form 0,70, or NULL.
   */
  double *xx, *x;

  int l1 = 0, l2 = 70;


  if (lightness && sscanf(lightness, "%d,%d", &l1, &l2) == 2){
    if (l1 < 0) l1 = 0;
    if (l2 > 100) l2 = 100;
    if (l1 > l2) l1 = l2;
  } else {
    l1 = 0; l2 = 70;
  }


  if (Verbose)
    fprintf(stderr,"LAB color lightness range = %d,%d\n", l1, l2);

  if (Verbose)
    fprintf(stderr,"size of lab gamut = %d\n", lab_gamut_data_size);

  // each L* value can be paired with 256 a* values and 256 b* values, so
  // compute the maximum number of doubles we will need to span the space
  size_t m = ((size_t)l2 - (size_t)l1 + 1) * 256 * 256 * 3;

  x = malloc(sizeof(double)*m);
  xx = x;
  *n = 0;
  for (size_t i = 0; i < (size_t)lab_gamut_data_size; i += 4){
    if (lab_gamut_data[i] >= l1 && lab_gamut_data[i] <= l2){
      int b_lower = lab_gamut_data[i + 2];
      int b_upper = lab_gamut_data[i + 3];
      for (int b = b_lower; b <= b_upper; ++b) {
        xx[0] = lab_gamut_data[i];
        xx[1] = lab_gamut_data[i+1];
        xx[2] = b;
        xx += 3;
        (*n)++;
      }
    }
  }

  return x;
}

QuadTree lab_gamut_quadtree(const char *lightness, int max_qtree_level){
  /* read the color gamut points list in the form "x y z\n ..." and store in the octtree */
  int n;
  double *x = lab_gamut(lightness, &n);
  QuadTree qt;
  int dim = 3;

  if (!x) return NULL;
  qt = QuadTree_new_from_point_list(dim, n, max_qtree_level, x);


  free(x);
  return qt;
}

static double lab_dist(color_lab x, color_lab y){
  return sqrt((x.l-y.l)*(x.l-y.l) +(x.a-y.a)*(x.a-y.a) +(x.b-y.b)*(x.b-y.b));
}

static void lab_interpolate(color_lab lab1, color_lab lab2, double t, double *colors){
  colors[0] = lab1.l + t*(lab2.l - lab1.l);
  colors[1] = lab1.a + t*(lab2.a - lab1.a);
  colors[2] = lab1.b + t*(lab2.b - lab1.b);
}

double *color_blend_rgb2lab(char *color_list, const int maxpoints) {
  /* give a color list of the form "#ff0000,#00ff00,...", get a list of around maxpoints
     colors in an array colors0 of size [maxpoints*3] of the form {{l,a,b},...}.
     color_list: either "#ff0000,#00ff00,...", or "pastel"
  */

  int nc = 1, r, g, b, i, ii, jj, cdim = 3;
  char *cl;
  color_rgb rgb;
  double step, dist_current;
  char *cp;

  cp = color_palettes_get(color_list);
  if (cp){
    color_list = cp;
  }

  if (maxpoints <= 0) return NULL;

  cl = color_list;
  while ((cl=strchr(cl, ',')) != NULL){
    cl++; nc++;
  }
  color_lab *lab = gv_calloc(MAX(nc, 1), sizeof(color_lab));

  cl = color_list - 1;
  nc = 0;
  do {
    cl++;
    if (sscanf(cl,"#%02X%02X%02X", &r, &g, &b) != 3) break;
    rgb.r = r; rgb.g = g; rgb.b = b;
    lab[nc++] = RGB2LAB(rgb);
  } while ((cl=strchr(cl, ',')) != NULL);

  double *dists = gv_calloc(MAX(1, nc), sizeof(double));
  dists[0] = 0;
  for (i = 0; i < nc - 1; i++){
    dists[i+1] = lab_dist(lab[i], lab[i+1]);
  }
  /* dists[i] is now the summed color distance from  the 0-th color to the i-th color */
  for (i = 0; i < nc - 1; i++){
    dists[i+1] += dists[i];
  }
  if (Verbose)
    fprintf(stderr,"sum = %f\n", dists[nc-1]);

  double *colors = gv_calloc(maxpoints * cdim, sizeof(double));
  if (maxpoints == 1){
    colors[0] = lab[0].l;
    colors[1] = lab[0].a;
    colors[2] = lab[0].b;
  } else {
    step = dists[nc-1]/(maxpoints - 1);
    ii = 0; jj = 0; dist_current = 0;
    while (dists[jj] < dists[ii] + step) jj++;
    
    for (i = 0; i < maxpoints; i++){
      lab_interpolate(lab[ii], lab[jj], (dist_current - dists[ii])/MAX(0.001, (dists[jj] - dists[ii])), colors);
      dist_current += step;
      colors += cdim;
      if (dist_current > dists[jj]) ii = jj;
      while (jj < nc -1 && dists[jj] < dists[ii] + step) jj++;
    }
  }
  free(dists);
  free(lab);
  return colors;
}
