/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <sparse/SparseMatrix.h>
#include "mmio.h"
#include "matrix_market.h"
#include <cgraph/alloc.h>
#include <cgraph/unreachable.h>
#include <assert.h>

static int mm_get_type(MM_typecode typecode)
{
    if (mm_is_complex(typecode)) {
	return MATRIX_TYPE_COMPLEX;
    } else if (mm_is_real(typecode)) {
	return MATRIX_TYPE_REAL;
    } else if (mm_is_integer(typecode)) {
	return MATRIX_TYPE_INTEGER;
    } else if (mm_is_pattern(typecode)) {
	return MATRIX_TYPE_PATTERN;
    }
    return MATRIX_TYPE_UNKNOWN;
}

SparseMatrix SparseMatrix_import_matrix_market(FILE * f)
{
    int ret_code, type;
    MM_typecode matcode;
    double *val = NULL, *v;
    int *vali = NULL, i, m, n, *I = NULL, *J = NULL, nz;
    void *vp = NULL;
    SparseMatrix A = NULL;
    int nzold;
    int c;

    if ((c = fgetc(f)) != '%') {
	ungetc(c, f);
	return NULL;
    }
    ungetc(c, f);
    if (mm_read_banner(f, &matcode) != 0) {
#ifdef DEBUG
	printf("Could not process Matrix Market banner.\n");
#endif
	return NULL;
    }


    /*  This is how one can screen matrix types if their application */
    /*  only supports a subset of the Matrix Market data types.      */

    if (!mm_is_matrix(matcode) || !mm_is_sparse(matcode)) {
	UNREACHABLE();
    }

    /* find out size of sparse matrix .... */
    if ((ret_code = mm_read_mtx_crd_size(f, &m, &n, &nz)) != 0) {
	assert(0);
	return NULL;
    }
    /* reseve memory for matrices */

    I = gv_calloc(nz, sizeof(int));
    J = gv_calloc(nz, sizeof(int));

	/* NOTE: when reading in doubles, ANSI C requires the use of the "l"  */
	/*   specifier as in "%lg", "%lf", "%le", otherwise errors will occur */
	/*  (ANSI C X3.159-1989, Sec. 4.9.6.2, p. 136 lines 13-15)            */
	type = mm_get_type(matcode);
	switch (type) {
	case MATRIX_TYPE_REAL:
	    val = gv_calloc(nz, sizeof(double));
	    for (i = 0; i < nz; i++) {
		int num = fscanf(f, "%d %d %lg\n", &I[i], &J[i], &val[i]);
		(void)num;
		assert(num == 3);
		I[i]--;		/* adjust from 1-based to 0-based */
		J[i]--;
	    }
	    if (mm_is_symmetric(matcode)) {
		I = gv_recalloc(I, nz, 2 * nz, sizeof(int));
		J = gv_recalloc(J, nz, 2 * nz, sizeof(int));
		val = gv_recalloc(val, nz, 2 * nz, sizeof(double));
		nzold = nz;
		for (i = 0; i < nzold; i++) {
		    if (I[i] != J[i]) {
			I[nz] = J[i];
			J[nz] = I[i];
			val[nz++] = val[i];
		    }
		}
	    } else if (mm_is_skew(matcode)) {
		I = gv_recalloc(I, nz, 2 * nz, sizeof(int));
		J = gv_recalloc(J, nz, 2 * nz, sizeof(int));
		val = gv_recalloc(val, nz, 2 * nz, sizeof(double));
		nzold = nz;
		for (i = 0; i < nzold; i++) {
		    assert(I[i] != J[i]);	/* skew symm has no diag */
		    I[nz] = J[i];
		    J[nz] = I[i];
		    val[nz++] = -val[i];
		}
	    } else {
		assert(!mm_is_hermitian(matcode));
	    }
	    vp = val;
	    break;
	case MATRIX_TYPE_INTEGER:
	    vali = gv_calloc(nz, sizeof(int));
	    for (i = 0; i < nz; i++) {
		int num = fscanf(f, "%d %d %d\n", &I[i], &J[i], &vali[i]);
		(void)num;
		assert(num == 3);
		I[i]--;		/* adjust from 1-based to 0-based */
		J[i]--;
	    }
	    if (mm_is_symmetric(matcode)) {
		I = gv_recalloc(I, nz, 2 * nz, sizeof(int));
		J = gv_recalloc(J, nz, 2 * nz, sizeof(int));
		vali = gv_recalloc(vali, nz, 2 * nz, sizeof(int));
		nzold = nz;
		for (i = 0; i < nzold; i++) {
		    if (I[i] != J[i]) {
			I[nz] = J[i];
			J[nz] = I[i];
			vali[nz++] = vali[i];
		    }
		}
	    } else if (mm_is_skew(matcode)) {
		I = gv_recalloc(I, nz, 2 * nz, sizeof(int));
		J = gv_recalloc(J, nz, 2 * nz, sizeof(int));
		vali = gv_recalloc(vali, nz, 2 * nz, sizeof(int));
		nzold = nz;
		for (i = 0; i < nzold; i++) {
		    assert(I[i] != J[i]);	/* skew symm has no diag */
		    I[nz] = J[i];
		    J[nz] = I[i];
		    vali[nz++] = -vali[i];
		}
	    } else {
		assert(!mm_is_hermitian(matcode));
	    }
	    vp = vali;
	    break;
	case MATRIX_TYPE_PATTERN:
	    for (i = 0; i < nz; i++) {
		int num = fscanf(f, "%d %d\n", &I[i], &J[i]);
		(void)num;
		assert(num == 2);
		I[i]--;		/* adjust from 1-based to 0-based */
		J[i]--;
	    }
	    if (mm_is_symmetric(matcode) || mm_is_skew(matcode)) {
		I = gv_recalloc(I, nz, 2 * nz, sizeof(int));
		J = gv_recalloc(J, nz, 2 * nz, sizeof(int));
		nzold = nz;
		for (i = 0; i < nzold; i++) {
		    if (I[i] != J[i]) {
			I[nz] = J[i];
			J[nz++] = I[i];
		    }
		}
	    } else {
		assert(!mm_is_hermitian(matcode));
	    }
	    break;
	case MATRIX_TYPE_COMPLEX:
	    val = gv_calloc(2 * nz, sizeof(double));
	    v = val;
	    for (i = 0; i < nz; i++) {
		int num = fscanf(f, "%d %d %lg %lg\n", &I[i], &J[i], &v[0], &v[1]);
		(void)num;
		assert(num == 4);
		v += 2;
		I[i]--;		/* adjust from 1-based to 0-based */
		J[i]--;
	    }
	    if (mm_is_symmetric(matcode)) {
		I = gv_recalloc(I, nz, 2 * nz, sizeof(int));
		J = gv_recalloc(J, nz, 2 * nz, sizeof(int));
		val = gv_recalloc(val, 2 * nz, 4 * nz, sizeof(double));
		nzold = nz;
		for (i = 0; i < nzold; i++) {
		    if (I[i] != J[i]) {
			I[nz] = J[i];
			J[nz] = I[i];
			val[2 * nz] = val[2 * i];
			val[2 * nz + 1] = val[2 * i + 1];
			nz++;
		    }
		}
	    } else if (mm_is_skew(matcode)) {
		I = gv_recalloc(I, nz, 2 * nz, sizeof(int));
		J = gv_recalloc(J, nz, 2 * nz, sizeof(int));
		val = gv_recalloc(val, 2 * nz, 4 * nz, sizeof(double));
		nzold = nz;
		for (i = 0; i < nzold; i++) {
		    assert(I[i] != J[i]);	/* skew symm has no diag */
		    I[nz] = J[i];
		    J[nz] = I[i];
		    val[2 * nz] = -val[2 * i];
		    val[2 * nz + 1] = -val[2 * i + 1];
		    nz++;

		}
	    } else if (mm_is_hermitian(matcode)) {
		I = gv_recalloc(I, nz, 2 * nz, sizeof(int));
		J = gv_recalloc(J, nz, 2 * nz, sizeof(int));
		val = gv_recalloc(val, 2 * nz, 4 * nz, sizeof(double));
		nzold = nz;
		for (i = 0; i < nzold; i++) {
		    if (I[i] != J[i]) {
			I[nz] = J[i];
			J[nz] = I[i];
			val[2 * nz] = val[2 * i];
			val[2 * nz + 1] = -val[2 * i + 1];
			nz++;
		    }
		}
	    }
	    vp = val;
	    break;
	default:
	    return 0;
	}

	A = SparseMatrix_from_coordinate_arrays(nz, m, n, I, J, vp,
						    type, sizeof(double));
    free(I);
    free(J);
    free(val);

    if (mm_is_symmetric(matcode)) {
	SparseMatrix_set_symmetric(A);
	SparseMatrix_set_pattern_symmetric(A);
    }


    return A;
}
