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
#include <sparse/SparseMatrix.h>
#include <sparse/general.h>
#include <math.h>
#include <sparse/QuadTree.h>
#include <stdbool.h>
#include <string.h>
#include <cgraph/agxbuf.h>
#include <cgraph/alloc.h>
#include <cgraph/prisize_t.h>
#include <cgraph/cgraph.h>
#include "make_map.h"
#include <sfdpgen/stress_model.h>
#include "country_graph_coloring.h"
#include <sparse/colorutil.h>
#include <neatogen/delaunay.h>

#include <edgepaint/lab.h>
#include <edgepaint/node_distinct_coloring.h>

void map_palette_optimal_coloring(char *color_scheme, SparseMatrix A0,
				  float **rgb_r, float **rgb_g, float **rgb_b){
  /*
    for a graph A, get a distinctive color of its nodes so that the color distanmce among all nodes are maximized. Here
    color distance on a node is defined as the minimum of color differences between a node and its neighbors.
    color_scheme: rgb, gray, lab, or one of the color palettes in color_palettes.h, or a list of hex rgb colors separaterd by comma like "#ff0000,#00ff00"
    A: the graph of n nodes
    cdim: dimension of the color space
    rgb_r, rgb_g, rgb_b: float array of length A->m + 1, which contains color for each country. 1-based
  */
 
  /*color: On input an array of size n*cdim, if NULL, will be allocated. On exit the final color assignment for node i is [cdim*i,cdim*(i+1)), in RGB (between 0 to 1)
  */
  double *colors = NULL;
  int n = A0->m, i, cdim;

  SparseMatrix A;
  bool weightedQ = true;

  {double *dist = NULL;
    A = SparseMatrix_symmetrize(A0, false);
    SparseMatrix_distance_matrix(A, 0, &dist);
    SparseMatrix_delete(A);
    A = SparseMatrix_from_dense(n, n, dist);
    free(dist);
    A = SparseMatrix_remove_diagonal(A);
    SparseMatrix_export(stdout, A);
  }

  // lightness: of the form 0,70, specifying the range of lightness of LAB
  // color. Ignored if scheme is not COLOR_LAB.
  char *lightness = "0,100";

  // accuracy is the threshold given so that when finding the coloring for each
  // node, the optimal is with in "accuracy" of the true global optimal.
  const double accuracy = 0.01;

  // seed: random_seed. If negative, consider -seed as the number of random
  // start iterations
  const int seed = -10;

  node_distinct_coloring(color_scheme, lightness, weightedQ, A, accuracy, seed,
                         &cdim, &colors);

  if (A != A0){
    SparseMatrix_delete(A);
  }
  *rgb_r = gv_calloc(n + 1, sizeof(float));
  *rgb_g = gv_calloc(n + 1, sizeof(float));
  *rgb_b = gv_calloc(n + 1, sizeof(float));

  for (i = 0; i < n; i++){
    (*rgb_r)[i+1] = (float) colors[cdim*i];
    (*rgb_g)[i+1] = (float) colors[cdim*i + 1];
    (*rgb_b)[i+1] = (float) colors[cdim*i + 2];
  }
  free(colors);
}

void map_optimal_coloring(int seed, SparseMatrix A, float *rgb_r,  float *rgb_g, float *rgb_b){
  int *p = NULL;
  float *u = NULL;
  int n = A->m;
  int i;

  country_graph_coloring(seed, A, &p);

  rgb_r++; rgb_b++; rgb_g++;/* seems necessary, but need to better think about cases when clusters are not contiguous */
  vector_float_take(n, rgb_r, n, p, &u);
  for (i = 0; i < n; i++) rgb_r[i] = u[i];
  vector_float_take(n, rgb_g, n, p, &u);
  for (i = 0; i < n; i++) rgb_g[i] = u[i];
  vector_float_take(n, rgb_b, n, p, &u);
  for (i = 0; i < n; i++) rgb_b[i] = u[i];
  free(u);
}

static int get_poly_id(int ip, SparseMatrix point_poly_map){
  return point_poly_map->ja[point_poly_map->ia[ip]];
}
 
void improve_contiguity(int n, int dim, int *grouping, SparseMatrix poly_point_map, double *x, SparseMatrix graph){
 /* 
     grouping: which group each of the vertex belongs to
     poly_point_map: a matrix of dimension npolys x (n + nrandom), poly_point_map[i,j] != 0 if polygon i contains the point j.
     .  If j < n, it is the original point, otherwise it is artificial point (forming the rectangle around a label) or random points.
  */
  int i, j, *ia, *ja, u, v;
  SparseMatrix point_poly_map, D;
  double dist;
  int nbad = 0, flag;
  int maxit = 10;
  double tol = 0.001;

  D = SparseMatrix_get_real_adjacency_matrix_symmetrized(graph);

  assert(graph->m == n);
  ia = D->ia; ja = D->ja;
  double *a = D->a;

  /* point_poly_map: each row i has only 1 entry at column j, which says that point i is in polygon j */
  point_poly_map = SparseMatrix_transpose(poly_point_map);

  for (i = 0; i < n; i++){
    u = i;
    for (j = ia[i]; j < ia[i+1]; j++){
      v = ja[j];
      dist = distance_cropped(x, dim, u, v);
      if (grouping[u] != grouping[v]){
	a[j] = 1.1*dist;
      }	else if (get_poly_id(u, point_poly_map) == get_poly_id(v, point_poly_map)){
	a[j] = dist;
      } else {
	nbad++;
	a[j] = 0.9*dist;
      }

    }
  }

  if (Verbose) fprintf(stderr,"ratio (edges among discontiguous regions vs total edges)=%f\n",((double) nbad)/ia[n]);
  stress_model(dim, D, &x, maxit, tol, &flag);

  assert(!flag);

  SparseMatrix_delete(D);
  SparseMatrix_delete(point_poly_map);
}

struct Triangle {
  int vertices[3];/* 3 points */
  double center[2]; /* center of the triangle */
};

static void normal(double v[], double normal[]){
  if (v[0] == 0){
    normal[0] = 1; normal[1] = 0;
  } else {
    normal[0] = -v[1];
    normal[1] = v[0];
  }
}

static void triangle_center(double x[], double y[], double z[], double c[]){
  /* find the "center" c, which is the intersection of the 3 vectors that are normal to each
     of the edges respectively, and which passes through the center of the edges respectively
     center[{x_, y_, z_}] := Module[
     {xy = 0.5*(x + y), yz = 0.5*(y + z), zx = 0.5*(z + x), nxy, nyz, 
     beta, cen},
     nxy = normal[y - x];
     nyz = normal[y - z];
     beta = (y-x).(xy - yz)/(nyz.(y-x));
     cen = yz + beta*nyz;
     Graphics[{Line[{x, y, z, x}], Red, Point[cen], Line[{cen, xy}], 
     Line[{cen, yz}], Green, Line[{cen, zx}]}]
     
     ]
 */
  double xy[2], yz[2], nxy[2], nyz[2], ymx[2], ymz[2], beta, bot;
  int i;

  for (i = 0; i < 2; i++) ymx[i] = y[i] - x[i];
  for (i = 0; i < 2; i++) ymz[i] = y[i] - z[i];
  for (i = 0; i < 2; i++) xy[i] = 0.5*(x[i] + y[i]);
  for (i = 0; i < 2; i++) yz[i] = 0.5*(y[i] + z[i]);


  normal(ymx, nxy);
  normal(ymz, nyz);
  bot = nyz[0]*(x[0]-y[0])+nyz[1]*(x[1]-y[1]);
  if (bot == 0){/* xy and yz are parallel */
    c[0] = xy[0]; c[1] = xy[1];
    return;
  }
  beta = ((x[0] - y[0])*(xy[0] - yz[0])+(x[1] - y[1])*(xy[1] - yz[1]))/bot;
  c[0] = yz[0] + beta*nyz[0];
  c[1] = yz[1] + beta*nyz[1];
}

static SparseMatrix matrix_add_entry(SparseMatrix A, int i, int j, int val){
  int i1 = i, j1 = j;
  if (i < j) {
    i1 = j; j1 = i;
  }
  A = SparseMatrix_coordinate_form_add_entry(A, j1, i1, &val);
  return SparseMatrix_coordinate_form_add_entry(A, i1, j1, &val);
}

static void plot_dot_edges(FILE *f, SparseMatrix A){
  int i, *ia, *ja, j;

  
  int n = A->m;
  ia = A->ia;
  ja = A->ja;
  for (i = 0; i < n; i++){
    for (j = ia[i]; j < ia[i+1]; j++){
      if (ja[j] == i) continue;
      fprintf(f,"%d -- %d;\n",i,ja[j]);
    }
  }
}

static void plot_dot_labels(FILE *f, int n, int dim, double *x, char **labels, float *fsz){
  int i;

  for (i = 0; i < n; i++){
    if (fsz){
      fprintf(f, "%d [label=\"%s\", pos=\"%lf,%lf\", fontsize=%f];\n",i, labels[i], x[i*dim], x[i*dim+1], fsz[i]); 
    } else {
      fprintf(f, "%d [label=\"%s\", pos=\"%lf,%lf\"];\n",i, labels[i], x[i*dim], x[i*dim+1]); 
    }
  }

}

static void dot_polygon(agxbuf *sbuff, int np, double *xp, double *yp,
                        double line_width, int fill, const char *cstring) {

  if (np > 0){
    if (fill >= 0){
      agxbprint(sbuff, " c %" PRISIZE_T " -%s C %" PRISIZE_T " -%s P %d ",
                strlen(cstring), cstring, strlen(cstring), cstring, np);
    } else {
      if (line_width > 0){
	size_t len_swidth = (size_t)snprintf(NULL, 0, "%f", line_width);
	agxbprint(sbuff, " c %" PRISIZE_T " -%s S %" PRISIZE_T
	          " -setlinewidth(%f) L %d ", strlen(cstring), cstring,
	          len_swidth + 14, line_width, np);
      } else {
	agxbprint(sbuff, " c %" PRISIZE_T " -%s L %d ", strlen(cstring), cstring, np);
      }
    }
    for (int i = 0; i < np; i++){
      agxbprint(sbuff, " %f %f",xp[i], yp[i]);
    }
  }
}

static void dot_one_poly(agxbuf *sbuff, double line_width, int fill, int np,
                         double *xp, double *yp, const char *cstring) {
  dot_polygon(sbuff, np, xp, yp, line_width, fill, cstring);
}

static void plot_dot_polygons(agxbuf *sbuff, double line_width,
                              const char *line_color, SparseMatrix polys,
                              double *x_poly, int *polys_groups, float *r,
                              float *g, float *b, const char *opacity) {
  int i, j, *ia = polys->ia, *ja = polys->ja, *a = polys->a, npolys = polys->m, nverts = polys->n, ipoly,first;
  int np = 0;
  int fill = -1;
  int use_line = (line_width >= 0);
  
  agxbuf cstring_buffer = {0};
  agxbput(&cstring_buffer, "#aaaaaaff");
  char *cstring = agxbuse(&cstring_buffer);

  size_t maxlen = 0;
  for (i = 0; i < npolys; i++) {
    int len = ia[i + 1] - ia[i];
    if (len > 0 && (size_t)len > maxlen) {
      maxlen = (size_t)len;
    }
  }

  double *xp = gv_calloc(maxlen, sizeof(double));
  double *yp = gv_calloc(maxlen, sizeof(double));

  if (Verbose) fprintf(stderr,"npolys = %d\n",npolys);
  first = abs(a[0]); ipoly = first + 1;
  for (i = 0; i < npolys; i++){
    np = 0;
    for (j = ia[i]; j < ia[i+1]; j++){
      assert(ja[j] < nverts && ja[j] >= 0);
      (void)nverts;
      if (abs(a[j]) != ipoly){/* the first poly, or a hole */
	ipoly = abs(a[j]);
	if (r && g && b) {
	  rgb2hex(r[polys_groups[i]], g[polys_groups[i]], b[polys_groups[i]],
	          &cstring_buffer, opacity);
	  cstring = agxbuse(&cstring_buffer);
	}
	dot_one_poly(sbuff, line_width, fill, np, xp, yp, cstring);
	np = 0;/* start a new polygon */
      } 
      xp[np] = x_poly[2*ja[j]]; yp[np++] = x_poly[2*ja[j]+1];
    }
    if (use_line) {
      dot_one_poly(sbuff, line_width, fill, np, xp, yp, line_color);
    } else {
      /* why set fill to polys_groups[i]?*/
      dot_one_poly(sbuff, -1, 1, np, xp, yp, cstring);
    }
  }
  agxbfree(&cstring_buffer);
  free(xp);
  free(yp);
}

void plot_dot_map(Agraph_t* gr, int n, int dim, double *x, SparseMatrix polys,
                  SparseMatrix poly_lines, double line_width,
                  const char *line_color, double *x_poly, int *polys_groups,
                  char **labels, float *fsz, float *r, float *g, float *b,
                  const char* opacity, SparseMatrix A, FILE* f) {
  /* if graph object exist, we just modify some attributes, otherwise we dump the whole graph */
  bool plot_polyQ = true;
  agxbuf sbuff = {0};

  if (!r || !g || !b) plot_polyQ = false;

  if (!gr) {
    fprintf(f, "graph map {\n node [margin = 0 width=0.0001 height=0.00001 shape=plaintext];\n graph [outputorder=edgesfirst, bgcolor=\"#dae2ff\"]\n edge [color=\"#55555515\",fontname=\"Helvetica-Bold\"]\n");
  } else {
    agattr(gr, AGNODE, "margin", "0"); 
    agattr(gr, AGNODE, "width", "0.0001"); 
    agattr(gr, AGNODE, "height", "0.0001"); 
    agattr(gr, AGNODE, "shape", "plaintext"); 
    agattr(gr, AGNODE, "margin", "0"); 
    agattr(gr, AGNODE, "fontname", "Helvetica-Bold"); 
    agattr(gr, AGRAPH, "outputorder", "edgesfirst");
    agattr(gr, AGRAPH, "bgcolor", "#dae2ff");
    if (!A) agattr(gr, AGEDGE, "style","invis");/* do not plot edges */
  }

  /*polygons */
  if (plot_polyQ) {
    if (!gr) fprintf(f,"_background = \"");
    plot_dot_polygons(&sbuff, -1., NULL, polys, x_poly, polys_groups, r, g, b, opacity);
  }

  /* polylines: line width is set here */
  if (line_width >= 0){
    plot_dot_polygons(&sbuff, line_width, line_color, poly_lines, x_poly, polys_groups, NULL, NULL, NULL, NULL);
  }
  if (!gr) {
    fprintf(f,"%s",agxbuse(&sbuff));
    fprintf(f,"\"\n");/* close polygons/lines */
  } else {
    agattr(gr, AGRAPH, "_background", agxbuse(&sbuff));
    agwrite(gr, f);
  }

  /* nodes */
  if (!gr && labels) plot_dot_labels(f, n, dim, x, labels, fsz);
  /* edges */
  if (!gr && A) plot_dot_edges(f, A);

  /* background color + plot label?*/

  if (!gr) fprintf(f, "}\n");

  agxbfree(&sbuff);
}

static void get_tri(int n, int dim, double *x, int *nt, struct Triangle **T, SparseMatrix *E) {
   /* always contains a self edge and is symmetric.
     input:
     n: number of points
     dim: dimension, only first2D used
     x: point j is stored x[j*dim]--x[j*dim+dim-1]
     output: 
     nt: number of triangles
     T: triangles
     E: a matrix of size nxn, if two points i > j are connected by an triangulation edge, and is neighboring two triangles
     .  t1 and t2, then A[i,j] is both t1 and t2
   */
  int i, j, i0, i1, i2, ntri;
  SparseMatrix A, B;

  int* trilist = get_triangles(x, n, &ntri);

  *T = gv_calloc(ntri, sizeof(struct Triangle));

  A = SparseMatrix_new(n, n, 1, MATRIX_TYPE_INTEGER, FORMAT_COORD);
  for (i = 0; i < ntri; i++) {
    for (j = 0; j < 3; j++) {
      (*T)[i].vertices[j] = trilist[i * 3 + j];
    }
    i0 = (*T)[i].vertices[0]; i1 = (*T)[i].vertices[1]; i2 = (*T)[i].vertices[2];

    triangle_center(&x[i0*dim], &x[i1*dim], &x[i2*dim], (*T)[i].center);
    A = matrix_add_entry(A, i0, i1, i);
    A = matrix_add_entry(A, i1, i2, i);
    A = matrix_add_entry(A, i2, i0, i);
  }

  B = SparseMatrix_from_coordinate_format_not_compacted(A);
  SparseMatrix_delete(A);
  B = SparseMatrix_sort(B);
  *E = B;

  *nt = ntri;

  free(trilist);
}

static SparseMatrix get_country_graph(int n, SparseMatrix A, int *groups, int GRP_RANDOM, int GRP_BBOX){
  /* form a graph each vertex is a group (a country), and a vertex is connected to another if the two countries shares borders.
   since the group ID may not be contiguous (e.g., only groups 2,3,5, -1), we will return NULL if one of the group has non-positive ID! */
  int *ia, *ja;
  int one = 1, jj, i, j, ig1, ig2;
  SparseMatrix B, BB;
  int min_grp, max_grp;
  
  min_grp = max_grp = groups[0];
  for (i = 0; i < n; i++) {
    max_grp = MAX(groups[i], max_grp);
    min_grp = MIN(groups[i], min_grp);
  }
  if (min_grp <= 0) return NULL;
  B = SparseMatrix_new(max_grp, max_grp, 1, MATRIX_TYPE_INTEGER, FORMAT_COORD);
  ia = A->ia;
  ja = A->ja;
  for (i = 0; i < n; i++){
    ig1 = groups[i]-1;/* add a diagonal entry */
    SparseMatrix_coordinate_form_add_entry(B, ig1, ig1, &one);
    for (j = ia[i]; j < ia[i+1]; j++){
      jj = ja[j];
      if (i != jj && groups[i] != groups[jj] && groups[jj] != GRP_RANDOM && groups[jj] != GRP_BBOX){
	ig1 = groups[i]-1; ig2 = groups[jj]-1;
	SparseMatrix_coordinate_form_add_entry(B, ig1, ig2, &one);
      }
    }
  }
  BB = SparseMatrix_from_coordinate_format(B);
  SparseMatrix_delete(B);
  return BB;
}

static void conn_comp(int n, SparseMatrix A, int *groups, SparseMatrix *poly_point_map){
  /* form a graph where only vertices that are connected as well as in the same group are connected */
  int *ia, *ja;
  int one = 1, jj, i, j;
  SparseMatrix B, BB;
  int ncomps, *comps = NULL, *comps_ptr = NULL;

  B = SparseMatrix_new(n, n, 1, MATRIX_TYPE_INTEGER, FORMAT_COORD);
  ia = A->ia;
  ja = A->ja;
  for (i = 0; i < n; i++){
    for (j = ia[i]; j < ia[i+1]; j++){
      jj = ja[j];
      if (i != jj && groups[i] == groups[jj]){
	SparseMatrix_coordinate_form_add_entry(B, i, jj, &one);
      }
    }
  }
  BB = SparseMatrix_from_coordinate_format(B);

  SparseMatrix_weakly_connected_components(BB, &ncomps, &comps, &comps_ptr);
  SparseMatrix_delete(B);
  SparseMatrix_delete(BB);
  *poly_point_map = SparseMatrix_new(ncomps, n, n, MATRIX_TYPE_PATTERN, FORMAT_CSR);
  free((*poly_point_map)->ia);
  free((*poly_point_map)->ja);
  (*poly_point_map)->ia = comps_ptr;
  (*poly_point_map)->ja = comps;
  (*poly_point_map)->nz = n;

}

static void get_poly_lines(int nt, SparseMatrix graph, SparseMatrix E, int ncomps, int *comps_ptr, int *comps,
			   int *groups, int *mask, SparseMatrix *poly_lines, int **polys_groups,
			   int GRP_RANDOM, int GRP_BBOX){
  /*============================================================

    polygon outlines 

    ============================================================*/
  int i, *tlist, nz, ipoly, ipoly2, nnt, ii, jj, t1, t2, t, cur, next, nn, j, nlink, sta;
  int *elist, edim = 3;/* a list tell which vertex a particular vertex is linked with during poly construction.
		since the surface is a cycle, each can only link with 2 others, the 3rd position is used to record how many links
	      */
  int *ie = E->ia, *je = E->ja, *e = E->a, n = E->m;
  int *ia = NULL, *ja = NULL;
  SparseMatrix A;
  int *gmask = NULL;


  graph = NULL;/* we disable checking whether a polyline cross an edge for now due to issues with labels */
  if (graph) {
    assert(graph->m == n);
    gmask = gv_calloc(n, sizeof(int));
    for (i = 0; i < n; i++) gmask[i] = -1;
    ia = graph->ia; ja = graph->ja;
    edim = 5;/* we also store info about whether an edge of a polygon corresponds to a real edge or not. */
  }

  for (i = 0; i < nt; i++) mask[i] = -1;
  /* loop over every point in each connected component */
  elist = gv_calloc(nt * edim, sizeof(int));
  tlist = gv_calloc(nt * 2, sizeof(int));
  *poly_lines = SparseMatrix_new(ncomps, nt, 1, MATRIX_TYPE_INTEGER, FORMAT_COORD);
  *polys_groups = gv_calloc(ncomps, sizeof(int));

  for (i = 0; i < nt; i++) elist[i*edim + 2] = 0;
  nz = ie[E->m] - ie[0];

  ipoly = 1;

  for (i = 0; i < ncomps; i++){
    nnt = 0;
    for (j = comps_ptr[i]; j < comps_ptr[i+1]; j++){
      ii = comps[j];

      if (graph){
	for (jj = ia[ii]; jj < ia[ii+1]; jj++) gmask[ja[jj]] = ii;
      }

      (*polys_groups)[i] = groups[ii];/* assign the grouping of each poly */

      /* skip the country formed by random points */
      if (groups[ii] == GRP_RANDOM || groups[ii] == GRP_BBOX) continue;

      /* always skip bounding box */
      if (groups[ii] == GRP_BBOX) continue;

      for (jj = ie[ii]; jj < ie[ii+1]; jj++){
	if (groups[je[jj]] != groups[ii] && jj < nz - 1  && je[jj] == je[jj+1]){/* an triangle edge neighboring 2 triangles and two ends not in the same groups */
	  t1 = e[jj];
	  t2 = e[jj+1];

	  nlink = elist[t1*edim + 2]%2;
	  elist[t1*edim + nlink] = t2;/* t1->t2*/
	  if (graph){
	    if (gmask[je[jj]] == ii){/* a real edge */
	      elist[t1*edim+nlink+3] = POLY_LINE_REAL_EDGE;
	    } else {
	      fprintf(stderr,"not real!!!\n");
	      elist[t1*edim+nlink+3] = POLY_LINE_NOT_REAL_EDGE;
	    }
	  }
	  elist[t1*edim + 2]++;

	  nlink = elist[t2*edim + 2]%2;
	  elist[t2*edim + nlink] = t1;/* t1->t2*/
	  elist[t2*edim + 2]++;
	  if (graph){
	    if (gmask[je[jj]] == ii){/* a real edge */
	      elist[t2*edim+nlink+3] = POLY_LINE_REAL_EDGE;
	    } else {
	      fprintf(stderr,"not real!!!\n");
	      elist[t2*edim+nlink+3] = POLY_LINE_NOT_REAL_EDGE;
	    }
	  }

	  tlist[nnt++] = t1; tlist[nnt++] = t2;
	  jj++;
	}
      }
    }/* done poly edges for this component i */


    /* debugging*/
#ifdef DEBUG
    /*
    for (j = 0; j < nnt; j++) {
      if (elist[tlist[j]*edim + 2]%2 != 0){
	printf("wrong: elist[%d*edim+2] = %d\n",j,elist[tlist[j]*edim + 2]%);
	assert(0);
      }
    }
    */
#endif

    /* form one or more (if there is a hole) polygon outlines for this component  */
    for (j = 0; j < nnt; j++){
      t = tlist[j];
      if (mask[t] != i){
	cur = sta = t; mask[cur] = i;
	next = neighbor(t, 1, edim, elist);
	nlink = 1;
	if (graph && neighbor(cur, 3, edim, elist) == POLY_LINE_NOT_REAL_EDGE){
	  ipoly2 = - ipoly;
	} else {
	  ipoly2 = ipoly;
	}
	SparseMatrix_coordinate_form_add_entry(*poly_lines, i, cur, &ipoly2);
	while (next != sta){
	  mask[next] = i;
	  
	  if (graph && neighbor(cur, nlink + 3, edim, elist) == POLY_LINE_NOT_REAL_EDGE){
	    ipoly2 = -ipoly;
	  } else {
	    ipoly2 = ipoly;
	  }
	  SparseMatrix_coordinate_form_add_entry(*poly_lines, i, next, &ipoly2);

	  nn = neighbor(next, 0, edim, elist);
	  nlink = 0;
	  if (nn == cur) {
	    nlink = 1;
	    nn = neighbor(next, 1, edim, elist);
	  }
	  assert(nn != cur);

	  cur = next;
	  next = nn;
	}

	if (graph && neighbor(cur, nlink + 3, edim, elist) == POLY_LINE_NOT_REAL_EDGE){
	  ipoly2 = -ipoly;
	} else {
	  ipoly2 = ipoly;
	}
	SparseMatrix_coordinate_form_add_entry(*poly_lines, i, sta, &ipoly2);/* complete a cycle by adding starting point */

	ipoly++;
      }

    }/* found poly_lines for this comp */
  }

  A = SparseMatrix_from_coordinate_format_not_compacted(*poly_lines);
  SparseMatrix_delete(*poly_lines);
  *poly_lines = A;

  free(tlist);
  free(elist);
}

static void cycle_print(int head, int *cycle, int *edge_table){
  int cur, next;

  cur = head;
  fprintf(stderr, "cycle (edges): {");
  while ((next = cycle_next(cur)) != head){
    fprintf(stderr, "%d,",cur);
    cur = next;
  }
  fprintf(stderr, "%d}\n",cur);

  cur = head;
  fprintf(stderr, "cycle (vertices): ");
  while ((next = cycle_next(cur)) != head){
    fprintf(stderr, "%d--",edge_head(cur));
    cur = next;
  }
  fprintf(stderr, "%d--%d\n",edge_head(cur),edge_tail(cur));
}

static int same_edge(int ecur, int elast, int *edge_table){
  return (edge_head(ecur) == edge_head(elast) && edge_tail(ecur) == edge_tail(elast))
	  || (edge_head(ecur) == edge_tail(elast) && edge_tail(ecur) == edge_head(elast));
}

static void get_polygon_solids(int nt, SparseMatrix E, int ncomps, int *comps_ptr, int *comps,
			       int *mask, SparseMatrix *polys){
  /*============================================================

    polygon slids that will be colored

    ============================================================*/
  int *edge_table;/* a table of edges of the triangle graph. If two vertex u and v are connected and are adjacent to two triangles
		     t1 and t2, then from u there are two edges to v, one denoted as t1->t2, and the other t2->t1. They are
		     numbered as e1 and e2. edge_table[e1]={t1,t2} and edge_table[e2]={t2,t1}
		  */
  SparseMatrix half_edges;/* a graph of triangle edges. If two vertex u and v are connected and are adjacent to two triangles
		     t1 and t2, then from u there are two edges to v, one denoted as t1->t2, and the other t2->t1. They are
		     numbered as e1 and e2. Likewise from v to u there are also two edges e1 and e2.
		  */

  int n = E->m, *ie = E->ia, *je = E->ja, *e = E->a, ne, i, j, t1, t2, jj, ii;
  int *cycle, cycle_head = 0;/* a list of edges that form a cycle that describe the polygon. cycle[e][0] gives the prev edge in the cycle from e,
	       cycle[e][1] gives the next edge
	     */
  int *edge_cycle_map, NOT_ON_CYCLE = -1;/* map an edge e to its position on cycle, unless it does not exist (NOT_ON_CYCLE) */
  int *emask;/* whether an edge is seens this iter */
  enum {NO_DUPLICATE = -1};
  int *elist, edim = 3;/* a list tell which edge a particular vertex is linked with when a voro cell has been visited,
		since the surface is a cycle, each vertex can only link with 2 edges, the 3rd position is used to record how many links
	      */

  int k, duplicate, ee = 0, ecur, enext, eprev, cur, next, nn, nlink, head, elast = 0, etail, tail, ehead, efirst; 

  int DEBUG_CYCLE = 0;
  SparseMatrix B;

  ne = E->nz;
  edge_table = gv_calloc(ne * 2, sizeof(int));

  for (i = 0; i < n; i++) mask[i] = -1;

  half_edges = SparseMatrix_new(n, n, 1, MATRIX_TYPE_INTEGER, FORMAT_COORD);

  ne = 0;
  for (i = 0; i < n; i++){
    for (j = ie[i]; j < ie[i+1]; j++){
      if (j < ie[n] - ie[0] - 1 && i > je[j] && je[j] == je[j+1]){/* an triangle edge neighboring 2 triangles. Since E is symmetric, we only do one edge of E*/
	t1 = e[j];
	t2 = e[j+1];
	jj = je[j];	  
	assert(jj < n);
	edge_table[ne*2] = t1;/*t1->t2*/
	edge_table[ne*2+1] = t2;
	half_edges = SparseMatrix_coordinate_form_add_entry(half_edges, i, jj, &ne);
	half_edges = SparseMatrix_coordinate_form_add_entry(half_edges, jj, i, &ne);
	ne++;

	edge_table[ne*2] = t2;/*t2->t1*/
	edge_table[ne*2+1] = t1;
	half_edges = SparseMatrix_coordinate_form_add_entry(half_edges, i, jj, &ne);
	half_edges = SparseMatrix_coordinate_form_add_entry(half_edges, jj, i, &ne);	


	ne++;
	j++;
      }
    }
  }
  assert(E->nz >= ne);

  cycle = gv_calloc(ne * 2, sizeof(int));
  B = SparseMatrix_from_coordinate_format_not_compacted(half_edges);
  SparseMatrix_delete(half_edges);half_edges = B;

  edge_cycle_map = gv_calloc(ne, sizeof(int));
  emask = gv_calloc(ne, sizeof(int));
  for (i = 0; i < ne; i++) edge_cycle_map[i] = NOT_ON_CYCLE;
  for (i = 0; i < ne; i++) emask[i] = -1;

  ie = half_edges->ia;
  je = half_edges->ja;
  e = half_edges->a;
  elist = gv_calloc(nt * 3, sizeof(int));
  for (i = 0; i < nt; i++) elist[i*edim + 2] = 0;

  *polys = SparseMatrix_new(ncomps, nt, 1, MATRIX_TYPE_INTEGER, FORMAT_COORD);

  for (i = 0; i < ncomps; i++){
    if (DEBUG_CYCLE) fprintf(stderr, "\n ============  comp %d has %d members\n",i, comps_ptr[i+1]-comps_ptr[i]);
    for (k = comps_ptr[i]; k < comps_ptr[i+1]; k++){
      ii = comps[k];
      mask[ii] = i;
      duplicate = NO_DUPLICATE;
      if (DEBUG_CYCLE) fprintf(stderr,"member = %d has %d neighbors\n",ii, ie[ii+1]-ie[ii]);
      for (j = ie[ii]; j < ie[ii+1]; j++){
	jj = je[j];
	ee = e[j];
	t1 = edge_head(ee);
	if (DEBUG_CYCLE) fprintf(stderr," linked with %d using half-edge %d, {head,tail} of the edge = {%d, %d}\n",jj, ee, t1, edge_tail(ee));
	nlink = elist[t1*edim + 2]%2;
	elist[t1*edim + nlink] = ee;/* t1->t2*/
	elist[t1*edim + 2]++;

	if (edge_cycle_map[ee] != NOT_ON_CYCLE) duplicate = ee;
	emask[ee] = ii;
      }

      if (duplicate == NO_DUPLICATE){
	/* this must be the first time the cycle is being established, a new voro cell*/
	ecur = ee;
	cycle_head = ecur;
	cycle_next(ecur) = ecur;
	cycle_prev(ecur) = ecur;
	edge_cycle_map[ecur] = 1;
	head = cur = edge_head(ecur);
	next = edge_tail(ecur);
	if (DEBUG_CYCLE) fprintf(stderr, "NEW CYCLE\n starting with edge %d, {head,tail}={%d,%d}\n", ee, head, next);
	while (next != head){
	  enext = neighbor(next, 0, edim, elist);/* two voro edges linked with triangle "next" */
	  if ((edge_head(enext) == cur && edge_tail(enext) == next)
	      || (edge_head(enext) == next && edge_tail(enext) == cur)){/* same edge */
	    enext = neighbor(next, 1, edim, elist);
	  };
	  if (DEBUG_CYCLE) fprintf(stderr, "cur edge = %d, next edge %d, {head,tail}={%d,%d},\n",ecur, enext, edge_head(enext), edge_tail(enext));
	  nn = edge_head(enext);
	  if (nn == next) nn = edge_tail(enext);
	  cycle_next(enext) = cycle_next(ecur);
	  cycle_prev(enext) = ecur;
	  cycle_next(ecur) = enext;
	  cycle_prev(ee) = enext;
	  edge_cycle_map[enext] = 1;

	  ecur = enext;
	  cur = next;
	  next = nn;
	} 
	if (DEBUG_CYCLE) cycle_print(ee, cycle,edge_table);
      } else {
	/* we found a duplicate edge, remove that, and all contiguous neighbors that overlap with the current voro
	 */
	ecur = ee = duplicate;
	while (emask[ecur] == ii){
	  /* contiguous overlapping edges, Cycling is not possible
	     since the cycle can not complete surround the new voro cell and yet
	     do not contain any other edges
	  */
	  ecur = cycle_next(ecur);
	}
	if (DEBUG_CYCLE) fprintf(stderr," duplicating edge = %d, starting from the a non-duplicating edge %d, search backwards\n",ee, ecur);

	ecur = cycle_prev(ecur);
	efirst = ecur;
	while (emask[ecur] == ii){
	  if (DEBUG_CYCLE) fprintf(stderr," remove edge %d (%d--%d)\n",ecur, edge_head(ecur), edge_tail(ecur));
	  /* short this duplicating edge */
	  edge_cycle_map[ecur] = NOT_ON_CYCLE;
	  enext = cycle_next(ecur);
	  eprev = cycle_prev(ecur);
	  cycle_next(ecur) = ecur;/* isolate this edge */
	  cycle_prev(ecur) = ecur;
	  cycle_next(eprev) = enext;/* short */
	  cycle_prev(enext) = eprev;
	  elast = ecur;/* record the last removed edge */
	  ecur = eprev;
	}

	if (DEBUG_CYCLE) {
	  fprintf(stderr, "remaining (broken) cycle = ");
	  cycle_print(cycle_next(ecur), cycle,edge_table);
	}

	/* we now have a broken cycle of head = edge_tail(ecur) and tail = edge_head(cycle_next(ecur)) */
	ehead = ecur; etail = cycle_next(ecur);
	cycle_head = ehead;
	head = edge_tail(ehead); 
	tail = edge_head(etail);

	/* pick an edge ev from head in the voro that is a removed edge: since the removed edges form a path starting from
	   efirst, and at elast (head of elast is head), usually we just need to check that ev is not the same as elast,
	   but in the case of a voro filling in a hole, we also need to check that ev is not efirst,
	   since in this case every edge of the voro cell is removed
	 */
	ecur = neighbor(head, 0, edim, elist);
	if (same_edge(ecur, elast, edge_table)){
	  ecur = neighbor(head, 1, edim, elist);
	};

	if (DEBUG_CYCLE) fprintf(stderr, "forwarding now from edge %d = {%d, %d}, try to reach vtx %d, first edge from voro = %d\n",
				 ehead, edge_head(ehead), edge_tail(ehead), tail, ecur);

	/* now go along voro edges till we reach the tail of the broken cycle*/
	cycle_next(ehead) = ecur;
	cycle_prev(ecur) = ehead;
	cycle_prev(etail) = ecur;
	cycle_next(ecur) = etail;
	if (same_edge(ecur, efirst, edge_table)){
	  if (DEBUG_CYCLE) fprintf(stderr, "this voro cell fill in a hole completely!!!!\n");
	} else {
	
	  edge_cycle_map[ecur] = 1;
	  head = cur = edge_head(ecur);
	  next = edge_tail(ecur);
	  if (DEBUG_CYCLE) fprintf(stderr, "starting with edge %d, {head,tail}={%d,%d}\n", ecur, head, next);
	  while (next != tail){
	    enext = neighbor(next, 0, edim, elist);/* two voro edges linked with triangle "next" */
	    if ((edge_head(enext) == cur && edge_tail(enext) == next)
		|| (edge_head(enext) == next && edge_tail(enext) == cur)){/* same edge */
	      enext = neighbor(next, 1, edim, elist);
	    };
	    if (DEBUG_CYCLE) fprintf(stderr, "cur edge = %d, next edge %d, {head,tail}={%d,%d},\n",ecur, enext, edge_head(enext), edge_tail(enext));


	    nn = edge_head(enext);
	    if (nn == next) nn = edge_tail(enext);
	    cycle_next(enext) = cycle_next(ecur);
	    cycle_prev(enext) = ecur;
	    cycle_next(ecur) = enext;
	    cycle_prev(etail) = enext;
	    edge_cycle_map[enext] = 1;
	    
	    ecur = enext;
	    cur = next;
	    next = nn;
	  }
	}

      }
      
    }
    /* done this component, load to sparse matrix, unset edge_map*/
    ecur = cycle_head;
    while ((enext = cycle_next(ecur)) != cycle_head){
      edge_cycle_map[ecur] = NOT_ON_CYCLE;
      head = edge_head(ecur);
      SparseMatrix_coordinate_form_add_entry(*polys, i, head, &i);
      ecur = enext;
    }
    edge_cycle_map[ecur] = NOT_ON_CYCLE;
    head = edge_head(ecur); tail = edge_tail(ecur);
    SparseMatrix_coordinate_form_add_entry(*polys, i, head, &i);
    SparseMatrix_coordinate_form_add_entry(*polys, i, tail, &i);


    /* unset edge_map */
  }

  B = SparseMatrix_from_coordinate_format_not_compacted(*polys);
  SparseMatrix_delete(*polys);
  *polys = B;
  
  SparseMatrix_delete(half_edges);
  free(cycle);
  free(edge_cycle_map);
  free(elist);
  free(emask);
  free(edge_table);
}

static void get_polygons(int n, int nrandom, int dim, SparseMatrix graph, int *grouping,
			 int nt, struct Triangle *Tp, SparseMatrix E, int *nverts, double **x_poly, 
			 SparseMatrix *poly_lines, SparseMatrix *polys, int **polys_groups, SparseMatrix *poly_point_map, SparseMatrix *country_graph){
  int i, j;
  int *mask;
  int *groups;
  int maxgrp;
  int *comps = NULL, *comps_ptr = NULL, ncomps;
  int GRP_RANDOM, GRP_BBOX;
  SparseMatrix B;

  assert(dim == 2);
  *nverts = nt;
 
  groups = gv_calloc(n + nrandom, sizeof(int));
  maxgrp = grouping[0];
  for (i = 0; i < n; i++) {
    maxgrp = MAX(maxgrp, grouping[i]);
    groups[i] = grouping[i];
  }

  GRP_RANDOM = maxgrp + 1; GRP_BBOX = maxgrp + 2;
  for (i = n; i < n + nrandom - 4; i++) {/* all random points in the same group */
    groups[i] = GRP_RANDOM;
  }
  for (i = n + nrandom - 4; i < n + nrandom; i++) {/* last 4 pts of the expanded bonding box in the same group */
    groups[i] = GRP_BBOX;
  }
  
  /* finding connected components: vertices that are connected in the triangle graph, as well as in the same group */
  mask = gv_calloc(MAX(n + nrandom, 2 * nt), sizeof(int));
  conn_comp(n + nrandom, E, groups, poly_point_map);

  ncomps = (*poly_point_map)->m;
  comps = (*poly_point_map)->ja;
  comps_ptr = (*poly_point_map)->ia;

  /* connected components are such that  the random points and the bounding box 4 points forms the last
     remaining components */
  for (i = ncomps - 1; i >= 0; i--) {
    if (groups[comps[comps_ptr[i]]] != GRP_RANDOM &&
        groups[comps[comps_ptr[i]]] != GRP_BBOX) break;
  }
  ncomps = i + 1;
  if (Verbose) fprintf(stderr,"ncomps = %d\n",ncomps);

  *x_poly = gv_calloc(dim * nt, sizeof(double));
  for (i = 0; i < nt; i++){
    for (j = 0; j < dim; j++){
      (*x_poly)[i*dim+j] = Tp[i].center[j];
    }
  }
  
  /*============================================================

    polygon outlines 

    ============================================================*/
  get_poly_lines(nt, graph, E, ncomps, comps_ptr, comps, groups, mask, poly_lines, polys_groups, GRP_RANDOM, GRP_BBOX);

  /*============================================================

    polygon solids

    ============================================================*/
  get_polygon_solids(nt, E, ncomps, comps_ptr, comps, mask, polys);

  B = get_country_graph(n, E, groups, GRP_RANDOM, GRP_BBOX);
  *country_graph = B;

  free(groups);
  free(mask);
}

static void make_map_internal(bool include_OK_points,
		      int n, int dim, double *x0, int *grouping0, SparseMatrix graph, double bounding_box_margin, int nrandom, int nedgep, 
		      double shore_depth_tol, int *nverts, double **x_poly, 
		      SparseMatrix *poly_lines, SparseMatrix *polys, int **polys_groups, SparseMatrix *poly_point_map,
		      SparseMatrix *country_graph, int highlight_cluster){


  double xmax[2], xmin[2], area, *x = x0;
  int i, j;
  QuadTree qt;
  int dim2 = 2, nn = 0;
  int max_qtree_level = 10;
  double ymin[2], min;
  int imin, nzok = 0, nzok0 = 0, nt;
  double *xran, point[2];
  struct Triangle *Tp;
  SparseMatrix E;
  double boxsize[2];
  bool INCLUDE_OK_POINTS = include_OK_points;/* OK points are random points inserted and found to be within shore_depth_tol of real/artificial points,
			      including them instead of throwing away increase realism of boundary */
  int *grouping = grouping0;

  int HIGHLIGHT_SET = highlight_cluster;

  for (j = 0; j < dim2; j++) {
    xmax[j] = x[j];
    xmin[j] = x[j];
  }

  for (i = 0; i < n; i++){
    for (j = 0; j < dim2; j++) {
      xmax[j] = MAX(xmax[j], x[i*dim+j]);
      xmin[j] = MIN(xmin[j], x[i*dim+j]);
    }
  }
  boxsize[0] = xmax[0] - xmin[0];
  boxsize[1] = xmax[1] - xmin[1];
  area = boxsize[0]*boxsize[1];

  if (nrandom == 0) {
    nrandom = n;
  } else if (nrandom < 0){
    nrandom = -nrandom * n;
  } else if (nrandom < 4) {/* by default we add 4 point on 4 corners anyway */
    nrandom = 0;
  } else {
    nrandom -= 4;
  }

  if (shore_depth_tol < 0) shore_depth_tol = sqrt(area/(double) n); /* set to average distance for random distribution */
  if (Verbose) fprintf(stderr, "nrandom=%d shore_depth_tol=%.08f\n", nrandom, shore_depth_tol);


  /* add artificial points along each edge to avoid as much as possible 
     two connected components be separated due to small shore depth */
  {
    int nz;
    double *y;
    int k, t, np=nedgep;
    if (graph && np){
      fprintf(stderr,"add art np = %d\n",np);
      nz = graph->nz;
      y = gv_calloc(dim * n + dim * nz * np, sizeof(double));
      for (i = 0; i < n*dim; i++) y[i] = x[i];
      grouping = gv_calloc(n + nz * np, sizeof(int));
      for (i = 0; i < n; i++) grouping[i] = grouping0[i];
      nz = n;
      for (i = 0; i < graph->m; i++){

	for (j = graph->ia[i]; j < graph->ia[i+1]; j++){
	  if (!HIGHLIGHT_SET || (grouping[i] == grouping[graph->ja[j]] && grouping[i] == HIGHLIGHT_SET)){
	    for (t = 0; t < np; t++){
	      for (k = 0; k < dim; k++){
		y[nz*dim+k] = t/((double) np)*x[i*dim+k] + (1-t/((double) np))*x[(graph->ja[j])*dim + k];
	      }
	      assert(n + (nz-n)*np + t < n + nz*np && n + (nz-n)*np + t >= 0);
	      if (t/((double) np) > 0.5){
		grouping[nz] = grouping[i];
	      } else {
		grouping[nz] = grouping[graph->ja[j]];
	      }
	      nz++;
	    }
	  }
	}
      }
      fprintf(stderr, "after adding edge points, n:%d->%d\n",n, nz);
      n = nz;
      x = y;
      qt = QuadTree_new_from_point_list(dim, nz, max_qtree_level, y);
    } else {
      qt = QuadTree_new_from_point_list(dim, n, max_qtree_level, x);
    }
  }
  graph = NULL;


  /* generate random points for lake/sea effect */
  if (nrandom != 0){
    for (i = 0; i < dim2; i++) {
      if (bounding_box_margin > 0){
	xmin[i] -= bounding_box_margin;
	xmax[i] += bounding_box_margin;
      } else if (bounding_box_margin < 0) {
	xmin[i] -= boxsize[i]*(-bounding_box_margin);
	xmax[i] += boxsize[i]*(-bounding_box_margin);
      } else { // auto bounding box
	xmin[i] -= MAX(boxsize[i]*0.2, 2.*shore_depth_tol);
	xmax[i] += MAX(boxsize[i]*0.2, 2*shore_depth_tol);
      }
    }
    if (Verbose) {
      double bbm = bounding_box_margin;
      if (bbm > 0)
	fprintf (stderr, "bounding box margin: %.06f", bbm);
      else if (bbm < 0)
	fprintf (stderr, "bounding box margin: (%.06f * %.06f)", boxsize[0], -bbm);
      else
	fprintf (stderr, "bounding box margin: %.06f", MAX(boxsize[0]*0.2, 2*shore_depth_tol));
    }
    if (nrandom < 0) {
      double n1, n2, area2;
      area2 = (xmax[1] - xmin[1])*(xmax[0] - xmin[0]);
      n1 = (int) area2/(shore_depth_tol*shore_depth_tol);
      n2 = n*((int) area2/area);
      nrandom = MAX(n1, n2);
    }
    srand(123);
    xran = gv_calloc((nrandom + 4) * dim2, sizeof(double));
    int nz = 0;
    if (INCLUDE_OK_POINTS){
      nzok0 = nzok = nrandom - 1;/* points that are within tolerance of real or artificial points */
      if (grouping == grouping0) {
        int *grouping2 = gv_calloc(n + nrandom, sizeof(int));
        memcpy(grouping2, grouping, sizeof(int)*n);
        grouping = grouping2;
      } else {
        grouping = gv_recalloc(grouping, n, n + nrandom, sizeof(int));
      }
    }
    nn = n;

    for (i = 0; i < nrandom; i++){

      for (j = 0; j < dim2; j++){
	point[j] = xmin[j] + (xmax[j] - xmin[j])*drand();
      }
      
      QuadTree_get_nearest(qt, point, ymin, &imin, &min);

      if (min > shore_depth_tol){/* point not too close, accepted */
	for (j = 0; j < dim2; j++){
	  xran[nz*dim2+j] = point[j];
	}
	nz++;
      } else if (INCLUDE_OK_POINTS && min > shore_depth_tol/10){/* avoid duplicate points */
	for (j = 0; j < dim2; j++){
	  xran[nzok*dim2+j] = point[j];
	}
	grouping[nn++] = grouping[imin];
	nzok--;

      }

    }
    nrandom = nz;
    if (Verbose) fprintf(stderr, "nn nrandom=%d\n", nrandom);
  } else {
    xran = gv_calloc(4 * dim2, sizeof(double));
  }



  /* add 4 corners even if nrandom = 0. The corners should be further away from the other points to avoid skinny triangles */
  for (i = 0; i < dim2; i++) xmin[i] -= 0.2*(xmax[i]-xmin[i]);
  for (i = 0; i < dim2; i++) xmax[i] += 0.2*(xmax[i]-xmin[i]);
  i = nrandom;
  for (j = 0; j < dim2; j++) xran[i*dim2+j] = xmin[j];
  i++;
  for (j = 0; j < dim2; j++) xran[i*dim2+j] = xmax[j];
  i++;
  xran[i*dim2] = xmin[0]; xran[i*dim2+1] = xmax[1];
  i++;
  xran[i*dim2] = xmax[0]; xran[i*dim2+1] = xmin[1];
  nrandom += 4;


  double *xcombined;
  if (INCLUDE_OK_POINTS){
    xcombined = gv_calloc((nn + nrandom) * dim2, sizeof(double));
  } else {
    xcombined = gv_calloc((n + nrandom) * dim2, sizeof(double));
  }
  for (i = 0; i < n; i++) {
    for (j = 0; j < dim2; j++) xcombined[i*dim2+j] = x[i*dim+j];
  }
  for (i = 0; i < nrandom; i++) {
    for (j = 0; j < dim2; j++) xcombined[(i + nn)*dim2+j] = xran[i*dim+j];
  }

  if (INCLUDE_OK_POINTS){
    for (i = 0; i < nn - n; i++) {
      for (j = 0; j < dim2; j++) xcombined[(i + n)*dim2+j] = xran[(nzok0 - i)*dim+j];
    }
    n = nn;
  }


  {
    int nz, nh = 0;/* the set to highlight */
    if (HIGHLIGHT_SET){
      if (Verbose) fprintf(stderr," highlight cluster %d, n = %d\n",HIGHLIGHT_SET, n);
      /* shift set to the beginning */
      nz = 0;
      for (i = 0; i < n; i++){
	if (grouping[i] == HIGHLIGHT_SET){
	  nh++;
	  for (j = 0; j < dim; j++){
	    xcombined[nz++] = x[i*dim+j];
	  }
	}
      }
      for (i = 0; i < n; i++){
	if (grouping[i] != HIGHLIGHT_SET){
	  for (j = 0; j < dim; j++){
	    xcombined[nz++] = x[i*dim+j];
	  }
	}
      }
      assert(nz == n*dim);
      for (i = 0; i < nh; i++){
	grouping[i] = 1;
      }
      for (i = nh; i < n; i++){
	grouping[i] = 2;
      }
      nrandom += n - nh;/* count everything except cluster HIGHLIGHT_SET as random */
      n = nh;
      if (Verbose) fprintf(stderr,"nh = %d\n",nh);
    }
  }

  get_tri(n + nrandom, dim2, xcombined, &nt, &Tp, &E);
  get_polygons(n, nrandom, dim2, graph, grouping, nt, Tp, E, nverts, x_poly, poly_lines, polys, polys_groups,
	       poly_point_map, country_graph);

  free(xcombined);
  SparseMatrix_delete(E);
  free(Tp);
  free(xran);
  if (grouping != grouping0) free(grouping);
  if (x != x0) free(x);
}

static void add_point(int *n, int igrp, double **x, int *nmax, double point[], int **groups){

  if (*n >= *nmax){
    int old_nmax = *nmax;
    *nmax = 20 + *n;
    *x = gv_recalloc(*x, 2 * old_nmax, 2 * *nmax, sizeof(double));
    *groups = gv_recalloc(*groups, old_nmax, *nmax, sizeof(int));
  }

  (*x)[(*n)*2] = point[0];
  (*x)[(*n)*2+1] = point[1];
  (*groups)[*n] = igrp;
  (*n)++;
}

static void get_boundingbox(int n, int dim, double *x, double *width, double *bbox){
  int i;
  bbox[0] = bbox[1] = x[0];
  bbox[2] = bbox[3] = x[1];
  
  for (i = 0; i < n; i++){
    bbox[0] = MIN(bbox[0], x[i*dim] - width[i*dim]);
    bbox[1] = MAX(bbox[1], x[i*dim] + width[i*dim]);
    bbox[2] = MIN(bbox[2], x[i*dim + 1] - width[i*dim+1]);
    bbox[3] = MAX(bbox[3], x[i*dim + 1] + width[i*dim+1]);
  }
}

void make_map_from_rectangle_groups(bool include_OK_points,
				   int n, int dim, double *x, double *sizes, 
				   int *grouping, SparseMatrix graph0, double bounding_box_margin, int nrandom, int *nart, int nedgep, 
				   double shore_depth_tol,
				   int *nverts, double **x_poly, 
				   SparseMatrix *poly_lines, SparseMatrix *polys, int **polys_groups, SparseMatrix *poly_point_map, 
				   SparseMatrix *country_graph, int highlight_cluster){

  /* create a list of polygons from a list of rectangles in 2D. rectangles belong to groups. rectangles in the same group that are also close 
     geometrically will be in the same polygon describing the outline of the group. The main difference for this function and
     make_map_from_point_groups is that in this function, the input are points with width/heights, and we try not to place
     "lakes" inside these rectangles. This is achieved approximately by adding artificial points along the perimeter of the rectangles,
     as well as near the center.

     input:
     include_OK_points: OK points are random points inserted and found to be within shore_depth_tol of real/artificial points,
     .                  including them instead of throwing away increase realism of boundary 
     n: number of points
     dim: dimension of the points. If dim > 2, only the first 2D is used.
     x: coordinates
     sizes: width and height
     grouping: which group each of the vertex belongs to
     graph: the link structure between points. If graph == NULL, this is not used. otherwise
     .      it is assumed that matrix is symmetric and the graph is undirected
     bounding_box_margin: margin used to form the bounding box.
     .      if negative, it is taken as relative. i.e., -0.5 means a margin of 0.5*box_size
     nrandom (input): number of random points to insert in the bounding box to figure out lakes and seas.
     .        If nrandom = 0, no points are inserted, if nrandom < 0, the number is decided automatically.
     .        
     nart: on entry, number of artificial points to be added along each side of a rectangle enclosing the labels. if < 0, auto-selected.
     . On exit, actual number of artificial points added.
     nedgep: number of artificial points are adding along edges to establish as much as possible a bright between nodes 
     .       connected by the edge, and avoid islands that are connected. k = 0 mean no points.
     shore_depth_tol: nrandom random points are inserted in the bounding box of the points,
     .      such random points are then weeded out if it is within distance of shore_depth_tol from 
     .      real points. If 0, auto assigned

     output:
     nverts: number of vertices in the Voronoi diagram
     x_poly: the 2D coordinates of these polygons, dimension nverts*2
     poly_lines: the sparse matrix representation of the polygon indices, as well as their identity. The matrix is of size
     .       npolygons x nverts. The i-th polygon is formed by linking vertices with index in the i-th row of the sparse matrix.
     .       Each row is of the form {{i,j1,m},...{i,jk,m},{i,j1,m},{i,l1,m+1},...}, where j1--j2--jk--j1 form one loop,
     .       and l1 -- l2 -- ... form another. Each row can have more than 1 loop only when the connected region the polylines represent
     .       has at least 1 holes.
     polys: the sparse matrix representation of the polygon indices, as well as their identity. The matrix is of size
     .       npolygons x nverts. The i-th polygon is formed by linking vertices with index in the i-th row of the sparse matrix.
     .       Unlike poly_lines, here each row represent an one stroke drawing of the SOLID polygon, vertices
     .       along this path may repeat
     polys_groups: the group (color) each polygon belongs to, this include all groups of the real points,
     .       plus the random point group and the bounding box group
     poly_point_map: a matrix of dimension npolys x (n + nrandom), poly_point_map[i,j] != 0 if polygon i contains the point j.
     .  If j < n, it is the original point, otherwise it is artificial point (forming the rectangle around a label) or random points.
    country_graph: shows which country is a neighbor of which country.
     .     if country i and country j are neighbor, then the {i,j} entry is the total number of vertices that
     .     belongs to i and j, and share an edge of the triangulation. In addition, {i,i} and {j,j} have values equal 
     .     to the number of vertices in each of the countries. If the input "grouping" has negative or zero value, then
     .     country_graph = NULL. 

     
  */
  double *X;
  int N, nmax, i, j, k, igrp;
  int *groups, K = *nart;/* average number of points added per side of rectangle */

  double avgsize[2],  avgsz, h[2], p1, p0;
  double point[2];
  int nadded[2];
  double delta[2];
  SparseMatrix graph = graph0;
  double bbox[4];

  if (K < 0){
    K = (int) 10/(1+n/400.);/* 0 if n > 3600*/
  }
  *nart = 0;
  if (Verbose){
    int maxgp = grouping[0];
    int mingp = grouping[0];
    for (i = 0; i < n; i++) {
      maxgp = MAX(maxgp, grouping[i]);
      mingp = MIN(mingp, grouping[i]);
    }
    fprintf(stderr, "max grouping - min grouping + 1 = %d\n",maxgp - mingp + 1); 
  }

  if (!sizes){
    make_map_internal(include_OK_points, n, dim, x, grouping, graph, bounding_box_margin, nrandom, nedgep,
			    shore_depth_tol, nverts, x_poly, 
			     poly_lines, polys, polys_groups, poly_point_map, country_graph, highlight_cluster);
    return;
  } else {

    /* add artificial node due to node sizes */
    avgsize[0] = 0;
    avgsize[1] = 0;
    for (i = 0; i < n; i++){
      for (j = 0; j < 2; j++) {
	avgsize[j] += sizes[i*dim+j];
      }
    }
    for (i = 0; i < 2; i++) avgsize[i] /= n;
    avgsz = 0.5*(avgsize[0] + avgsize[1]);
    if (Verbose) fprintf(stderr, "avgsize = {%f, %f}\n",avgsize[0], avgsize[1]);

    nmax = 2*n;
    X = gv_calloc(dim * (n + nmax), sizeof(double));
    groups = gv_calloc(n + nmax, sizeof(int));
    for (i = 0; i < n; i++) {
      groups[i] = grouping[i];
      for (j = 0; j < 2; j++){
	X[i*2+j] = x[i*dim+j];
      }
    }
    N = n;

    if (shore_depth_tol < 0) {
      shore_depth_tol = -(shore_depth_tol)*avgsz;
    } else if (shore_depth_tol == 0){
      double area;
      get_boundingbox(n, dim, x, sizes, bbox);
      area = (bbox[1] - bbox[0])*(bbox[3] - bbox[2]);
      shore_depth_tol = sqrt(area/(double) n); 
      if (Verbose) fprintf(stderr,"setting shore length ======%f\n",shore_depth_tol);
    } else {
    }

    /* add artificial points in an anti-clockwise fashion */

    if (K > 0){
      delta[0] = .5*avgsize[0]/K; delta[1] = .5*avgsize[1]/K;/* small perturbation to make boundary between labels looks more fractal */
    } else {
      delta[0] = delta[1] = 0.;
    }
    for (i = 0; i < n; i++){
      igrp = grouping[i];
      for (j = 0; j < 2; j++) {
	if (avgsz == 0){
	  nadded[j] = 0;
	} else {
	  nadded[j] = (int) K*sizes[i*dim+j]/avgsz;
	}
      }

      /*top: left to right */
      if (nadded[0] > 0){
	h[0] = sizes[i*dim]/nadded[0];
	point[0] = x[i*dim] - sizes[i*dim]/2;
	p1 = point[1] = x[i*dim+1] + sizes[i*dim + 1]/2;
	add_point(&N, igrp, &X, &nmax, point, &groups);
	for (k = 0; k < nadded[0] - 1; k++){
	  point[0] += h[0];
	  point[1] = p1 + (0.5-drand())*delta[1];
	  add_point(&N, igrp, &X, &nmax, point, &groups);
	}
	
	/* bot: right to left */
	point[0] = x[i*dim] + sizes[i*dim]/2;
	p1 = point[1] = x[i*dim+1] - sizes[i*dim + 1]/2;
	add_point(&N, igrp, &X, &nmax, point, &groups);
	for (k = 0; k < nadded[0] - 1; k++){
	  point[0] -= h[0];
	  point[1] = p1 + (0.5-drand())*delta[1];
	  add_point(&N, igrp, &X, &nmax, point, &groups);
	}
      }

      if (nadded[1] > 0){	
	/* left: bot to top */
	h[1] = sizes[i*dim + 1]/nadded[1];
	p0 = point[0] = x[i*dim] - sizes[i*dim]/2;
	point[1] = x[i*dim+1] - sizes[i*dim + 1]/2;
	add_point(&N, igrp, &X, &nmax, point, &groups);
	for (k = 0; k < nadded[1] - 1; k++){
	  point[0] = p0 + (0.5-drand())*delta[0];
	  point[1] += h[1];
	  add_point(&N, igrp, &X, &nmax, point, &groups);
	}
	
	/* right: top to bot */
	p0 = point[0] = x[i*dim] + sizes[i*dim]/2;
	point[1] = x[i*dim+1] + sizes[i*dim + 1]/2;
	add_point(&N, igrp, &X, &nmax, point, &groups);
	for (k = 0; k < nadded[1] - 1; k++){
	  point[0] = p0 + (0.5-drand())*delta[0];
	  point[1] -= h[1];
	  add_point(&N, igrp, &X, &nmax, point, &groups);
	}	
      }
      *nart = N - n;

    }/* done adding artificial points due to node size*/

    make_map_internal(include_OK_points, N, dim, X, groups, graph, bounding_box_margin, nrandom, nedgep,
			    shore_depth_tol, nverts, x_poly, 
			    poly_lines, polys, polys_groups, poly_point_map, country_graph, highlight_cluster);
    if (graph != graph0) SparseMatrix_delete(graph);
    free(groups);
    free(X);
  }
}
