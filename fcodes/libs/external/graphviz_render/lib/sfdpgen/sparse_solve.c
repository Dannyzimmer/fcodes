/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <assert.h>
#include <cgraph/alloc.h>
#include <string.h>
#include <sfdpgen/sparse_solve.h>
#include <sfdpgen/sfdpinternal.h>
#include <common/memory.h>
#include <math.h>
#include <common/arith.h>
#include <common/types.h>
#include <common/globals.h>

/* #define DEBUG_PRINT */

struct uniform_stress_matmul_data{
  double alpha;
  SparseMatrix A;
};

static double *Operator_uniform_stress_matmul_apply(Operator o, double *x, double *y){
  struct uniform_stress_matmul_data *d = o->data;
  SparseMatrix A = d->A;
  double alpha = d->alpha;
  double xsum = 0.;
  int m = A->m, i;

  SparseMatrix_multiply_vector(A, x, &y);

  /* alpha*V*x */
  for (i = 0; i < m; i++) xsum += x[i];

  for (i = 0; i < m; i++) y[i] += alpha*(m*x[i] - xsum);

  return y;
}



Operator Operator_uniform_stress_matmul(SparseMatrix A, double alpha){
  Operator o;
  struct uniform_stress_matmul_data *d;

  o = MALLOC(sizeof(struct Operator_struct));
  o->data = d = MALLOC(sizeof(struct uniform_stress_matmul_data));
  d->alpha = alpha;
  d->A = A;
  o->Operator_apply = Operator_uniform_stress_matmul_apply;
  return o;
}


static double *Operator_matmul_apply(Operator o, double *x, double *y){
  SparseMatrix A = o->data;
  SparseMatrix_multiply_vector(A, x, &y);
  return y;
}

static Operator Operator_matmul_new(SparseMatrix A){
  Operator o = gv_alloc(sizeof(struct Operator_struct));
  o->data = A;
  o->Operator_apply = Operator_matmul_apply;
  return o;
}


static void Operator_matmul_delete(Operator o){
  free(o);
}


static double* Operator_diag_precon_apply(Operator o, double *x, double *y){
  int i, m;
  double *diag = o->data;
  m = (int) diag[0];
  diag++;
  for (i = 0; i < m; i++) y[i] = x[i]*diag[i];
  return y;
}


Operator Operator_uniform_stress_diag_precon_new(SparseMatrix A, double alpha){
  Operator o;
  double *diag;
  int i, j, m = A->m, *ia = A->ia, *ja = A->ja;
  double *a = A->a;

  assert(A->type == MATRIX_TYPE_REAL);

  assert(a);

  o = MALLOC(sizeof(struct Operator_struct));
  o->data = MALLOC(sizeof(double)*(m + 1));
  diag = o->data;

  diag[0] = m;
  diag++;
  for (i = 0; i < m; i++){
    diag[i] = 1./(m-1);
    for (j = ia[i]; j < ia[i+1]; j++){
      if (i == ja[j] && fabs(a[j]) > 0) diag[i] = 1./((m-1)*alpha+a[j]);
    }
  }

  o->Operator_apply = Operator_diag_precon_apply;

  return o;
}


static Operator Operator_diag_precon_new(SparseMatrix A){
  Operator o;
  double *diag;
  int i, j, m = A->m, *ia = A->ia, *ja = A->ja;
  double *a = A->a;

  assert(A->type == MATRIX_TYPE_REAL);

  assert(a);

  o = N_GNEW(1,struct Operator_struct);
  o->data = N_GNEW((A->m + 1),double);
  diag = o->data;

  diag[0] = m;
  diag++;
  for (i = 0; i < m; i++){
    diag[i] = 1.;
    for (j = ia[i]; j < ia[i+1]; j++){
      if (i == ja[j] && fabs(a[j]) > 0) diag[i] = 1./a[j];
    }
  }

  o->Operator_apply = Operator_diag_precon_apply;

  return o;
}

static void Operator_diag_precon_delete(Operator o){
  free(o->data);
  free(o);
}

static double conjugate_gradient(Operator A, Operator precon, int n, double *x, double *rhs, double tol, int maxit){
  double *z, *r, *p, *q, res = 10*tol, alpha;
  double rho = 1.0e20, rho_old = 1, res0, beta;
  double* (*Ax)(Operator o, double *in, double *out) = A->Operator_apply;
  double* (*Minvx)(Operator o, double *in, double *out) = precon->Operator_apply;
  int iter = 0;

  z = N_GNEW(n,double);
  r = N_GNEW(n,double);
  p = N_GNEW(n,double);
  q = N_GNEW(n,double);

  r = Ax(A, x, r);
  r = vector_subtract_to(n, rhs, r);

  res0 = res = sqrt(vector_product(n, r, r))/n;
#ifdef DEBUG_PRINT
    if (Verbose){
      fprintf(stderr, "on entry, cg iter = %d of %d, residual = %g, tol = %g\n", iter, maxit, res, tol);
    }
#endif

  while ((iter++) < maxit && res > tol*res0){
    z = Minvx(precon, r, z);
    rho = vector_product(n, r, z);

    if (iter > 1){
      beta = rho/rho_old;
      p = vector_saxpy(n, z, p, beta);
    } else {
      memcpy(p, z, sizeof(double)*n);
    }

    q = Ax(A, p, q);

    alpha = rho/vector_product(n, p, q);

    x = vector_saxpy2(n, x, p, alpha);
    r = vector_saxpy2(n, r, q, -alpha);
    
    res = sqrt(vector_product(n, r, r))/n;

#ifdef DEBUG_PRINT
    if (Verbose && 0){
      fprintf(stderr, "   cg iter = %d, residual = %g, relative res = %g\n", iter, res, res/res0);
    }
#endif



    rho_old = rho;
  }
  free(z); free(r); free(p); free(q);
#ifdef DEBUG
    _statistics[0] += iter - 1;
#endif

#ifdef DEBUG_PRINT
  if (Verbose){
    fprintf(stderr, "   cg iter = %d, residual = %g, relative res = %g\n", iter, res, res/res0);
  }
#endif
  return res;
}

double cg(Operator Ax, Operator precond, int n, int dim, double *x0, double *rhs, double tol, int maxit){
  double *x, *b, res = 0;
  int k, i;
  x = N_GNEW(n, double);
  b = N_GNEW(n, double);
  for (k = 0; k < dim; k++){
    for (i = 0; i < n; i++) {
      x[i] = x0[i*dim+k];
      b[i] = rhs[i*dim+k];
    }
    
    res += conjugate_gradient(Ax, precond, n, x, b, tol, maxit);
    for (i = 0; i < n; i++) {
      rhs[i*dim+k] = x[i];
    }
  }
  free(x);
  free(b);
  return res;
}

double SparseMatrix_solve(SparseMatrix A, int dim, double *x0, double *rhs, double tol, int maxit){
  Operator Ax, precond;
  int n = A->m;
  double res = 0;

  Ax =  Operator_matmul_new(A);
  precond = Operator_diag_precon_new(A);
  res = cg(Ax, precond, n, dim, x0, rhs, tol, maxit);
  Operator_matmul_delete(Ax);
  Operator_diag_precon_delete(precond);
  return res;
}

