/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/


#include <neatogen/matrix_ops.h>
#include <common/memory.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

static double p_iteration_threshold = 1e-3;

bool power_iteration(double **square_mat, int n, int neigs, double **eigs,
		double *evals, int initialize)
{
    /* compute the 'neigs' top eigenvectors of 'square_mat' using power iteration */

    int i, j;
    double *tmp_vec = N_GNEW(n, double);
    double *last_vec = N_GNEW(n, double);
    double *curr_vector;
    double len;
    double angle;
    double alpha;
    int iteration = 0;
    int largest_index;
    double largest_eval;
    int Max_iterations = 30 * n;

    double tol = 1 - p_iteration_threshold;

    if (neigs >= n) {
	neigs = n;
    }

    for (i = 0; i < neigs; i++) {
	curr_vector = eigs[i];
	/* guess the i-th eigen vector */
      choose:
	if (initialize)
	    for (j = 0; j < n; j++)
		curr_vector[j] = rand() % 100;
	/* orthogonalize against higher eigenvectors */
	for (j = 0; j < i; j++) {
	    alpha = -dot(eigs[j], 0, n - 1, curr_vector);
	    scadd(curr_vector, 0, n - 1, alpha, eigs[j]);
	}
	len = norm(curr_vector, 0, n - 1);
	if (len < 1e-10) {
	    /* We have chosen a vector colinear with prvious ones */
	    goto choose;
	}
	vecscale(curr_vector, 0, n - 1, 1.0 / len, curr_vector);
	iteration = 0;
	do {
	    iteration++;
	    cpvec(last_vec, 0, n - 1, curr_vector);

	    right_mult_with_vector_d(square_mat, n, n, curr_vector,
				     tmp_vec);
	    cpvec(curr_vector, 0, n - 1, tmp_vec);

	    /* orthogonalize against higher eigenvectors */
	    for (j = 0; j < i; j++) {
		alpha = -dot(eigs[j], 0, n - 1, curr_vector);
		scadd(curr_vector, 0, n - 1, alpha, eigs[j]);
	    }
	    len = norm(curr_vector, 0, n - 1);
	    if (len < 1e-10 || iteration > Max_iterations) {
		/* We have reached the null space (e.vec. associated with e.val. 0) */
		goto exit;
	    }

	    vecscale(curr_vector, 0, n - 1, 1.0 / len, curr_vector);
	    angle = dot(curr_vector, 0, n - 1, last_vec);
	} while (fabs(angle) < tol);
	evals[i] = angle * len;	/* this is the Rayleigh quotient (up to errors due to orthogonalization):
				   u*(A*u)/||A*u||)*||A*u||, where u=last_vec, and ||u||=1
				 */
    }
  exit:
    for (; i < neigs; i++) {
	/* compute the smallest eigenvector, which are  */
	/* probably associated with eigenvalue 0 and for */
	/* which power-iteration is dangerous */
	curr_vector = eigs[i];
	/* guess the i-th eigen vector */
	for (j = 0; j < n; j++)
	    curr_vector[j] = rand() % 100;
	/* orthogonalize against higher eigenvectors */
	for (j = 0; j < i; j++) {
	    alpha = -dot(eigs[j], 0, n - 1, curr_vector);
	    scadd(curr_vector, 0, n - 1, alpha, eigs[j]);
	}
	len = norm(curr_vector, 0, n - 1);
	vecscale(curr_vector, 0, n - 1, 1.0 / len, curr_vector);
	evals[i] = 0;

    }


    /* sort vectors by their evals, for overcoming possible mis-convergence: */
    for (i = 0; i < neigs - 1; i++) {
	largest_index = i;
	largest_eval = evals[largest_index];
	for (j = i + 1; j < neigs; j++) {
	    if (largest_eval < evals[j]) {
		largest_index = j;
		largest_eval = evals[largest_index];
	    }
	}
	if (largest_index != i) {	/* exchange eigenvectors: */
	    cpvec(tmp_vec, 0, n - 1, eigs[i]);
	    cpvec(eigs[i], 0, n - 1, eigs[largest_index]);
	    cpvec(eigs[largest_index], 0, n - 1, tmp_vec);

	    evals[largest_index] = evals[i];
	    evals[i] = largest_eval;
	}
    }

    free(tmp_vec);
    free(last_vec);

    return (iteration <= Max_iterations);
}



void
mult_dense_mat(double **A, float **B, int dim1, int dim2, int dim3,
	       float ***CC)
{
/*
  A is dim1 x dim2, B is dim2 x dim3, C = A x B 
*/

    double sum;
    int i, j, k;
    float *storage;
    float **C = *CC;
    if (C != NULL) {
	storage = realloc(C[0], dim1 * dim3 * sizeof(A[0]));
	*CC = C = realloc(C, dim1 * sizeof(A));
    } else {
	storage = malloc(dim1 * dim3 * sizeof(A[0]));
	*CC = C = malloc(dim1 * sizeof(A));
    }

    for (i = 0; i < dim1; i++) {
	C[i] = storage;
	storage += dim3;
    }

    for (i = 0; i < dim1; i++) {
	for (j = 0; j < dim3; j++) {
	    sum = 0;
	    for (k = 0; k < dim2; k++) {
		sum += A[i][k] * B[k][j];
	    }
	    C[i][j] = (float) (sum);
	}
    }
}

void
mult_dense_mat_d(double **A, float **B, int dim1, int dim2, int dim3,
		 double ***CC)
{
/*
  A is dim1 x dim2, B is dim2 x dim3, C = A x B 
*/
    double **C = *CC;
    double *storage;
    int i, j, k;
    double sum;

    if (C != NULL) {
	storage = realloc(C[0], dim1 * dim3 * sizeof(double));
	*CC = C = realloc(C, dim1 * sizeof(double *));
    } else {
	storage = malloc(dim1 * dim3 * sizeof(double));
	*CC = C = malloc(dim1 * sizeof(double *));
    }

    for (i = 0; i < dim1; i++) {
	C[i] = storage;
	storage += dim3;
    }

    for (i = 0; i < dim1; i++) {
	for (j = 0; j < dim3; j++) {
	    sum = 0;
	    for (k = 0; k < dim2; k++) {
		sum += A[i][k] * B[k][j];
	    }
	    C[i][j] = sum;
	}
    }
}

void
mult_sparse_dense_mat_transpose(vtx_data * A, double **B, int dim1,
				int dim2, float ***CC)
{
/*
  A is dim1 x dim1 and sparse, B is dim2 x dim1, C = A x B 
*/

    float *storage;
    int i, j, k;
    double sum;
    float *ewgts;
    int *edges;
    int nedges;
    float **C = *CC;
    if (C != NULL) {
	storage = realloc(C[0], dim1 * dim2 * sizeof(A[0]));
	*CC = C = realloc(C, dim1 * sizeof(A));
    } else {
	storage = malloc(dim1 * dim2 * sizeof(A[0]));
	*CC = C = malloc(dim1 * sizeof(A));
    }

    for (i = 0; i < dim1; i++) {
	C[i] = storage;
	storage += dim2;
    }

    for (i = 0; i < dim1; i++) {
	edges = A[i].edges;
	ewgts = A[i].ewgts;
	nedges = A[i].nedges;
	for (j = 0; j < dim2; j++) {
	    sum = 0;
	    for (k = 0; k < nedges; k++) {
		sum += ewgts[k] * B[j][edges[k]];
	    }
	    C[i][j] = (float) (sum);
	}
    }
}



/* Copy a range of a double vector to a double vector */
void cpvec(double *copy, int beg, int end, double *vec)
{
    int i;

    copy = copy + beg;
    vec = vec + beg;
    for (i = end - beg + 1; i; i--) {
	*copy++ = *vec++;
    }
}

/* Returns scalar product of two double n-vectors. */
double dot(double *vec1, int beg, int end, double *vec2)
{
    int i;
    double sum;

    sum = 0.0;
    vec1 += beg;
    vec2 += beg;
    for (i = end - beg + 1; i; i--) {
	sum += (*vec1++) * (*vec2++);
    }
    return (sum);
}


/* Scaled add - fills double vec1 with vec1 + alpha*vec2 over range*/
void scadd(double *vec1, int beg, int end, double fac, double *vec2)
{
    int i;

    vec1 += beg;
    vec2 += beg;
    for (i = end - beg + 1; i; i--) {
	(*vec1++) += fac * (*vec2++);
    }
}

/* Scale - fills vec1 with alpha*vec2 over range, double version */
void vecscale(double *vec1, int beg, int end, double alpha, double *vec2)
{
    int i;

    vec1 += beg;
    vec2 += beg;
    for (i = end - beg + 1; i; i--) {
	(*vec1++) = alpha * (*vec2++);
    }
}

/* Returns 2-norm of a double n-vector over range. */
double norm(double *vec, int beg, int end)
{
    return sqrt(dot(vec, beg, end, vec));
}

void orthog1(int n, double *vec	/* vector to be orthogonalized against 1 */
    )
{
    int i;
    double *pntr;
    double sum;

    sum = 0.0;
    pntr = vec;
    for (i = n; i; i--) {
	sum += *pntr++;
    }
    sum /= n;
    pntr = vec;
    for (i = n; i; i--) {
	*pntr++ -= sum;
    }
}

#define RANGE 500

void init_vec_orth1(int n, double *vec)
{
    /* randomly generate a vector orthogonal to 1 (i.e., with mean 0) */
    int i;

    for (i = 0; i < n; i++)
	vec[i] = rand() % RANGE;

    orthog1(n, vec);
}

void
right_mult_with_vector(vtx_data * matrix, int n, double *vector,
		       double *result)
{
    int i, j;

    double res;
    for (i = 0; i < n; i++) {
	res = 0;
	for (j = 0; j < matrix[i].nedges; j++)
	    res += matrix[i].ewgts[j] * vector[matrix[i].edges[j]];
	result[i] = res;
    }
    /* orthog1(n,vector); */
}

void
right_mult_with_vector_f(float **matrix, int n, double *vector,
			 double *result)
{
    int i, j;

    double res;
    for (i = 0; i < n; i++) {
	res = 0;
	for (j = 0; j < n; j++)
	    res += matrix[i][j] * vector[j];
	result[i] = res;
    }
    /* orthog1(n,vector); */
}

void
vectors_subtraction(int n, double *vector1, double *vector2,
		    double *result)
{
    int i;
    for (i = 0; i < n; i++) {
	result[i] = vector1[i] - vector2[i];
    }
}

void
vectors_addition(int n, double *vector1, double *vector2, double *result)
{
    int i;
    for (i = 0; i < n; i++) {
	result[i] = vector1[i] + vector2[i];
    }
}

void
vectors_scalar_mult(int n, double *vector, double alpha, double *result)
{
    int i;
    for (i = 0; i < n; i++) {
	result[i] = vector[i] * alpha;
    }
}

void copy_vector(int n, double *source, double *dest)
{
    int i;
    for (i = 0; i < n; i++)
	dest[i] = source[i];
}

double vectors_inner_product(int n, double *vector1, double *vector2)
{
    int i;
    double result = 0;
    for (i = 0; i < n; i++) {
	result += vector1[i] * vector2[i];
    }

    return result;
}

double max_abs(int n, double *vector)
{
    double max_val = -1e50;
    int i;
    for (i = 0; i < n; i++)
	max_val = fmax(max_val, fabs(vector[i]));

    return max_val;
}

void
right_mult_with_vector_transpose(double **matrix,
				 int dim1, int dim2,
				 double *vector, double *result)
{
    /* matrix is dim2 x dim1, vector has dim2 components, result=matrix^T x vector */
    int i, j;

    double res;
    for (i = 0; i < dim1; i++) {
	res = 0;
	for (j = 0; j < dim2; j++)
	    res += matrix[j][i] * vector[j];
	result[i] = res;
    }
}

void
right_mult_with_vector_d(double **matrix,
			 int dim1, int dim2,
			 double *vector, double *result)
{
    /* matrix is dim1 x dim2, vector has dim2 components, result=matrix x vector */
    int i, j;

    double res;
    for (i = 0; i < dim1; i++) {
	res = 0;
	for (j = 0; j < dim2; j++)
	    res += matrix[i][j] * vector[j];
	result[i] = res;
    }
}


/*****************************
** Single precision (float) **
** version                  **
*****************************/

void orthog1f(int n, float *vec)
{
    int i;
    float *pntr;
    float sum;

    sum = 0.0;
    pntr = vec;
    for (i = n; i; i--) {
	sum += *pntr++;
    }
    sum /= n;
    pntr = vec;
    for (i = n; i; i--) {
	*pntr++ -= sum;
    }
}

void right_mult_with_vector_ff
    (float *packed_matrix, int n, float *vector, float *result) {
    /* packed matrix is the upper-triangular part of a symmetric matrix arranged in a vector row-wise */
    int i, j, index;
    float vector_i;

    float res;
    for (i = 0; i < n; i++) {
	result[i] = 0;
    }
    for (index = 0, i = 0; i < n; i++) {
	res = 0;
	vector_i = vector[i];
	/* deal with main diag */
	res += packed_matrix[index++] * vector_i;
	/* deal with off diag */
	for (j = i + 1; j < n; j++, index++) {
	    res += packed_matrix[index] * vector[j];
	    result[j] += packed_matrix[index] * vector_i;
	}
	result[i] += res;
    }
}

void
vectors_substractionf(int n, float *vector1, float *vector2, float *result)
{
    int i;
    for (i = 0; i < n; i++) {
	result[i] = vector1[i] - vector2[i];
    }
}

void
vectors_additionf(int n, float *vector1, float *vector2, float *result)
{
    int i;
    for (i = 0; i < n; i++) {
	result[i] = vector1[i] + vector2[i];
    }
}

void
vectors_mult_additionf(int n, float *vector1, float alpha, float *vector2)
{
    int i;
    for (i = 0; i < n; i++) {
	vector1[i] = vector1[i] + alpha * vector2[i];
    }
}

void vectors_scalar_multf(int n, float *vector, float alpha, float *result)
{
    int i;
    for (i = 0; i < n; i++) {
	result[i] = vector[i] * alpha;
    }
}

void copy_vectorf(int n, float *source, float *dest)
{
    int i;
    for (i = 0; i < n; i++)
	dest[i] = source[i];
}

double vectors_inner_productf(int n, float *vector1, float *vector2)
{
    int i;
    double result = 0;
    for (i = 0; i < n; i++) {
	result += vector1[i] * vector2[i];
    }

    return result;
}

void set_vector_val(int n, double val, double *result)
{
    int i;
    for (i = 0; i < n; i++)
	result[i] = val;
}

void set_vector_valf(int n, float val, float* result)
{
    int i;
    for (i = 0; i < n; i++)
	result[i] = val;
}

double max_absf(int n, float *vector)
{
    int i;
    float max_val = -1e30f;
    for (i = 0; i < n; i++)
	max_val = fmaxf(max_val, fabsf(vector[i]));

    return max_val;
}

void square_vec(int n, float *vec)
{
    int i;
    for (i = 0; i < n; i++) {
	vec[i] *= vec[i];
    }
}

void invert_vec(int n, float *vec)
{
    int i;
    for (i = 0; i < n; i++) {
	if (vec[i] != 0.0) {
	    vec[i] = 1.0f / vec[i];
	}
    }
}

void sqrt_vec(int n, float *vec)
{
    int i;
    for (i = 0; i < n; i++) {
	vec[i] = sqrtf(vec[i]);
    }
}

void sqrt_vecf(int n, float *source, float *target)
{
    int i;
    for (i = 0; i < n; i++) {
	if (source[i] >= 0.0) {
	    target[i] = sqrtf(source[i]);
	}
    }
}

void invert_sqrt_vec(int n, float *vec)
{
    int i;
    for (i = 0; i < n; i++) {
	if (vec[i] > 0.0) {
	    vec[i] = 1.0f / sqrtf(vec[i]);
	}
    }
}
