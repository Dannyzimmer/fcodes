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

#include <sparse/general.h>
#include <sparse/SparseMatrix.h>
#include <mingle/nearest_neighbor_graph_ann.h>
#include <mingle/nearest_neighbor_graph.h>
#include <vector>

SparseMatrix nearest_neighbor_graph(int nPts, int num_neighbors, double *x) {
  /* Gives a nearest neighbor graph of a list of dim-dimendional points. The result is a sparse matrix
     of nPts x nPts, with num_neigbors entries per row.

    nPts: number of points
    num_neighbors: number of neighbors needed
    dim: dimension == 4
    x: nPts*dim vector. The i-th point is x[i*dim : i*dim + dim - 1]

  */
  SparseMatrix A = NULL;

#ifdef HAVE_ANN
  int nz;
  int k = num_neighbors;

  /* need to *2 as we do two sweeps of neighbors, so could have repeats */
  std::vector<int> irn(nPts * k * 2);
  std::vector<int> jcn(nPts * k * 2);
  std::vector<double> val(nPts * k * 2);

  nearest_neighbor_graph_ann(nPts, num_neighbors, x, nz, irn, jcn, val);

  A = SparseMatrix_from_coordinate_arrays(nz, nPts, nPts, irn.data(),
                                          jcn.data(), val.data(),
                                          MATRIX_TYPE_REAL, sizeof(double));
#else
  (void)nPts;
  (void)num_neighbors;
  (void)x;
#endif

  return A;
}
