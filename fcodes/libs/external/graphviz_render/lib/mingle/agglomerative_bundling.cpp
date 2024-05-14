/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <common/types.h>
#include <common/globals.h>
#include <sparse/general.h>
#include <math.h>
#include <time.h>
#include <sparse/SparseMatrix.h>
#include <mingle/edge_bundling.h>
#include <mingle/ink.h>
#include <mingle/agglomerative_bundling.h>
#include <mingle/nearest_neighbor_graph.h>
#include <string.h>
#include <vector>

enum {MINGLE_DEBUG=0};

namespace {
struct Agglomerative_Ink_Bundling_struct {
  int level; /* 0, 1, ... */
  int n;
  SparseMatrix A;  /* n x n matrix, where n is the number of edges/bundles in
                      this level */
  SparseMatrix P;  /* prolongation matrix from level + 1 to level */
  SparseMatrix R0; /* this is basically R[level - 1].R[level - 2]...R[0], which
                      gives the map of bundling i to the original edges: first
                      row of R0 gives the nodes on the finest grid corresponding
                      to the coarsest node 1, etc */
  SparseMatrix R;  /* striction mtrix from level to level + 1*/
  Agglomerative_Ink_Bundling_struct *next;
  std::vector<double>
      inks; /* amount of ink needed to draw this edge/bundle. Dimension n. */
  double total_ink; /* amount of ink needed to draw this edge/bundle. Dimension
                       n. */
  pedge
      *edges; /* the original edge info. This does not vary level to level and
                 is of dimenion n0, where n0 is the number of original edges */
  bool delete_top_level_A; /*whether the top level matrix should be deleted on
                              garbage collecting the grid */
};
} // namespace

using Agglomerative_Ink_Bundling = Agglomerative_Ink_Bundling_struct *;

static Agglomerative_Ink_Bundling Agglomerative_Ink_Bundling_init(SparseMatrix A, pedge *edges, int level){
  Agglomerative_Ink_Bundling grid;
  int n = A->n, i;

  assert(SparseMatrix_is_symmetric(A, true));

  if (!A) return NULL;
  assert(A->m == n);
  grid = (Agglomerative_Ink_Bundling)MALLOC(sizeof(struct Agglomerative_Ink_Bundling_struct));
  grid->level = level;
  grid->n = n;
  grid->A = A;
  grid->P = NULL;
  grid->R0 = NULL;
  grid->R = NULL;
  grid->next = NULL;
  grid->edges = edges;
  grid->delete_top_level_A = false;
  grid->total_ink = -1;
  if (level == 0){
    double total_ink = 0;
    for (i = 0; i < n; i++) {
      grid->inks.push_back(ink1(edges[i]));
      total_ink += grid->inks[i];
    }
    grid->total_ink = total_ink;
  }
  return grid;
} 

static void Agglomerative_Ink_Bundling_delete(Agglomerative_Ink_Bundling grid){
  if (!grid) return;
  if (grid->A){
    if (grid->level == 0) {
      if (grid->delete_top_level_A) SparseMatrix_delete(grid->A);
    } else {
      SparseMatrix_delete(grid->A);
    }
  }
  SparseMatrix_delete(grid->P);
  /* on level 0, R0 = NULL, on level 1, R0 = R */
  if (grid->level > 1) SparseMatrix_delete(grid->R0);
  SparseMatrix_delete(grid->R);

  Agglomerative_Ink_Bundling_delete(grid->next);
  free(grid);
}

static void Agglomerative_Ink_Bundling_establish(Agglomerative_Ink_Bundling grid, int *pick, double angle_param, double angle){
  /* pick is a work array of dimension n, with n the total number of original edges */
  SparseMatrix A = grid->A;
  int n = grid->n, level = grid->level, nc = 0;
  int *ia = A->ia, *ja = A->ja;
  int i, j, k, jj, jc, jmax, ni, nj, npicks;
  pedge *edges = grid->edges;
  const std::vector<double> &inks = grid->inks;
  double inki, inkj;
  double gain, maxgain, minink, total_gain = 0;
  int *ip = NULL, *jp = NULL, ie;
  std::vector<std::vector<int>> cedges;/* a table listing the content of bundled edges in the coarsen grid.
		    cedges[i] contain the list of origonal edges that make up the bundle i in the next level */
  double ink0, ink1, grand_total_ink = 0, grand_total_gain = 0;
  point_t meet1, meet2;

  if (Verbose > 1) fprintf(stderr,"level ===================== %d, n = %d\n",grid->level, n);
  cedges.resize(n);
  std::vector<double> cinks(n, 0.0);

  if (grid->level > 0){
    ip = grid->R0->ia;
    jp = grid->R0->ja;
  }

  assert(n == A->n);
  std::vector<int> matching(n, UNMATCHED);

  for (i = 0; i < n; i++){
    if (matching[i] != UNMATCHED) continue;

    /* find the best matching in ink saving */
    maxgain = 0;
    minink = -1;
    jmax = -1;
    for (j = ia[i]; j < ia[i+1]; j++){
      jj = ja[j];
      if (jj == i) continue;

      /* ink saving of merging i and j */
      if ((jc=matching[jj]) == UNMATCHED){
	/* neither i nor jj are matched */
	inki = inks[i]; inkj = inks[jj];
	if (ip && jp){/* not the first level */
	  ni = (ip[i+1] - ip[i]);/* number of edges represented by i */
	  nj = (ip[jj+1] - ip[jj]);/* number of edges represented by jj */
	  memcpy(pick, &(jp[ip[i]]), sizeof(int)*ni);
	  memcpy(pick+ni, &(jp[ip[jj]]), sizeof(int)*nj);
	} else {/* first level */
	  pick[0] = i; pick[1] = jj;
	  ni = nj = 1;
	}
	if (MINGLE_DEBUG) if (Verbose) fprintf(stderr, "ink(%d)=%f, ink(%d)=%f", i, inki, jj, inkj);
      } else {
	/* j is already matched. Its content is on cedges[jc] */
	inki = inks[i]; inkj = cinks[jc];
	if (MINGLE_DEBUG) if (Verbose) fprintf(stderr, "ink(%d)=%f, ink(%d->%d)=%f", i, inki, jj, jc, inkj);
	if (ip) {
	  ni = (ip[i+1] - ip[i]);/* number of edges represented by i */
	  memcpy(pick, &(jp[ip[i]]), sizeof(int)*ni);
	} else {
	  ni = 1; pick[0] = i; 
	}
	nj = cedges[jc].size();
	npicks = ni;
	for (k = 0; k < nj; k++) {
	  pick[npicks++] = cedges[jc][k];
	}
      }

      npicks = ni + nj;
      ink1 = ink(edges, npicks, pick, &ink0, &meet1, &meet2, angle_param, angle);
      if (MINGLE_DEBUG) {
	if (Verbose) {
		fprintf(stderr,", if merging {");
		for (k = 0; k < npicks; k++) fprintf(stderr,"%d,", pick[k]);
		fprintf(stderr,"}, ");
		fprintf(stderr, " ink0=%f, ink1=%f", inki+inkj, ink1);
	}
      }

      gain = inki + inkj - ink1;
      if (MINGLE_DEBUG) if (Verbose) fprintf(stderr, " gain=%f", gain);
      if (gain > maxgain){
	maxgain = gain;
	minink = ink1;
	jmax = jj;
	if (MINGLE_DEBUG) if (Verbose) fprintf(stderr, "maxgain=%f", maxgain);
      } 
      if (MINGLE_DEBUG) if (Verbose) fprintf(stderr, "\n");

      

    }


    /* now merge i and jmax */
    if (maxgain > 0){
      /* a good bundling of i and another edge jmax is found */
      total_gain += maxgain;
      jc = matching[jmax];
      if (jc == UNMATCHED){/* i and j both unmatched. Add j in the table first */
	if (MINGLE_DEBUG) if (Verbose) printf("maxgain=%f, merge %d with best edge: %d to form coarsen edge %d. Ink=%f\n",maxgain, i, jmax, nc, minink);
	matching[i] = matching[jmax] = nc;
	if (ip){
	  for (k = ip[jmax]; k < ip[jmax+1]; k++) {
	    ie = jp[k];
	    cedges[nc].push_back(ie);
	  }
	} else {
	  cedges[nc].push_back(jmax);
	}
	jc = nc;
	nc++;
      } else {/*j is already matched */
	if (MINGLE_DEBUG) if (Verbose) printf("maxgain=%f, merge %d with existing cluster %d\n",maxgain, i, jc);
	matching[i] = jc;
	grand_total_ink -= cinks[jc];/* ink of cluster jc was already added, and will be added again as part of a larger cluster with i, so dicount */
      }
    } else {/*i can not match/bundle successfully */
      if (MINGLE_DEBUG) if (Verbose) fprintf(stderr, "no gain in bundling node %d\n",i);
      assert(maxgain <= 0);
      matching[i] = nc;
      jc = nc;
      minink = inks[i];
      nc++;
    }


    /* add i to the appropriate table */
    if (ip){
      for (k = ip[i]; k < ip[i+1]; k++) {
	ie = jp[k];
	cedges[jc].push_back(ie);
      }
    } else {
	cedges[jc].push_back(i);
    }
    cinks[jc] = minink;
    grand_total_ink += minink;
    grand_total_gain += maxgain;

    if (MINGLE_DEBUG){
      if (Verbose) {
        fprintf(stderr," coarse edge[%d]={",jc);
        for (const int &cedge : cedges[jc]) {
          fprintf(stderr,"%d,", cedge);
        }
        fprintf(stderr,"}, grand_total_gain=%f\n",grand_total_gain);
      }
    }

  }

  if (nc >= 1 && total_gain > 0){
    /* now set up restriction and prolongation operator */
    SparseMatrix P, R, R1, R0, B, cA;
    double one = 1.;
    Agglomerative_Ink_Bundling cgrid;

    R1 = SparseMatrix_new(nc, n, 1, MATRIX_TYPE_REAL, FORMAT_COORD);
    for (i = 0; i < n; i++){
      jj = matching[i];
      SparseMatrix_coordinate_form_add_entry(R1, jj, i, &one);
    }
    R = SparseMatrix_from_coordinate_format(R1);
    SparseMatrix_delete(R1);
    P = SparseMatrix_transpose(R);
    B = SparseMatrix_multiply(R, A);
    if (!B) return;
    cA = SparseMatrix_multiply(B, P); 
    if (!cA) return;
    SparseMatrix_delete(B);
    grid->P = P;
    grid->R = R;

    level++;
    cgrid = Agglomerative_Ink_Bundling_init(cA, edges, level); 

    if (grid->R0){
      R0 = SparseMatrix_multiply(R, grid->R0);
    } else {
      assert(grid->level == 0);
      R0 = R;
    }
    cgrid->R0 = R0;
    cgrid->inks = cinks;
    cgrid->total_ink = grand_total_ink;

    if (Verbose > 1) fprintf(stderr,"level %d->%d, edges %d -> %d, ink %f->%f , gain = %f, or %f\n", grid->level, cgrid->level, grid->n, 
			 cgrid->n, grid->total_ink, grand_total_ink, grid->total_ink - grand_total_ink, grand_total_gain);	 
    assert(fabs(grid->total_ink - cgrid->total_ink - grand_total_gain) <= 0.0001*grid->total_ink);

    Agglomerative_Ink_Bundling_establish(cgrid, pick, angle_param, angle);
    grid->next = cgrid;

  } else {
    if (Verbose > 1) fprintf(stderr,"no more improvement, orig ink = %f, gain = %f, stop and final bundling found\n", grand_total_ink, grand_total_gain);
    /* no more improvement, stop and final bundling found */
  }
}

static Agglomerative_Ink_Bundling Agglomerative_Ink_Bundling_new(SparseMatrix A0, pedge *edges, double angle_param, double angle){
  /* give a link of edges and their nearest neighbor graph, return a multilevel of edge bundling based on ink saving */
  Agglomerative_Ink_Bundling grid;
  SparseMatrix A = A0;

  if (!SparseMatrix_is_symmetric(A, false) || A->type != MATRIX_TYPE_REAL){
    A = SparseMatrix_get_real_adjacency_matrix_symmetrized(A);
  }
  grid = Agglomerative_Ink_Bundling_init(A, edges, 0);

  std::vector<int> pick(A0->m);
  
  Agglomerative_Ink_Bundling_establish(grid, pick.data(), angle_param, angle);

  if (A != A0) grid->delete_top_level_A = true; // be sure to clean up later

  return grid;
}

static pedge* agglomerative_ink_bundling_internal(int dim, SparseMatrix A, pedge* edges, int nneighbors, int *recurse_level, int MAX_RECURSE_LEVEL, double angle_param, double angle, double *current_ink, double *ink00) {

  int i, j, jj, k;
  int *ia, *ja;
  int *pick;
  Agglomerative_Ink_Bundling grid, cgrid;
  SparseMatrix R;
  double ink0, ink1;
  point_t meet1, meet2;
  pedge e;
  double TOL = 0.0001, wgt_all;
  clock_t start;
  
  (*recurse_level)++;
  if (Verbose > 1) fprintf(stderr, "agglomerative_ink_bundling_internal, recurse level ------- %d\n",*recurse_level);

  assert(A->m == A->n);

  start = clock();
  grid = Agglomerative_Ink_Bundling_new(A, edges, angle_param, angle);
  if (Verbose > 1)
    fprintf(stderr, "CPU for agglomerative bundling %f\n", ((double) (clock() - start))/CLOCKS_PER_SEC);
  ink0 = grid->total_ink;

  /* find coarsest */
  cgrid = grid;
  while (cgrid->next){
    cgrid = cgrid->next;
  }
  ink1 = cgrid->total_ink;

  if (*current_ink < 0){
    *current_ink = *ink00 = ink0;
    if (Verbose > 1)
      fprintf(stderr,"initial total ink = %f\n",*current_ink);
  } 
  if (ink1 < ink0){
    *current_ink -= ink0 - ink1;
  }

  if (Verbose > 1) fprintf(stderr,"ink: %f->%f, edges: %d->%d, current ink = %f, percentage gain over original = %f\n", ink0, ink1, grid->n, cgrid->n, *current_ink, (ink0-ink1)/(*ink00));

  /* if no meaningful improvement (0.0001%), out, else rebundle the middle section */
  if ((ink0-ink1)/(*ink00) < 0.000001 || *recurse_level > MAX_RECURSE_LEVEL) {
    /* project bundles up */
    R = cgrid->R0;
    if (R){
      ia = R->ia;
      ja = R->ja;
      for (i = 0; i < R->m; i++){
	pick = &(ja[ia[i]]);
	
	if (MINGLE_DEBUG) if (Verbose) fprintf(stderr,"calling ink2...\n");
	ink1 = ink(edges, ia[i+1]-ia[i], pick, &ink0, &meet1, &meet2, angle_param, angle);
	if (MINGLE_DEBUG) if (Verbose) fprintf(stderr,"finish calling ink2...\n");
	assert(fabs(ink1 - cgrid->inks[i])<=MAX(TOL, TOL*ink1) && ink1 - ink0 <= TOL);
	(void)TOL;
	assert(ink1 < 1000 * ink0); /* assert that points were found */
	wgt_all = 0.;
	if (ia[i+1]-ia[i] > 1){
	  for (j = ia[i]; j < ia[i+1]; j++){
	    /* make this edge 4 points, insert two meeting points at 1 and 2, make 3 the last point */
	    jj = ja[j];
	    edges[jj] = pedge_double(edges[jj]);/* has to call pedge_double twice: from 2 points to 3 points to 5 points. The last point not used, may be
						 improved later */
	    e = edges[jj] = pedge_double(edges[jj]);

	    e->wgts.resize(4);	
	    e->x[1*dim] = meet1.x;
	    e->x[1*dim+1] = meet1.y;
	    e->x[2*dim] = meet2.x;
	    e->x[2*dim+1] = meet2.y;
	    e->x[3*dim] = e->x[4*dim];
	    e->x[3*dim+1] = e->x[4*dim+1];
	    e->npoints = 4;
	    for (k = 0; k < 3; k++) e->wgts[k] = e->wgt;
	    wgt_all += e->wgt;
	    
	  }
	  for (j = ia[i]; j < ia[i+1]; j++){
	    e = edges[ja[j]];
	    e->wgts[1] = wgt_all;
	  }
	}
	
      } 
    }
  } else {
    pedge midedge;/* middle section of edges that will be bundled again */
    int ne, npp, l;
    SparseMatrix A_mid;
    double wgt;

   /* make new edges using meet1 and meet2.
       
       call Agglomerative_Ink_Bundling_new
       
       inherit new edges to old edges
    */

    R = cgrid->R0;
    assert(R && cgrid != grid);/* if ink improved, we should have gone at leat 1 level down! */
    ia = R->ia;
    ja = R->ja;
    ne = R->m;
    std::vector<pedge> mid_edges(ne);
    std::vector<double> xx(4 * ne);
    for (i = 0; i < R->m; i++){
      pick = &(ja[ia[i]]);
      wgt = 0.;
      for (j = ia[i]; j < ia[i+1]; j++) wgt += edges[j]->wgt;
      if (MINGLE_DEBUG) if (Verbose) fprintf(stderr,"calling ink3...\n");
      ink1 = ink(edges, ia[i+1]-ia[i], pick, &ink0, &meet1, &meet2, angle_param, angle);
      if (MINGLE_DEBUG) if (Verbose) fprintf(stderr,"done calling ink3...\n");
      assert(fabs(ink1 - cgrid->inks[i])<=MAX(TOL, TOL*ink1) && ink1 - ink0 <= TOL);
      assert(ink1 < 1000 * ink0); /* assert that points were found */
      xx[i*4 + 0] = meet1.x;
      xx[i*4 + 1] = meet1.y;
      xx[i*4 + 2] = meet2.x;
      xx[i*4 + 3] = meet2.y;
      mid_edges[i] = pedge_wgt_new(2, dim, &xx.data()[i*4], wgt);
    }

    A_mid = nearest_neighbor_graph(ne, MIN(nneighbors, ne), xx.data());

    agglomerative_ink_bundling_internal(dim, A_mid, mid_edges.data(), nneighbors, recurse_level, MAX_RECURSE_LEVEL, angle_param, angle, current_ink, ink00);
    SparseMatrix_delete(A_mid);
    
    /* patching edges with the new mid-section */
    for (i = 0; i < R->m; i++){
      pick = &(ja[ia[i]]);
      midedge = mid_edges[i];
      npp = midedge->npoints + 2;
      for (j = ia[i]; j < ia[i+1]; j++){
	jj = ja[j];
	e = edges[jj] = pedge_wgts_realloc(edges[jj], npp);

	assert(e->npoints == 2);
	for (l = 0; l < dim; l++){/* move the second point to the last */
	  e->x[(npp - 1)*dim+l] = e->x[1*dim+l];
	}

	for (k = 0; k < midedge->npoints; k++){
	  for (l = 0; l < dim; l++){
	    e->x[(k+1)*dim+l] = midedge->x[k*dim+l];
	  }
	  if (k < midedge->npoints - 1){
	    if (!midedge->wgts.empty()) {
	      e->wgts[(k+1)] = midedge->wgts[k];
	    } else {
	      e->wgts[(k+1)] = midedge->wgt;
	    }
	  }
	}
	e->wgts[npp - 2] = e->wgts[0];/* the last interval take from the 1st interval */


	e->npoints = npp;
      }
    }

    for (i = 0; i < ne; i++) pedge_delete(mid_edges[i]);

  }

  Agglomerative_Ink_Bundling_delete(grid);

  return edges;
}


pedge* agglomerative_ink_bundling(int dim, SparseMatrix A, pedge* edges, int nneighbor, int MAX_RECURSE_LEVEL, double angle_param, double angle) {
  int recurse_level = 0;
  double current_ink = -1, ink0;
  pedge *edges2;

  ink_count = 0;
  edges2 = agglomerative_ink_bundling_internal(dim, A, edges, nneighbor, &recurse_level, MAX_RECURSE_LEVEL, angle_param, angle, &current_ink, &ink0);

  
  if (Verbose > 1)
    fprintf(stderr,"initial total ink = %f, final total ink = %f, inksaving = %f percent, total ink_calc = %f, avg ink_calc per edge = %f\n", ink0, current_ink, (ink0-current_ink)/ink0, ink_count,  ink_count/(double) A->m);
  return edges2;
}
