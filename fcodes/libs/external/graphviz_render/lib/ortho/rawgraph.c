/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

 /* Implements graph.h  */

#include "config.h"
#include <cgraph/alloc.h>
#include <cgraph/list.h>
#include <ortho/rawgraph.h>
#include <common/intset.h>
#include <stdbool.h>
#include <stddef.h>

#define UNSCANNED 0
#define SCANNING  1
#define SCANNED   2

rawgraph*
make_graph(int n)
{
    int i;
    rawgraph* g = gv_alloc(sizeof(rawgraph));
    g->nvs = n;
    g->vertices = gv_calloc(n, sizeof(vertex));
    for(i=0;i<n;i++) {
        g->vertices[i].adj_list = openIntSet ();
        g->vertices[i].color = UNSCANNED;
    }
    return g;
}

void
free_graph(rawgraph* g)
{
    int i;
    for(i=0;i<g->nvs;i++)
        dtclose(g->vertices[i].adj_list);
    free (g->vertices);
    free (g);
}
 
void 
insert_edge(rawgraph* g, int v1, int v2)
{
    intitem obj;

    obj.id = v2;
    dtinsert(g->vertices[v1].adj_list,&obj);
}

void
remove_redge(rawgraph* g, int v1, int v2)
{
    intitem obj;
    obj.id = v2;
    dtdelete (g->vertices[v1].adj_list, &obj);
    obj.id = v1;
    dtdelete (g->vertices[v2].adj_list, &obj);
}

bool
edge_exists(rawgraph* g, int v1, int v2)
{
    return dtmatch (g->vertices[v1].adj_list, &v2) != 0;
}

DEFINE_LIST(int_stack, int)

static int DFS_visit(rawgraph *g, int v, int time, int_stack_t *sp) {
    Dt_t* adj;
    Dtlink_t* link;
    int id;
    vertex* vp;

    vp = g->vertices + v;
    vp->color = SCANNING;
    adj = vp->adj_list;
    time = time + 1;

    for(link = dtflatten (adj); link; link = dtlink(adj,link)) {
        id = ((intitem*)dtobj(adj,link))->id;
        if(g->vertices[id].color == UNSCANNED)
            time = DFS_visit(g, id, time, sp);
    }
    vp->color = SCANNED;
    int_stack_push(sp, v);
    return time + 1;
}

void
top_sort(rawgraph* g)
{
    int i;
    int time = 0;
    int count = 0;

    if (g->nvs == 0) return;
    if (g->nvs == 1) {
        g->vertices[0].topsort_order = count;
		return;
	}

    int_stack_t sp = {0};
    int_stack_reserve(&sp, (size_t)g->nvs);
    for(i=0;i<g->nvs;i++) {
        if(g->vertices[i].color == UNSCANNED)
            time = DFS_visit(g, i, time, &sp);
    }
    while (!int_stack_is_empty(&sp)) {
        int v = int_stack_pop(&sp);
        g->vertices[v].topsort_order = count;
        count++;
    }
    int_stack_free(&sp);
}
