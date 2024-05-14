/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include    <assert.h>
#include    <cgraph/list.h>
#include    <patchwork/patchwork.h>
#include    <limits.h>
#include    <neatogen/adjust.h>
#include    <pack/pack.h>
#include    <neatogen/neatoprocs.h>
#include    <stdbool.h>

/* the following code shamelessly copied from lib/fdpgen/layout.c
and should be extracted and made into a common function */

DEFINE_LIST(clist, graph_t*)

/* mkClusters:
 * Attach list of immediate child clusters.
 * NB: By convention, the indexing starts at 1.
 * If pclist is NULL, the graph is the root graph or a cluster
 * If pclist is non-NULL, we are recursively scanning a non-cluster
 * subgraph for cluster children.
 */
static void
mkClusters (graph_t * g, clist_t* pclist, graph_t* parent)
{
    graph_t* subg;
    clist_t  list = {0};
    clist_t* clist;

    if (pclist == NULL) {
        // [0] is empty. The clusters are in [1..cnt].
        clist_append(&list, NULL);
        clist = &list;
    }
    else
        clist = pclist;

    for (subg = agfstsubg(g); subg; subg = agnxtsubg(subg)) {
        if (!strncmp(agnameof(subg), "cluster", 7)) {
	    agbindrec(subg, "Agraphinfo_t", sizeof(Agraphinfo_t), true);
#ifdef FDP_GEN
            GD_alg(subg) = gv_alloc(sizeof(gdata)); /* freed in cleanup_subgs */
            GD_ndim(subg) = GD_ndim(parent);
            LEVEL(subg) = LEVEL(parent) + 1;
            GPARENT(subg) = parent;
#endif
            clist_append(clist, subg);
            mkClusters(subg, NULL, subg);
        }
        else {
            mkClusters(subg, clist, parent);
        }
    }
    if (pclist == NULL) {
        assert(clist_size(&list) - 1 <= INT_MAX);
        GD_n_cluster(g) = (int)(clist_size(&list) - 1);
        if (clist_size(&list) > 1) {
            clist_shrink_to_fit(&list);
            GD_clust(g) = clist_detach(&list);
        } else {
            clist_free(&list);
        }
    }
}

static void patchwork_init_node(node_t * n)
{
    agset(n,"shape","box");
    /* common_init_node_opt(n,FALSE); */
}

static void patchwork_init_edge(edge_t * e)
{
    agbindrec(e, "Agedgeinfo_t", sizeof(Agnodeinfo_t), true);  // edge custom data
    /* common_init_edge(e); */
}

static void patchwork_init_node_edge(graph_t * g)
{
    node_t *n;
    edge_t *e;
    int i = 0;
    rdata* alg = gv_calloc(agnnodes(g), sizeof(rdata));

    GD_neato_nlist(g) = gv_calloc(agnnodes(g) + 1, sizeof(node_t*));
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	agbindrec(n, "Agnodeinfo_t", sizeof(Agnodeinfo_t), true);  // node custom data
	ND_alg(n) = alg + i;
	GD_neato_nlist(g)[i++] = n;
	patchwork_init_node(n);

	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    patchwork_init_edge(e);
	}
    }
}

static void patchwork_init_graph(graph_t * g)
{
    N_shape = agattr(g, AGNODE, "shape","box");
    setEdgeType (g, EDGETYPE_LINE);
    /* GD_ndim(g) = late_int(g,agfindattr(g,"dim"),2,2); */
    Ndim = GD_ndim(g) = 2;	/* The algorithm only makes sense in 2D */
    mkClusters(g, NULL, g);
    patchwork_init_node_edge(g);
}

/* patchwork_layout:
 * The current version makes no use of edges, neither for a notion of connectivity
 * nor during drawing.
 */
void patchwork_layout(Agraph_t *g)
{
    patchwork_init_graph(g);

    if ((agnnodes(g) == 0) && (GD_n_cluster(g) == 0)) return;

    patchworkLayout (g);

    dotneato_postprocess(g);
}

static void patchwork_cleanup_graph(graph_t * g)
{
    free(GD_neato_nlist(g));
    free(GD_clust(g));
}

void patchwork_cleanup(graph_t * g)
{
    node_t *n;
    edge_t *e;

    n = agfstnode(g);
    if (!n) return;
    free (ND_alg(n));
    for (; n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    gv_cleanup_edge(e);
	}
	gv_cleanup_node(n);
    }
    patchwork_cleanup_graph(g);
}

/**
 * @dir lib/patchwork
 * @brief squarified [treemap](https://en.wikipedia.org/wiki/Treemapping) layout engine, API patchwork/patchwork.h
 * @ingroup engines
 *
 * [Patchwork layout user manual](https://graphviz.org/docs/layouts/patchwork/)
 *
 * Other @ref engines
 */
