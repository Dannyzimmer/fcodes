/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/


/*
 * Break cycles in a directed graph by depth-first search.
 */

#include <dotgen/dot.h>
#include <stdbool.h>
#include <stddef.h>

void reverse_edge(edge_t * e)
{
    edge_t *f;

    delete_fast_edge(e);
    if ((f = find_fast_edge(aghead(e), agtail(e))))
	merge_oneway(e, f);
    else
	virtual_edge(aghead(e), agtail(e), e);
}

static void 
dfs(node_t * n)
{
    int i;
    edge_t *e;
    node_t *w;

    if (ND_mark(n))
	return;
    ND_mark(n) = TRUE;
    ND_onstack(n) = true;
    for (i = 0; (e = ND_out(n).list[i]); i++) {
	w = aghead(e);
	if (ND_onstack(w)) {
	    reverse_edge(e);
	    i--;
	} else {
	    if (!ND_mark(w))
		dfs(w);
	}
    }
    ND_onstack(n) = false;
}


void acyclic(graph_t * g)
{
    node_t *n;

    for (size_t c = 0; c < GD_comp(g).size; c++) {
	GD_nlist(g) = GD_comp(g).list[c];
	for (n = GD_nlist(g); n; n = ND_next(n))
	    ND_mark(n) = FALSE;
	for (n = GD_nlist(g); n; n = ND_next(n))
	    dfs(n);
    }
}

