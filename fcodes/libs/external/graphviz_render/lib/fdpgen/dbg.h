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

#ifdef DEBUG

#include <fdpgen/fdp.h>
#include <stdio.h>
#include <cgraph/cgraph.h>

    extern void outputGraph(Agraph_t *, FILE *, int);

    extern void incInd(void);
    extern void decInd(void);
    extern void prIndent(void);

    extern void dump(graph_t *g, int doAll);
    extern void dumpG(graph_t * g, char *fname, int);

#endif

#ifdef __cplusplus
}
#endif
