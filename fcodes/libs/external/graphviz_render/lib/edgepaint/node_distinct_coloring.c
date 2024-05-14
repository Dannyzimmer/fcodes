/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/
#include <sparse/general.h>
#include <sparse/SparseMatrix.h>
#include <sparse/QuadTree.h>
#include <edgepaint/node_distinct_coloring.h>
#include <edgepaint/lab.h>
#include <edgepaint/furtherest_point.h>
#include <sparse/color_palette.h>
#include <stdbool.h>
#include <string.h>

static void node_distinct_coloring_internal2(int scheme, QuadTree qt,
                                             bool weightedQ, SparseMatrix A,
                                             int cdim, double accuracy,
                                             int seed, double *colors,
                                             double *color_diff0,
                                             double *color_diff_sum0) {
  /* here we assume the graph is connected. And that the matrix is symmetric */
  int i, j, *ia, *ja, n, k = 0;
  int max_level;
  double center[3];
  double width;
  double *a = NULL;
  double *x;
  double dist_max;
  double color_diff = 0, color_diff_old;
  double color_diff_sum = 0, color_diff_sum_old, *cc;
  int iter = 0;
  static const int iter_max = 100;
  double cspace_size = 0.7;
  double red[3], black[3], min;
  int imin;
  double *wgt = NULL;

  assert(accuracy > 0);
  max_level = MAX(1, -log(accuracy)/log(2.));
  max_level = MIN(30, max_level);

  {
    color_rgb rgb = { .r = 255*0.5, .g = 0, .b = 0 };
    color_lab lab = RGB2LAB(rgb);
    red[0] = lab.l; red[1] = lab.a; red[2] = lab.b;
  }

  n = A->m;
  if (n == 1){
    if (scheme == COLOR_LAB){
      assert(qt);
      QuadTree_get_nearest(qt, black, colors, &imin, &min);
      LAB2RGB_real_01(colors);
      *color_diff0 = 1000; *color_diff_sum0 = 1000;
    } else {
      for (i = 0; i < cdim; i++) colors[i] = 0;
      *color_diff0 = sqrt(cdim); *color_diff_sum0 = sqrt(cdim); 
    }
    return;
  } else if (n == 2){
    if (scheme == COLOR_LAB){
      assert(qt);
      QuadTree_get_nearest(qt, black, colors, &imin, &min);
      LAB2RGB_real_01(colors);

      QuadTree_get_nearest(qt, red, colors+cdim, &imin, &min);
      LAB2RGB_real_01(colors+cdim);
      *color_diff0 = 1000; *color_diff_sum0 = 1000;

    } else {
      for (i = 0; i < cdim; i++) colors[i] = 0;
      for (i = 0; i < cdim; i++) colors[cdim+i] = 0;
      colors[cdim] = 0.5;
      *color_diff0 = sqrt(cdim); *color_diff_sum0 = sqrt(cdim); 
    }
     return;
  }
  assert(n == A->m);
  ia = A->ia;
  ja = A->ja;
  if (A->type == MATRIX_TYPE_REAL && A->a){
    a = (double*) A->a;
  } 

  /* cube [0, cspace_size]^3: only uised if not LAB */
  center[0] = center[1] = center[2] = cspace_size*0.5;
  width = cspace_size*0.5;

  /* randomly assign colors first */
  srand(seed);
  for (i = 0; i < n*cdim; i++) colors[i] = cspace_size*drand();

  x = MALLOC(sizeof(double)*cdim*n);
  if (weightedQ) wgt = MALLOC(sizeof(double)*n);

  color_diff = 0; color_diff_old = -1;
  color_diff_sum = 0; color_diff_sum_old = -1;

  while (iter++ < iter_max && (color_diff > color_diff_old || (color_diff == color_diff_old && color_diff_sum > color_diff_sum_old))){
    color_diff_old = color_diff;
    color_diff_sum_old = color_diff_sum;
    for (i = 0; i < n; i++){
      k = 0;
      for (j = ia[i]; j < ia[i+1]; j++){
	if (ja[j] == i) continue;
	memcpy(&(x[k*cdim]), &(colors[ja[j]*cdim]), sizeof(double)*cdim);
	if (wgt && a) wgt[k] = a[j];
	k++;
      }
      cc = &(colors[i*cdim]);
      if (scheme == COLOR_LAB){
	furtherest_point_in_list(k, cdim, wgt, x, qt, max_level, &dist_max, &cc);
      } else if (scheme == COLOR_RGB || scheme == COLOR_GRAY){
	furtherest_point(k, cdim, wgt, x, center, width, max_level, &dist_max, &cc);
      } else {
	assert(0);
      }
      if (i == 0){
	color_diff = dist_max;
	color_diff_sum = dist_max;
      } else {
	color_diff = MIN(dist_max, color_diff);
	color_diff_sum += dist_max;
      }
    }

    if (Verbose) fprintf(stderr,"iter ---- %d ---, color_diff = %f, color_diff_sum = %f\n", iter, color_diff, color_diff_sum);
  }

  if (scheme == COLOR_LAB){
    /* convert from LAB to RGB */
    color_rgb rgb;
    color_lab lab;
    for (i = 0; i < n; i++){
      lab = color_lab_init(colors[i*cdim], colors[i*cdim+1], colors[i*cdim+2]);
      rgb = LAB2RGB(lab);
      colors[i*cdim] = (rgb.r)/255;
      colors[i*cdim+1] = (rgb.g)/255;
      colors[i*cdim+2] = (rgb.b)/255;
    }
  }
  *color_diff0 = color_diff;
  *color_diff_sum0 = color_diff_sum;
  free(x);
}
 
static void node_distinct_coloring_internal(int scheme, QuadTree qt,
                                            bool weightedQ, SparseMatrix A,
                                            int cdim, double accuracy,
                                            int seed, double *colors) {
  int i;
  double color_diff;
  double color_diff_sum;
  if (seed < 0) {
    /* do multiple iterations and pick the best */
    int iter, seed_max = -1;
    double color_diff_max = -1;
    srand(123);
    iter = -seed;
    for (i = 0; i < iter; i++){
      seed = irand(100000);
      node_distinct_coloring_internal2(scheme, qt, weightedQ, A, cdim, accuracy, seed, colors, &color_diff, &color_diff_sum);
      if (color_diff_max < color_diff){
	seed_max = seed; color_diff_max = color_diff;
      }
    }
    seed = seed_max;
  } 
  node_distinct_coloring_internal2(scheme, qt, weightedQ, A, cdim, accuracy, seed, colors, &color_diff, &color_diff_sum);
 
}

int node_distinct_coloring(char *color_scheme, char *lightness, bool weightedQ,
                           SparseMatrix A0, double accuracy, int seed,
                           int *cdim0, double **colors) {
  /* 
     for a graph A, get a distinctive color of its nodes so that the color distance among all neighboring nodes are maximized. Here
     color distance on a node is defined as the minimum of color differences between a node and its neighbors (or the minimum of weighted color differences if weightedQ = true,
     where weights are stored as entries of A0.
     accuracy is the threshold given so that when finding the coloring for each node, the optimal is
     with in "accuracy" of the true global optimal. 
     color_scheme: rgb, gray, lab, or one of the color palettes in color_palettes.h, or a list of hex rgb colors separaterd by comma like "#ff0000,#00ff00"
     lightness: of the form 0,70, specifying the range of lightness of LAB color. Ignored if scheme is not COLOR_LAB.
     .          if NULL, 0,70 is assumed
     A: the graph of n nodes
     accuracy: how accurate to find the optimal
     seed: random_seed. If negative, consider -seed as the number of random start iterations
     cdim: dimension of the color space
     color: On input an array of size n*cdim, if NULL, will be allocated. On exit the final color assignment for node i is [cdim*i,cdim*(i+1)), in RGB (between 0 to 1)
  */ 
  SparseMatrix B, A = A0;
  int ncomps, *comps = NULL, *comps_ptr = NULL;
  int nn, n;
  double *ctmp;
  int i, j, jj;
  QuadTree qt = NULL;
  int cdim;
  int scheme = COLOR_LAB;
  int maxcolors = 10000, max_qtree_level = 10, r, g, b;
  char *color_list = NULL;

  color_list = color_palettes_get(color_scheme);
  if (color_list) color_scheme = color_list;

  cdim = *cdim0 = 3;
  if (strcmp(color_scheme, "lab") == 0){
    if (Verbose) fprintf(stderr,"lab\n");
    scheme =  COLOR_LAB;
    qt = lab_gamut_quadtree(lightness, max_qtree_level);
    if (!qt){
      fprintf(stderr, "out of memory\n");
      return -1;
    }
  } else if (strcmp(color_scheme, "rgb") == 0){
    if (Verbose) fprintf(stderr,"rgb\n");
    scheme = COLOR_RGB;
  } else if (strcmp(color_scheme, "gray") == 0){
    scheme = COLOR_GRAY;
    cdim = *cdim0 = 1;
  } else if (sscanf(color_scheme,"#%02X%02X%02X", &r, &g, &b) == 3 ){
    scheme = COLOR_LAB;
    double *color_points = color_blend_rgb2lab(color_scheme, maxcolors);
    assert(color_points);
    qt = QuadTree_new_from_point_list(cdim, maxcolors, max_qtree_level,
                                      color_points);
    free(color_points);
    assert(qt);
  } else {
    return ERROR_BAD_COLOR_SCHEME;
  }


  if (accuracy <= 0) accuracy = 0.0001;

  n = A->m;
  if (n != A->n) {
    return -1;
  }
 
  if (!(*colors)) {
    *colors = MALLOC(sizeof(double)*cdim*n);
  }
  ctmp = MALLOC(sizeof(double)*cdim*n);

  B = SparseMatrix_symmetrize(A, false);
  A = B;

  SparseMatrix_weakly_connected_components(A, &ncomps, &comps, &comps_ptr); 
  
  for (i = 0; i < ncomps; i++){
    nn = comps_ptr[i+1] - comps_ptr[i];
    B = SparseMatrix_get_submatrix(A, nn, nn, &(comps[comps_ptr[i]]), &(comps[comps_ptr[i]]));
    node_distinct_coloring_internal(scheme, qt, weightedQ, B, cdim, accuracy, seed, ctmp);

    for (j = comps_ptr[i]; j < comps_ptr[i+1]; j++){
      jj = j - comps_ptr[i];
      memcpy(&((*colors)[comps[j]*cdim]), &(ctmp[jj*cdim]), cdim*sizeof(double));
    }
    SparseMatrix_delete(B);
  }
  free(ctmp);

  if (A != A0) SparseMatrix_delete(A);
  return 0;
}
