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

#include <sparse/SparseMatrix.h>
#include <cgraph/cgraph.h>
#include <stdbool.h>

void make_map_from_rectangle_groups(
    bool include_OK_points, int n, int dim, double *x, double *sizes,
    int *grouping, SparseMatrix graph, double bounding_box_margin, int nrandom,
    int *nart, int nedgep, double shore_depth_tol, int *nverts, double **x_poly,
    SparseMatrix *poly_lines, SparseMatrix *polys, int **polys_groups,
    SparseMatrix *poly_point_map, SparseMatrix *country_graph,
    int highlight_cluster);

void improve_contiguity(int n, int dim, int *grouping, SparseMatrix poly_point_map, double *x, SparseMatrix graph);

void plot_dot_map(Agraph_t* gr, int n, int dim, double *x, SparseMatrix polys,
                  SparseMatrix poly_lines, double line_width,
                  const char *line_color, double *x_poly, int *polys_groups,
                  char **labels, float *fsz, float *r, float *g, float *b,
                  const char* opacity, SparseMatrix A, FILE*);

void map_optimal_coloring(int seed, SparseMatrix A, float *rgb_r,  float *rgb_g, float *rgb_b);
void map_palette_optimal_coloring(char *color_scheme, SparseMatrix A,
                                  float **rgb_r, float **rgb_g, float **rgb_b);

enum {POLY_LINE_REAL_EDGE, POLY_LINE_NOT_REAL_EDGE};
#define neighbor(t, i, edim, elist) elist[(edim)*(t)+i]
#define edge_head(e) edge_table[2*(e)]
#define edge_tail(e) edge_table[2*(e)+1]
#define cycle_prev(e) cycle[2*(e)]
#define cycle_next(e) cycle[2*(e)+1]
