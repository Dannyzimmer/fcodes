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

#ifdef __cplusplus
extern "C" {
#endif

#include "geom.h"
#include <sparse/SparseMatrix.h>
#include <stdbool.h>

#define DFLT_MARGIN     4   /* 4 points */

typedef enum {
    AM_NONE, AM_VOR,
    AM_SCALE, AM_NSCALE, AM_SCALEXY, AM_PUSH, AM_PUSHPULL,
    AM_ORTHO, AM_ORTHO_YX, AM_ORTHOXY, AM_ORTHOYX,
    AM_PORTHO, AM_PORTHO_YX, AM_PORTHOXY, AM_PORTHOYX, AM_COMPRESS,
    AM_VPSC, AM_IPSEP, AM_PRISM
} adjust_mode;

typedef struct {
    adjust_mode mode;
    char *print;
    int value;
    double scaling;
} adjust_data;

typedef struct {
    float x, y;
    bool doAdd;  /* if true, x and y are in points */
} expand_t;

    extern expand_t sepFactor(graph_t * G);
    extern expand_t esepFactor(graph_t * G);
    extern int adjustNodes(graph_t * G);
    extern int normalize(graph_t * g);
    extern int removeOverlapAs(graph_t*, char*);
    extern int removeOverlapWith(graph_t*, adjust_data*);
    extern int cAdjust(graph_t *, int);
    extern int scAdjust(graph_t *, int);
    extern adjust_data *graphAdjustMode(graph_t *G, adjust_data*, char* dflt);
    extern double *getSizes(Agraph_t * g, pointf pad, int *n_elabels, int **elabels);
    extern SparseMatrix makeMatrix(Agraph_t *g);

#ifdef __cplusplus
}
#endif
