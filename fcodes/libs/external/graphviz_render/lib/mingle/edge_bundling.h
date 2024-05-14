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
#include <vector>

struct pedge_struct {
  double wgt; /* weight, telling how many original edges this edge represent. If this edge consists of multiple sections of different weights then this is a lower bound. This only applied for agglomerative bundling */
  int npoints;/* number of poly points */
  int len;/* length of arra x. len >= npoints */
  int dim;/* dim >= 2. Point i is stored from x[i*dim]*/
  double edge_length;
  double *x;/* coordinates of the npoints poly points. Dimension dim*npoints */
  std::vector<double> wgts;/* number of original edges each section represnet. Dimension npoint - 1. This only applied for agglomerative bundling Null for other methods */
};

typedef struct pedge_struct* pedge;

pedge* edge_bundling(SparseMatrix A, int dim, double *x, int maxit_outer, double K, int method, int nneighbor, int compatibility_method, int max_recursion, double angle_param, double angle);
void pedge_delete(pedge e);
pedge pedge_wgts_realloc(pedge e, int n);
void pedge_export_gv(FILE *fp, int ne, pedge *edges);
enum {METHOD_NONE = -1, METHOD_FD, METHOD_INK_AGGLOMERATE, METHOD_INK};
enum {COMPATIBILITY_DIST = 0, COMPATIBILITY_FULL};
pedge pedge_new(int np, int dim, double *x);
pedge pedge_wgt_new(int np, int dim, double *x, double wgt);
pedge pedge_double(pedge e);
