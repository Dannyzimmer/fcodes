/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#define STANDALONE
#include "country_graph_coloring.h"
#include <math.h>
#include "power.h"
#include <stdbool.h>
#include <time.h>

static void get_local_12_norm(int n, int i, const int *ia, const int *ja,
    const int *p, double *norm){
  int j, nz = 0;
  norm[0] = n; norm[1] = 0;
  for (j = ia[i]; j < ia[i+1]; j++){
    if (ja[j] == i) continue;
    norm[0] = MIN(norm[0], abs(p[i] - p[ja[j]]));
    nz++;
    norm[1] += abs(p[i] - p[ja[j]]);
  }
  if (nz > 0) norm[1] /= nz;
}
static void get_12_norm(int n, int *ia, int *ja, int *p, double *norm){
  /* norm[0] := antibandwidth
     norm[1] := (\sum_{{i,j}\in E} |p[i] - p[j]|)/|E|
     norm[2] := (\sum_{i\in V} (Min_{{j,i}\in E} |p[i] - p[j]|)/|V|
  */
  int i, j, nz = 0;
  double tmp;
  norm[0] = n; norm[1] = 0; norm[2] = 0;
  for (i = 0; i < n; i++){
    tmp = n;
    for (j = ia[i]; j < ia[i+1]; j++){
      if (ja[j] == i) continue;
      norm[0] = MIN(norm[0], abs(p[i] - p[ja[j]]));
      norm[1] += abs(p[i] - p[ja[j]]);
      tmp = MIN(tmp, abs(p[i] - p[ja[j]]));
      nz++;
    }
    norm[2] += tmp;
  }
  norm[2] /= n;
  norm[1] /= nz;
}

void improve_antibandwidth_by_swapping(SparseMatrix A, int *p){
  bool improved = true;
  int cnt = 1, n = A->m, i, j, *ia = A->ia, *ja = A->ja;
  double norm1[3], norm2[3], norm11[3], norm22[3];
  double pi, pj;
  clock_t start = clock();
  FILE *fp = NULL;
  
  if (Verbose){
    fprintf(stderr,"saving timing vs antiband data to timing_greedy\n");
    fp = fopen("timing_greedy","w");
  }
  assert(SparseMatrix_is_symmetric(A, true));
  while (improved){
    improved = false;
    for (i = 0; i < n; i++){
      get_local_12_norm(n, i, ia, ja, p, norm1);
      for (j = 0; j < n; j++){
	if (j == i) continue;
	get_local_12_norm(n, j, ia, ja, p, norm2);
	pi = (p)[i]; pj = (p)[j];
	(p)[i] = pj;
	(p)[j] = pi;
	get_local_12_norm(n, i, ia, ja, p, norm11);
	get_local_12_norm(n, j, ia, ja, p, norm22);
	if (fmin(norm11[0], norm22[0]) > fmin(norm1[0], norm2[0])){
	  improved = true;
	  norm1[0] = norm11[0];
	  norm1[1] = norm11[1];
	  continue;
	}
	(p)[i] = pi;
	(p)[j] = pj;
      }
      if (i%100 == 0 && Verbose) {
	get_12_norm(n, ia, ja, p, norm1);
	fprintf(fp, "%f %f %f\n", ((double)(clock() - start)) / CLOCKS_PER_SEC,
	        norm1[0], norm1[2]);
      }
    }
    if (Verbose) {
      get_12_norm(n, ia, ja, p, norm1);
      fprintf(stderr, "[%d] aband = %f, aband_avg = %f\n", cnt++, norm1[0], norm1[2]);
      fprintf(fp,"%f %f %f\n", ((double)(clock() - start)) / CLOCKS_PER_SEC,
              norm1[0], norm1[2]);
    }
  }
  if (fp != NULL) {
    fclose(fp);
  }
}
  
static void country_graph_coloring_internal(int seed, SparseMatrix A, int **p){
  int n = A->m, i, j, jj;
  SparseMatrix L, A2;
  int *ia = A->ia, *ja = A->ja;
  int a = -1;
  double nrow;
  double norm1[3];
  clock_t start, start2;

  start = clock();
  assert(A->m == A->n);
  A2 = SparseMatrix_symmetrize(A, true);
  ia = A2->ia; ja = A2->ja;

  /* Laplacian */
  L = SparseMatrix_new(n, n, 1, MATRIX_TYPE_REAL, FORMAT_COORD);
  for (i = 0; i < n; i++){
    nrow = 0.;
    for (j = ia[i]; j < ia[i+1]; j++){
      jj = ja[j];
      if (jj != i){
	nrow ++;
	L = SparseMatrix_coordinate_form_add_entry(L, i, jj, &a);
      }
    }
    L = SparseMatrix_coordinate_form_add_entry(L, i, i, &nrow);
  }
  L = SparseMatrix_from_coordinate_format(L);

  /* largest eigen vector */
  double *v = power_method(L, L->n, seed);

  vector_ordering(n, v, p);
  if (Verbose)
    fprintf(stderr, "cpu time for spectral ordering (before greedy) = %f\n",
            ((double)(clock() - start)) / CLOCKS_PER_SEC);

  start2 = clock();
  /* swapping */
  improve_antibandwidth_by_swapping(A2, *p);
  if (Verbose) {
    fprintf(stderr, "cpu time for greedy refinement = %f\n",
            ((double)(clock() - start2)) / CLOCKS_PER_SEC);

    fprintf(stderr, "cpu time for spectral + greedy = %f\n",
            ((double)(clock() - start)) / CLOCKS_PER_SEC);

  }
  get_12_norm(n, ia, ja, *p, norm1);

  if (A2 != A) SparseMatrix_delete(A2);
  SparseMatrix_delete(L);
}
void country_graph_coloring(int seed, SparseMatrix A, int **p){
  country_graph_coloring_internal(seed, A, p);
}
