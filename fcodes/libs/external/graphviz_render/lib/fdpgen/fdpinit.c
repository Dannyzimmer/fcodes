/**
 * @file
 * @brief API fdpgen/fdp.h: @ref fdp_init_node_edge, @ref fdp_cleanup
 */

/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/


/* fdpinit.c:
 * Written by Emden R. Gansner
 *
 * Mostly boilerplate initialization and cleanup code.
 */

/* uses PRIVATE interface */
#define FDP_PRIVATE 1

#include    <cgraph/alloc.h>
#include    <fdpgen/tlayout.h>
#include    <neatogen/neatoprocs.h>
#include    <cgraph/agxbuf.h>
#include    <stdbool.h>

static void initialPositions(graph_t * g)
{
    int i;
    node_t *np;
    attrsym_t *possym;
    attrsym_t *pinsym;
    double *pvec;
    char *p;
    char c;

    possym = agattr(g,AGNODE, "pos", NULL);
    if (!possym)
	return;
    pinsym = agattr(g,AGNODE, "pin", NULL);
    for (i = 0; (np = GD_neato_nlist(g)[i]); i++) {
	p = agxget(np, possym);
	if (p[0]) {
	    pvec = ND_pos(np);
	    c = '\0';
	    if (sscanf(p, "%lf,%lf%c", pvec, pvec + 1, &c) >= 2) {
		if (PSinputscale > 0.0) {
		    int j;
		    for (j = 0; j < NDIM; j++)
			pvec[j] = pvec[j] / PSinputscale;
		}
		ND_pinned(np) = P_SET;
		if (c == '!'
		    || (pinsym && mapbool(agxget(np, pinsym))))
		    ND_pinned(np) = P_PIN;
	    } else
		fprintf(stderr,
			"Warning: node %s, position %s, expected two floats\n",
			agnameof(np), p);
	}
    }
}

/* init_edge:
 */
static void init_edge(edge_t * e, attrsym_t * E_len)
{
    agbindrec(e, "Agedgeinfo_t", sizeof(Agedgeinfo_t), true);	//node custom data
    ED_factor(e) = late_double(e, E_weight, 1.0, 0.0);
    ED_dist(e) = late_double(e, E_len, fdp_parms->K, 0.0);

    common_init_edge(e);
}

static void init_node(node_t * n)
{
    common_init_node(n);
    ND_pos(n) = gv_calloc(GD_ndim(agraphof(n)), sizeof(double));
    gv_nodesize(n, GD_flip(agraphof(n)));
}

void fdp_init_node_edge(graph_t * g)
{
    attrsym_t *E_len;
    node_t *n;
    edge_t *e;
    int nn;
    int i;

    aginit(g, AGNODE, "Agnodeinfo_t", sizeof(Agnodeinfo_t), true);
    processClusterEdges(g);

    /* Get node count after processClusterEdges(), as this function may
     * add new nodes.
     */
    nn = agnnodes(g);
    GD_neato_nlist(g) = gv_calloc(nn + 1, sizeof(node_t*));

    for (i = 0, n = agfstnode(g); n; n = agnxtnode(g, n)) {
	init_node (n);
	GD_neato_nlist(g)[i] = n;
	ND_id(n) = i++;
    }

    E_len = agattr(g,AGEDGE, "len", NULL);
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    init_edge(e, E_len);
	}
    }
    initialPositions(g);
}

static void cleanup_subgs(graph_t * g)
{
    graph_t *subg;
    int i;

    for (i = 1; i <= GD_n_cluster(g); i++) {
	subg = GD_clust(g)[i];
	free_label(GD_label(subg));
	if (GD_alg(subg)) {
	    free(PORTS(subg));
	    free(GD_alg(subg));
	}
	cleanup_subgs(subg);
    }
    free (GD_clust(g));
}

static void fdp_cleanup_graph(graph_t * g)
{
    cleanup_subgs(g);
    free(GD_neato_nlist(g));
    free(GD_alg(g));
}

void fdp_cleanup(graph_t * g)
{
    node_t *n;
    edge_t *e;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    gv_cleanup_edge(e);
	}
	gv_cleanup_node(n);
    }
    fdp_cleanup_graph(g);
}

/**
 * @dir lib/fdpgen
 * @brief [Force-Directed Placement](https://en.wikipedia.org/wiki/Force-directed_graph_drawing) layout engine, API fdpgen/fdp.h
 * @ingroup engines
 *
 * [FDP layout user manual](https://graphviz.org/docs/layouts/fdp/)
 *
 * Other @ref engines
 */
