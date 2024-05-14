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

#include <algorithm>
#include <common/types.h>
#include <common/globals.h>
#include <sparse/general.h>
#include <math.h>
#include <sparse/SparseMatrix.h>
#include <mingle/edge_bundling.h>
#include <time.h>
#include <sparse/clustering.h>
#include <mingle/ink.h>
#include <mingle/agglomerative_bundling.h>
#include <string.h>
#include <vector>

#define SMALL 1.e-10

static double norm(int n, double *x){
  double res = 0;
  int i;

  for (i = 0; i < n; i++) res += x[i]*x[i];
  return sqrt(res);

}
static double sqr_dist(int dim, double *x, double *y){
  int i;
  double res = 0;
  for (i = 0; i < dim; i++) res += (x[i] - y[i])*(x[i] - y[i]);
  return res;
}
static double dist(int dim, double *x, double *y){
  return sqrt(sqr_dist(dim,x,y));
}

pedge pedge_new(int np, int dim, double *x){
  pedge e;

  e = (pedge)MALLOC(sizeof(struct pedge_struct));
  e->npoints = np;
  e->dim = dim;
  e->len = np;
  e->x = (double*)MALLOC(dim*e->len*sizeof(double));
  memcpy(e->x, x, dim*e->len*sizeof(double));
  e->edge_length = dist(dim, &x[0*dim], &x[(np-1)*dim]);
  e->wgt = 1.;
  return e;

}
pedge pedge_wgt_new(int np, int dim, double *x, double wgt){
  pedge e;
  int i;

  e = (pedge)MALLOC(sizeof(struct pedge_struct));
  e->npoints = np;
  e->dim = dim;
  e->len = np;
  e->x = (double*)MALLOC(dim*e->len*sizeof(double));
  memcpy(e->x, x, dim*e->len*sizeof(double));
  e->edge_length = dist(dim, &x[0*dim], &x[(np-1)*dim]);
  e->wgt = wgt;
  e->wgts.resize(np - 1);
  for (i = 0; i < np - 1; i++) e->wgts[i] = wgt;
  return e;

}
void pedge_delete(pedge e){
  free(e->x);
  free(e);
}

static double edge_compatibility(pedge e1, pedge e2){
  /* two edges are u1->v1, u2->v2.
     return 1 if two edges are exactly the same, 0 if they are very different.
   */
  double *u1, *v1, *u2, *v2, *u, dist1, dist2, len1, len2;
  int dim = e1->dim;
  bool flipped = false;

  u1 = e1->x;
  v1 = e1->x+e1->npoints*dim-dim;
  u2 = e2->x;
  v2 = e2->x+e2->npoints*dim-dim;
  dist1 = sqr_dist(dim, u1, u2) + sqr_dist(dim, v1, v2);
  dist2 =  sqr_dist(dim, u1, v2) + sqr_dist(dim, v1, u2);
  if (dist1 > dist2){
    u = u2;
    u2 = v2;
    v2 = u;
    dist1 = dist2;
    flipped = true;
  }
  len1 = dist(dim, u1, v1);
  len2 = dist(dim, u2, v2);
  dist1 = std::max(0.1, dist1 / (len1 + len2 + 0.0001 * dist1));
  if (flipped){
    return -1/dist1; 
  } else {
    return 1/dist1; 
  }
}

static double edge_compatibility_full(pedge e1, pedge e2){
  /* two edges are u1->v1, u2->v2.
     return 1 if two edges are exactly the same, 0 if they are very different.
     This is based on Holten and van Wijk's paper
   */
  double *u1, *v1, *u2, *v2, *u, dist1, dist2, len1, len2, len;
  double tmp, ca, cp, cs;
  int dim = e1->dim, i;
  bool flipped = false;

  u1 = e1->x;
  v1 = e1->x+e1->npoints*dim-dim;
  u2 = e2->x;
  v2 = e2->x+e2->npoints*dim-dim;
  dist1 = sqr_dist(dim, u1, u2) + sqr_dist(dim, v1, v2);
  dist2 =  sqr_dist(dim, u1, v2) + sqr_dist(dim, v1, u2);
  if (dist1 > dist2){
    u = u2;
    u2 = v2;
    v2 = u;
    dist1 = dist2;
    flipped = true;
  }
  len1 = std::max(dist(dim, u1, v1), SMALL);
  len2 = std::max(dist(dim, u2, v2), SMALL);
  len = 0.5*(len1+len2);

  /* angle compatibility */
  ca = 0;
  for (i = 0; i < dim; i++) 
    ca += (v1[i]-u1[i])*(v2[i]-u2[i]);
  ca = fabs(ca/(len1*len2));
  assert(ca > -0.001);

  /* scale compatibility */
  cs = 2 / (std::max(len1, len2) / len + len / std::min(len1, len2));
  assert(cs > -0.001 && cs < 1.001);
 
  /* position compatibility */
  cp = 0;
  for (i = 0; i < dim; i++) {
    tmp = .5*(v1[i]+u1[i])-.5*(v2[i]+u2[i]);
    cp += tmp*tmp;
  }
  cp = sqrt(cp);
  cp = len/(len + cp);
  assert(cp > -0.001 && cp < 1.001);

  /* visibility compatibility */

  dist1 = cp*ca*cs;
  if (flipped){
    return -dist1; 
  } else {
    return dist1; 
  }
}

static void fprint_rgb(FILE* fp, int r, int g, int b, int alpha){
  fprintf(fp, "#%02x%02x%02x%02x", r, g, b, alpha);
}

void pedge_export_gv(FILE *fp, int ne, pedge *edges){
  pedge edge;
  double *x, t;
  int i, j, k, kk, dim, sta, sto;
  double maxwgt = 0, len, len_total0;
  int r, g, b;
  double tt1[3]={0.15,0.5,0.85};
  double tt2[4]={0.15,0.4,0.6,0.85};
  double *tt;

  fprintf(fp,"strict graph{\n");
  /* points */
  for (i = 0; i < ne; i++){
    edge = edges[i];
    x = edge->x;
    dim = edge->dim;
    sta = 0; sto = edge->npoints - 1;

    fprintf(fp, "%d [pos=\"", i);
    for (k = 0; k < dim; k++) {
      if (k != 0)  fprintf(fp, ",");
      fprintf(fp, "%f", x[sta*dim+k]);
    }
    fprintf(fp, "\"];\n");

    fprintf(fp, "%d [pos=\"", i + ne);
    for (k = 0; k < dim; k++) {
      if (k != 0)  fprintf(fp, ",");
      fprintf(fp, "%f", x[sto*dim+k]);
    }
    fprintf(fp, "\"];\n");

  }

  /* figure out max number of bundled original edges in a pedge */
  for (i = 0; i < ne; i++){
    edge = edges[i];
    if (!edge->wgts.empty()) {
      for (j = 0; j < edge->npoints - 1; j++){
	maxwgt = std::max(maxwgt, edge->wgts[j]);
      }
    }
  }

  /* spline and colors */
  for (i = 0; i < ne; i++){
    fprintf(fp,"%d -- %d [pos=\"", i, i + ne);
    edge = edges[i];
    x = edge->x;
    dim = edge->dim;
    /* splines */
    for (j = 0; j < edge->npoints; j++){
      if (j != 0) {
	int mm = 3;
	fprintf(fp," ");
	/* there are ninterval+1 points, add 3*ninterval+2 points, get rid of internal ninternal-1 points,
	  make into 3*ninterval+4 points so that gviz spline rendering can work */
	if (j == 1 || j == edge->npoints - 1) {
	  /* every interval gets 3 points inmserted except the first and last one */
	  tt = tt2;
	  mm = 4;
	} else {
	  tt = tt1;
	}
	for (kk = 1; kk <= mm; kk++){
	  t = tt[kk-1];
	  for (k = 0; k < dim; k++) {
	    if (k != 0) fprintf(fp,",");
	    fprintf(fp, "%f", (x[(j-1)*dim+k]*(1-t)+x[j*dim+k]*(t)));
	  }
	  fprintf(fp," ");
	}
      }
      if (j == 0 || j == edge->npoints - 1){
	for (k = 0; k < dim; k++) {
	  if (k != 0) fprintf(fp,",");
	  fprintf(fp, "%f", x[j*dim+k]);
	}
      }
    }
    /* colors based on how much bundling */
    if (!edge->wgts.empty()) {
      fprintf(fp, "\", wgts=\"");
      for (j = 0; j < edge->npoints - 1; j++){
	if (j != 0) fprintf(fp,",");
	fprintf(fp, "%f", edge->wgts[j]);
      }

      len_total0 = 0;
      fprintf(fp, "\", color=\"");
      for (j = 0; j < edge->npoints - 1; j++){
	len = 0;
	for (k = 0; k < dim; k++){
	  len += (edge->x[dim*j+k] - edge->x[dim*(j+1)+k])*(edge->x[dim*j+k] - edge->x[dim*(j+1)+k]);
	}
	len = sqrt(len/k);
	len_total0 += len;
      }
      for (j = 0; j < edge->npoints - 1; j++){
	len = 0;
	for (k = 0; k < dim; k++){
	  len += (edge->x[dim*j+k] - edge->x[dim*(j+1)+k])*(edge->x[dim*j+k] - edge->x[dim*(j+1)+k]);
	}
	len = sqrt(len/k);
	t = edge->wgts[j]/maxwgt;
	/* interpotate between red (t = 1) to blue (t = 0) */
	r = 255*t; g = 0; b = 255*(1-t); b = 255*(1-t);
	if (j != 0) fprintf(fp,":");
	fprint_rgb(fp, r, g, b, 85);
	if (j < edge->npoints - 2) fprintf(fp,";%f",len/len_total0);
      }

    }
    fprintf(fp, "\"];\n");
    
  }
  fprintf(fp,"}\n");
}

#ifdef DEBUG
static void pedge_print(char *comments, pedge e){
  int i, j, dim;
  dim = e->dim;
  fprintf(stderr,"%s", comments);
  for (i = 0; i < e->npoints; i++){
    if (i > 0) fprintf(stderr,",");
    fprintf(stderr,"{");
    for (j = 0; j < dim; j++){
      if (j > 0) fprintf(stderr,",");
      fprintf(stderr,"%f",e->x[dim*i+j]);
    }
    fprintf(stderr,"}");
  }
  fprintf(stderr,"\n");
}
#endif

pedge pedge_wgts_realloc(pedge e, int n){
  /* diff from pedge_alloc: allocate wgts if do not exist and initialize to wgt */
  int i;
  if (n <= e->npoints) return e;
  e->x = (double*)REALLOC(e->x, e->dim*n*sizeof(double));
  if (e->wgts.empty()){
    e->wgts.resize(n - 1);
    for (i = 0; i < e->npoints; i++) e->wgts[i] = e->wgt;
  } else {
    e->wgts.resize(n - 1);
  }
  e->len = n;
  return e;
}


pedge pedge_double(pedge e){
  /* double the number of points (more precisely, add a point between two points in the polyline */
  int npoints = e->npoints, len = e->len, i, dim = e->dim;
  double *x;
  int j, ii, ii2, np;

  assert(npoints >= 2);
  if (npoints*2-1 > len){
    len = 3*npoints;
    e->x = (double*)REALLOC(e->x, dim*len*sizeof(double));
  }
  x = e->x;

  for (i = npoints - 1; i >= 0; i--){
    ii = 2*i;
    for (j = 0; j < dim; j++){
      x[dim*ii + j] = x[dim*i + j];
    }
  }

  for (i = 0; i < npoints - 1; i++){
    ii = 2*i;/* left and right interpolant of a new point */
    ii2 = 2*(i+1);
    for (j = 0; j < dim; j++){
      x[dim*(2*i + 1) + j] = 0.5*(x[dim*ii + j] + x[dim*ii2 + j]);
    }
  }
  e->len = len;
  np = e->npoints = 2*e->npoints - 1;
  e->edge_length = dist(dim, &x[0*dim], &x[(np-1)*dim]);
  
  return e;
}

static void edge_tension_force(std::vector<double> &force, pedge e) {
  double *x = e->x;
  int dim = e->dim;
  int np = e->npoints;
  int i, left, right, j;
  double s;

  /* tention force = ((np-1)*||2x-xleft-xright||)/||e||, so the force is norminal and unitless
  */
  s =  (np - 1) / std::max(SMALL, e->edge_length);
  for (i = 1; i <= np - 2; i++){
    left = i - 1;
    right = i + 1;
    for (j = 0; j < dim; j++) force[i*dim + j] += s*(x[left*dim + j] - x[i*dim + j]);
    for (j = 0; j < dim; j++) force[i*dim + j] += s*(x[right*dim + j] - x[i*dim + j]);
  }
}

static void edge_attraction_force(double similarity, pedge e1, pedge e2,
                                  std::vector<double> &force) {
  /* attrractive force from x2 applied to x1 */
  double *x1 = e1->x, *x2 = e2->x;
  int dim = e1->dim;
  int np = e1->npoints;
  int i, j;
  double dist, s, ss;
  double edge_length = e1->edge_length;


  assert(e1->npoints == e2->npoints);

  /* attractive force = 1/d where d = D/||e1|| is the relative distance, D is the distance between e1 and e2.
   so the force is norminal and unitless
  */
  if (similarity > 0){
    s = edge_length;
    s = similarity*edge_length;
    for (i = 1; i <= np - 2; i++){
      dist = sqr_dist(dim, &x1[i*dim], &x2[i*dim]);
      if (dist < SMALL) dist = SMALL;
      ss = s/(dist+0.1*edge_length*sqrt(dist));
      for (j = 0; j < dim; j++) force[i*dim + j] += ss*(x2[i*dim + j] - x1[i*dim + j]);
    }
  } else {/* clip e2 */
    s = -edge_length;
    s = -similarity*edge_length; 
    for (i = 1; i <= np - 2; i++){
      dist = sqr_dist(dim, &x1[i*dim], &x2[(np - 1 - i)*dim]);
      if (dist < SMALL) dist = SMALL;
      ss = s/(dist+0.1*edge_length*sqrt(dist));
      for (j = 0; j < dim; j++) force[i*dim + j] += ss*(x2[(np - 1 - i)*dim + j] - x1[i*dim + j]);
    }
  }

}

static pedge* force_directed_edge_bundling(SparseMatrix A, pedge* edges, int maxit, double step0, double K) {
  int i, j, ne = A->n, k;
  int *ia = A->ia, *ja = A->ja, iter = 0;
  double *a = (double*) A->a;
  pedge e1, e2;
  int np = edges[0]->npoints, dim = edges[0]->dim;
  double *x;
  double step = step0;
  double fnorm_a, fnorm_t, edge_length, start;
  
  if (Verbose > 1)
    fprintf(stderr, "total interaction pairs = %d out of %d, avg neighbors per edge = %f\n",A->nz, A->m*A->m, A->nz/(double) A->m);

  std::vector<double> force_t(dim * np);
  std::vector<double> force_a(dim * np);
  while (step > 0.001 && iter < maxit){
    start = clock();
    iter++;
    for (i = 0; i < ne; i++){
      for (j = 0; j < dim*np; j++) {
	force_t[j] = 0.;
	force_a[j] = 0.;
      }
      e1 = edges[i];
      x = e1->x;
      edge_tension_force(force_t, e1);
      for (j = ia[i]; j < ia[i+1]; j++){
	e2 = edges[ja[j]];
	edge_attraction_force(a[j], e1, e2, force_a);
      }
      fnorm_t = std::max(SMALL, norm(dim * (np - 2), &force_t.data()[dim]));
      fnorm_a = std::max(SMALL, norm(dim * (np - 2), &force_a.data()[dim]));
      edge_length = e1->edge_length;

      for (j = 1; j <= np - 2; j++){
	for (k = 0; k < dim; k++) {
	  x[j * dim + k] += step * edge_length
	                  * (force_t[j * dim + k] + K * force_a[j * dim+k])
	                  / hypot(fnorm_t, K * fnorm_a);
	}
      }
      
    }
    step = step*0.9;
  if (Verbose > 1)
    fprintf(stderr, "iter ==== %d cpu = %f npoints = %d\n",iter, ((double) (clock() - start))/CLOCKS_PER_SEC, np - 2);
  }

  return edges;
}

static pedge* modularity_ink_bundling(int dim, int ne, SparseMatrix B, pedge* edges, double angle_param, double angle){
  int *assignment = NULL, nclusters;
  double modularity;
  int *clusterp, *clusters;
  SparseMatrix D, C;
  point_t meet1, meet2;
  double ink0, ink1;
  pedge e;
  int i, j, jj;

  SparseMatrix BB;

  /* B may contain negative entries */
  BB = SparseMatrix_copy(B);
  BB = SparseMatrix_apply_fun(BB, fabs);
  modularity_clustering(BB, true, 0, &nclusters, &assignment, &modularity);
  SparseMatrix_delete(BB);

  if (Verbose > 1) fprintf(stderr, "there are %d clusters, modularity = %f\n",nclusters, modularity);
  
  C = SparseMatrix_new(1, 1, 1, MATRIX_TYPE_PATTERN, FORMAT_COORD);
  
  for (i = 0; i < ne; i++){
    jj = assignment[i];
    SparseMatrix_coordinate_form_add_entry(C, jj, i, NULL);
  }
  
  D = SparseMatrix_from_coordinate_format(C);
  SparseMatrix_delete(C);
  clusterp = D->ia;
  clusters = D->ja;
  for (i = 0; i < nclusters; i++){
    ink1 = ink(edges, clusterp[i+1] - clusterp[i], &clusters[clusterp[i]], &ink0, &meet1, &meet2, angle_param, angle);
    if (Verbose > 1)
      fprintf(stderr,"nedges = %d ink0 = %f, ink1 = %f\n",clusterp[i+1] - clusterp[i], ink0, ink1);
    if (ink1 < ink0){
      for (j = clusterp[i]; j < clusterp[i+1]; j++){
	/* make this edge 5 points, insert two meeting points at 1 and 2, make 3 the last point */
	edges[clusters[j]] = pedge_double(edges[clusters[j]]);
	e = edges[clusters[j]] = pedge_double(edges[clusters[j]]);
	e->x[1*dim] = meet1.x;
	e->x[1*dim+1] = meet1.y;
	e->x[2*dim] = meet2.x;
	e->x[2*dim+1] = meet2.y;
	e->x[3*dim] = e->x[4*dim];
	e->x[3*dim+1] = e->x[4*dim+1];
	e->npoints = 4;
      }
    }
  }
  SparseMatrix_delete(D);
  return edges;
}

static SparseMatrix check_compatibility(SparseMatrix A, int ne, pedge *edges, int compatibility_method, double tol){
  /* go through the links and make sure edges are compatible */
  SparseMatrix B, C;
  int *ia, *ja, i, j, jj;
  double start;
  double dist;

  B = SparseMatrix_new(1, 1, 1, MATRIX_TYPE_REAL, FORMAT_COORD);
  ia = A->ia; ja = A->ja;
  start = clock();
  for (i = 0; i < ne; i++){
    for (j = ia[i]; j < ia[i+1]; j++){
      jj = ja[j];
      if (i == jj) continue;
      if (compatibility_method == COMPATIBILITY_DIST){
	dist = edge_compatibility_full(edges[i], edges[jj]);
      } else if (compatibility_method == COMPATIBILITY_FULL){
	dist = edge_compatibility(edges[i], edges[jj]);
      } 

      if (fabs(dist) > tol){
	B = SparseMatrix_coordinate_form_add_entry(B, i, jj, &dist);
	B = SparseMatrix_coordinate_form_add_entry(B, jj, i, &dist);
      }
    }
  }
  C = SparseMatrix_from_coordinate_format(B);
  SparseMatrix_delete(B);
  B = C;
  if (Verbose > 1)
    fprintf(stderr, "edge compatibilitu time = %f\n",((double) (clock() - start))/CLOCKS_PER_SEC);
  return B;
}

pedge* edge_bundling(SparseMatrix A0, int dim, double *x, int maxit_outer, double K, int method, int nneighbor, int compatibility_method,
		     int max_recursion, double angle_param, double angle){
  /* bundle edges. 
     A: edge graph
     x: edge i is at {p,q}, 
     .  where p = x[2*dim*i : 2*dim*i+dim-1]
     .    and q = x[2*dim*i+dim : 2*dim*i+2*dim-1]
     maxit_outer: max outer iteration for force directed bundling. Every outer iter subdivide each edge segment into 2.
     K: norminal edge length in force directed bundling
     method: which method to use.
     nneighbor: number of neighbors to be used in forming nearest neighbor graph. Used only in agglomerative method
     compatibility_method: which method to use to calculate compatibility. Used only in force directed.
     max_recursion: used only in agglomerative method. Specify how many level of recursion to do to bundle bundled edges again

  */
  int ne = A0->m;
  pedge *edges;
  SparseMatrix A = A0, B = NULL;
  int i;
  double tol = 0.001;
  int k;
  double step0 = 0.1, start = 0.0;
  int maxit = 10;

  assert(A->n == ne);
  edges = (pedge*)MALLOC(sizeof(pedge)*ne);

  for (i = 0; i < ne; i++){
    edges[i] = pedge_new(2, dim, &x[dim*2*i]);
  }

  A = SparseMatrix_symmetrize(A0, true);



  if (Verbose) start = clock();
  if (method == METHOD_INK){

    /* go through the links and make sure edges are compatible */
    B = check_compatibility(A, ne, edges, compatibility_method, tol);

    edges = modularity_ink_bundling(dim, ne, B, edges, angle_param, angle);

  } else if (method == METHOD_INK_AGGLOMERATE){
#ifdef HAVE_ANN
    /* plan: merge a node with its neighbors if doing so improve. Form coarsening graph, repeat until no more ink saving */
    edges = agglomerative_ink_bundling(dim, A, edges, nneighbor, max_recursion, angle_param, angle);
#else
    (void)max_recursion;
    (void)nneighbor;
    agerr (AGERR, "Graphviz built without approximate nearest neighbor library ANN; agglomerative inking not available\n");
#endif
  } else if (method == METHOD_FD){/* FD method */
    
    /* go through the links and make sure edges are compatible */
    B = check_compatibility(A, ne, edges, compatibility_method, tol);


    for (k = 0; k < maxit_outer; k++){
      for (i = 0; i < ne; i++){
	edges[i] = pedge_double(edges[i]);
      }
      step0 /= 2;
      edges = force_directed_edge_bundling(B, edges, maxit, step0, K);
    }
    
  } else if (method == METHOD_NONE){
    ;
  } else {
    assert(0);
  }
  if (Verbose)
    fprintf(stderr, "total edge bundling cpu = %f\n",((double) (clock() - start))/CLOCKS_PER_SEC);
  if (B != A) SparseMatrix_delete(B);
  if (A != A0) SparseMatrix_delete(A);

  return edges;
}
