/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <cgraph/alloc.h>
#include "power.h"
#include <sparse/SparseMatrix.h>

// Maxium of iterations that will be done in power_method
static const int maxit = 100;

// Accuracy control (convergence criterion) for power_method
static const double tolerance = 0.00001;

double *power_method(void *A, int n, int random_seed) {
  /* find largest eigenvector of a matrix A.

     This converges only if the largest eigenvector/value is real (e.g., if A is symmetric) and the
     next largest eigenvalues separate from the largest ones

     input:
     A: the matrix
     n: dimension of matrix A
     random_seed: seed for eigenvector initialization
     matrix: max number f iterations

     output:
     eigv: eigenvectors. The i-th is at eigvs[i*n, i*(n+1) - 1]

     Function PowerIteration (A – m × m matrix )
     % This function computes u1, u2, . . . , uk, the first k eigenvectors of S.
     const tolerance ← 0.001
     ui ← random
     ui ← ui/||ui||
     do
       vi ← ui
       ui ← A vi/||A vi||
     while (ui^T vi < 1-tolerance) (halt when direction change is small)
     vi = ui
     return v1,v2,...
   */
  double *v, *u, *vv;
  int iter = 0;
  double res, unorm;
  int i;

  double *eigv = gv_calloc(n, sizeof(double));

  vv = gv_calloc(n, sizeof(double));
  u = gv_calloc(n, sizeof(double));

  srand((unsigned)random_seed);

  v = &eigv[n];
  for (i = 0; i < n; i++) u[i] = drand();
  res = sqrt(vector_product(n, u, u));
  if (res > 0) res =  1/res;
  for (i = 0; i < n; i++) {
    u[i] = u[i]*res;
    v[i] = u[i];
  }
  iter = 0;
  do {
    SparseMatrix_multiply_vector(A, u, &vv);

    unorm = vector_product(n, vv, vv);/* ||u||^2 */
    unorm = sqrt(unorm);
    if (unorm > 0) {
      unorm = 1/unorm;
    } else {
      // ||A.v||=0, so v must be an eigenvec correspond to eigenvalue zero
      for (i = 0; i < n; i++) vv[i] = u[i];
      unorm = sqrt(vector_product(n, vv, vv));
      if (unorm > 0) unorm = 1/unorm;
    }
    res = 0.;

    for (i = 0; i < n; i++) {
      u[i] = vv[i]*unorm;
      res = res + u[i]*v[i];
      v[i] = u[i];
    }
  } while (res < 1 - tolerance && iter++ < maxit);
  free(u);
  free(vv);

  return eigv;
}
