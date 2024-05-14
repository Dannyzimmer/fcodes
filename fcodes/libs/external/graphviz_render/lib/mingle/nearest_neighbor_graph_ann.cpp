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

#ifdef HAVE_ANN

#include <ANN/ANN.h>					// ANN declarations
#include <mingle/nearest_neighbor_graph_ann.h>
#include <utility>
#include <vector>

static const int dim = 4; // dimension

/*
static void printPt(ostream &out, ANNpoint p)			// print point
{
  out << "" << p[0];
  for (int i = 1; i < dim; i++) {
    out << "," << p[i];
  }
  out << "";
}
*/

static void sortPtsX(int n, ANNpointArray pts){
  /* sort so that edges always go from left to right in x-doordinate */
  for (int i = 0; i < n; i++){
    ANNpoint p = pts[i];
    if (p[0] < p[2] || (p[0] == p[2] && p[1] < p[3])) continue;
    std::swap(p[0], p[2]);
    std::swap(p[1], p[3]);
  }
}

static void sortPtsY(int n, ANNpointArray pts){
  /* sort so that edges always go from left to right in x-doordinate */
  for (int i = 0; i < n; i++){
    ANNpoint p = pts[i];
    if (p[1] < p[3] || (p[1] == p[3] && p[0] < p[2])) continue;
    std::swap(p[0], p[2]);
    std::swap(p[1], p[3]);
  }
}

void nearest_neighbor_graph_ann(int nPts, int k, double *x,
                                int &nz0, std::vector<int> &irn,
                                std::vector<int> &jcn,
                                std::vector<double> &val) {

  /* Gives a nearest neighbor graph is a list of dim-dimendional points. The connectivity is in irn/jcn, and the distance in val.
     
    nPts: number of points
    dim: dimension
    k: number of neighbors needed
    x: nPts*dim vector. The i-th point is x[i*dim : i*dim + dim - 1]
    nz: number of entries in the connectivity matrix irn/jcn/val
    irn, jcn: the connectivity 
    val: the distance

    note that there could be repeates
  */

  // error tolerance
  const double eps = 0;

  ANNpointArray dataPts = annAllocPts(nPts, dim);			// allocate data points
  std::vector<ANNidx> nnIdx(k);						// allocate near neighbor indices
  std::vector<ANNdist> dists(k);					// allocate near neighbor dists

  for (int i = 0; i < nPts; i++){
    double *xx =  dataPts[i];
    for (int j = 0; j < dim; j++) xx[j] = x[i*dim + j];
  }

  //========= graph when sort based on x ========
  int nz = 0;
  sortPtsX(nPts, dataPts);
  ANNkd_tree kdTree(					// build search structure
			  dataPts,					// the data points
			  nPts,						// number of points
			  dim);						// dimension of space
  for (int ip = 0; ip < nPts; ip++){
    kdTree.annkSearch(						// search
		       dataPts[ip],						// query point
		       k,								// number of near neighbors
		       nnIdx.data(),						// nearest neighbors (returned)
		       dists.data(),						// distance (returned)
		       eps);							// error bound

    for (int i = 0; i < k; i++) {			// print summary
      if (nnIdx[i] == ip) continue;
      val[nz] = dists[i];
      irn[nz] = ip;
      jcn[nz++] = nnIdx[i];
      //cout << ip << "--" << nnIdx[i] << " [len = " << dists[i]<< ", weight = \"" << 1./(dists[i]) << "\", wt = \"" << 1./(dists[i]) << "\"]\n";
      //printPt(cout, dataPts[ip]);
      //cout << "--";
      //printPt(cout, dataPts[nnIdx[i]]);
      //cout << "\n";
    }
  }
  

  //========= graph when sort based on y ========
  sortPtsY(nPts, dataPts);
  kdTree = ANNkd_tree(					// build search structure
			  dataPts,					// the data points
			  nPts,						// number of points
			  dim);						// dimension of space
  for (int ip = 0; ip < nPts; ip++){
    kdTree.annkSearch(						// search
		       dataPts[ip],						// query point
		       k,								// number of near neighbors
		       nnIdx.data(),						// nearest neighbors (returned)
		       dists.data(),						// distance (returned)
		       eps);							// error bound
      
    for (int i = 0; i < k; i++) {			// print summary
      if (nnIdx[i] == ip) continue;
      val[nz] = dists[i];
      irn[nz] = ip;
      jcn[nz++] = nnIdx[i];
      // cout << ip << "--" << nnIdx[i] << " [len = " << dists[i]<< ", weight = \"" << 1./(dists[i]) << "\", wt = \"" << 1./(dists[i]) << "\"]\n";
    }
  }
    
  nz0 = nz;

  annDeallocPts(dataPts);
  annClose();									// done with ANN



}
 
#endif  /* HAVE_ANN */
