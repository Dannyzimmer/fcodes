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

#include <neatogen/defs.h>
#include <neatogen/sgd.h>

    extern void dijkstra(int, vtx_data *, int, DistType *);
    extern void dijkstra_f(int, vtx_data *, int, float *);

    /* Dijkstra bounded to nodes in *unweighted* radius */
    extern int dijkstra_bounded(int, vtx_data *, int, DistType *, int,
				int *);
    extern int dijkstra_sgd(graph_sgd *, int, term_sgd *);

#ifdef __cplusplus
}
#endif
