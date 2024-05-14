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
#include <stdbool.h>

enum {ERROR_NOT_SQUARE_MATRIX = -100};

/* a flag to indicate that p should be set auto */
#define AUTOP -1.0001234

enum {SMOOTHING_NONE, SMOOTHING_STRESS_MAJORIZATION_GRAPH_DIST, SMOOTHING_STRESS_MAJORIZATION_AVG_DIST, SMOOTHING_STRESS_MAJORIZATION_POWER_DIST, SMOOTHING_SPRING, SMOOTHING_TRIANGLE, SMOOTHING_RNG};

enum {QUAD_TREE_HYBRID_SIZE = 10000};

enum {QUAD_TREE_NONE = 0, QUAD_TREE_NORMAL, QUAD_TREE_FAST, QUAD_TREE_HYBRID};

struct spring_electrical_control_struct {
  double p;/*a negativve real number default to -1. repulsive force = dist^p */
  double q;/*a positive real number default to 2. attractive force = dist^q */
  int random_start;/* whether to apply SE from a random layout, or from exisiting layout */
  double K;/* the natural distance. If K < 0, K will be set to the average distance of an edge */
  double C;/* another parameter. f_a(i,j) = C*dist(i,j)^2/K * d_ij, f_r(i,j) = K^(3-p)/dist(i,j)^(-p). By default C = 0.2. */
  int multilevels;/* if <=1, single level */
  int quadtree_size;/* cut off size above which quadtree approximation is used */
  int max_qtree_level;/* max level of quadtree */
  double bh;/* Barnes-Hutt constant, if width(snode)/dist[i,snode] < bh, treat snode as a supernode. default 0.2*/
  double tol;/* minimum different between two subsequence config before terminating. ||x-xold|| < tol */
  int maxiter;
  double cool;/* default 0.9 */
  double step;/* initial step size */
  int adaptive_cooling;
  int random_seed;
  bool beautify_leaves : 1;
  int smoothing;
  int overlap;
  bool do_shrinking;
  int tscheme; /* octree scheme. 0 (no octree), 1 (normal), 2 (fast) */
  double initial_scaling;/* how to scale the layout of the graph before passing to overlap removal algorithm.
			  positive values are absolute in points, negative values are relative
			  to average label size.
			  */
  double rotation;/* degree of rotation */
  int edge_labeling_scheme; /* specifying whether to treat node of the form |edgelabel|* as a special node representing an edge label. 
			       0 (no action, default), 1 (penalty based method to make that kind of node close to the center of its neighbor), 
			       1 (penalty based method to make that kind of node close to the old center of its neighbor),
			       3 (two step process of overlap removal and straightening) */
};

typedef struct  spring_electrical_control_struct  *spring_electrical_control; 

spring_electrical_control spring_electrical_control_new(void);
void spring_electrical_control_print(spring_electrical_control ctrl);

void spring_electrical_embedding(int dim, SparseMatrix A0, spring_electrical_control ctrl, double *x, int *flag);
void spring_electrical_embedding_fast(int dim, SparseMatrix A0, spring_electrical_control ctrl, double *x, int *flag);

void multilevel_spring_electrical_embedding(int dim, SparseMatrix A0, SparseMatrix D, spring_electrical_control ctrl, double *label_sizes, 
					    double *x, int n_edge_label_nodes, int *edge_label_nodes, int *flag);

void export_embedding(FILE *fp, int dim, SparseMatrix A, double *x, double *width);
void spring_electrical_control_delete(spring_electrical_control ctrl);
void print_matrix(double *x, int n, int dim);

double average_edge_length(SparseMatrix A, int dim, double *coord);

void spring_electrical_spring_embedding(int dim, SparseMatrix A, SparseMatrix D, spring_electrical_control ctrl, double *x, int *flag);
void force_print(FILE *fp, int n, int dim, double *x, double *force);

enum {MAX_I = 20, OPT_UP = 1, OPT_DOWN = -1, OPT_INIT = 0};
struct oned_optimizer_struct{
  int i;
  double work[MAX_I+1];
  int direction;
};
typedef struct oned_optimizer_struct *oned_optimizer;
void oned_optimizer_delete(oned_optimizer opt);
oned_optimizer oned_optimizer_new(int i);
void oned_optimizer_train(oned_optimizer opt, double work);
int oned_optimizer_get(oned_optimizer opt);
void interpolate_coord(int dim, SparseMatrix A, double *x);
int power_law_graph(SparseMatrix A);
void pcp_rotate(int n, int dim, double *x);
