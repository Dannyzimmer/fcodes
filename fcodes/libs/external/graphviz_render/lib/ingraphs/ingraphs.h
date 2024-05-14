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

/* The ingraphs library works with libcgraph with all user-supplied data. For
 * this to work, the include file relies upon its context to supply a definition
 * of Agraph_t.
 */

#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct {
	union {
	    char**     Files;
	    Agraph_t** Graphs;
	} u;
	int ctr;
	int ingraphs;
	void *fp;
	Agraph_t *(*readf)(void*);
	bool heap;
	unsigned errors;
    } ingraph_state;

    extern ingraph_state *newIngraph(ingraph_state *, char **);
    extern ingraph_state *newIng(ingraph_state *, char **, Agraph_t *(*readf)(void*));
    extern ingraph_state *newIngGraphs(ingraph_state *, Agraph_t**, Agraph_t *(*readf)(void*));
    extern void closeIngraph(ingraph_state * sp);
    extern Agraph_t *nextGraph(ingraph_state *);
    extern char *fileName(ingraph_state *);

#ifdef __cplusplus
}
#endif
