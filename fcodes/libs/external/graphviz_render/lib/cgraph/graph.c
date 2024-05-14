/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <assert.h>
#include <cgraph/alloc.h>
#include <cgraph/cghdr.h>
#include <stdbool.h>
#include <stdlib.h>

Agraph_t *Ag_G_global;

/*
 * this code sets up the resource management discipline
 * and returns a new main graph struct.
 */
static Agclos_t *agclos(Agdisc_t * proto)
{
    Agclos_t *rv;

    /* establish an allocation arena */
    rv = gv_calloc(1, sizeof(Agclos_t));
    rv->disc.id = ((proto && proto->id) ? proto->id : &AgIdDisc);
    rv->disc.io = ((proto && proto->io) ? proto->io : &AgIoDisc);
    return rv;
}

/*
 * Open a new main graph with the given descriptor (directed, strict, etc.)
 */
Agraph_t *agopen(char *name, Agdesc_t desc, Agdisc_t * arg_disc)
{
    Agraph_t *g;
    Agclos_t *clos;
    IDTYPE gid;

    clos = agclos(arg_disc);
    g = gv_calloc(1, sizeof(Agraph_t));
    AGTYPE(g) = AGRAPH;
    g->clos = clos;
    g->desc = desc;
    g->desc.maingraph = true;
    g->root = g;
    g->clos->state.id = g->clos->disc.id->open(g, arg_disc);
    if (agmapnametoid(g, AGRAPH, name, &gid, true))
	AGID(g) = gid;
    g = agopen1(g);
    agregister(g, AGRAPH, g);
    return g;
}

/* 
 * initialize dictionaries, set seq, invoke init method of new graph
 */
Agraph_t *agopen1(Agraph_t * g)
{
    Agraph_t *par;

    g->n_seq = agdtopen(g, &Ag_subnode_seq_disc, Dttree);
    g->n_id = agdtopen(g, &Ag_subnode_id_disc, Dttree);
    g->e_seq = agdtopen(g, g == agroot(g)? &Ag_mainedge_seq_disc : &Ag_subedge_seq_disc, Dttree);
    g->e_id = agdtopen(g, g == agroot(g)? &Ag_mainedge_id_disc : &Ag_subedge_id_disc, Dttree);
    g->g_dict = agdtopen(g, &Ag_subgraph_id_disc, Dttree);

    par = agparent(g);
    if (par) {
	uint64_t seq = agnextseq(par, AGRAPH);
	assert((seq & SEQ_MASK) == seq && "sequence ID overflow");
	AGSEQ(g) = seq & SEQ_MASK;
	dtinsert(par->g_dict, g);
    }
    if (!par || par->desc.has_attrs)
	agraphattr_init(g);
    agmethod_init(g, g);
    return g;
}

/*
 * Close a graph or subgraph, freeing its storage.
 */
int agclose(Agraph_t * g)
{
    Agraph_t *subg, *next_subg, *par;
    Agnode_t *n, *next_n;

    par = agparent(g);

    for (subg = agfstsubg(g); subg; subg = next_subg) {
	next_subg = agnxtsubg(subg);
	agclose(subg);
    }

    for (n = agfstnode(g); n; n = next_n) {
	next_n = agnxtnode(g, n);
	agdelnode(g, n);
    }

    aginternalmapclose(g);
    agmethod_delete(g, g);

    assert(dtsize(g->n_id) == 0);
    if (agdtclose(g, g->n_id)) return FAILURE;
    assert(dtsize(g->n_seq) == 0);
    if (agdtclose(g, g->n_seq)) return FAILURE;

    assert(dtsize(g->e_id) == 0);
    if (agdtclose(g, g->e_id)) return FAILURE;
    assert(dtsize(g->e_seq) == 0);
    if (agdtclose(g, g->e_seq)) return FAILURE;

    assert(dtsize(g->g_dict) == 0);
    if (agdtclose(g, g->g_dict)) return FAILURE;

    if (g->desc.has_attrs)
	if (agraphattr_delete(g)) return FAILURE;
    agrecclose((Agobj_t *) g);
    agfreeid(g, AGRAPH, AGID(g));

    if (par) {
	agdelsubg(par, g);
	agfree(par, g);
    } else {
	void *clos;
	while (g->clos->cb)
	    agpopdisc(g, g->clos->cb->f);
	AGDISC(g, id)->close(AGCLOS(g, id));
	if (agstrclose(g)) return FAILURE;
	clos = g->clos;
	free(g);
	free(clos);
    }
    return SUCCESS;
}

uint64_t agnextseq(Agraph_t * g, int objtype)
{
    return ++(g->clos->seq[objtype]);
}

int agnnodes(Agraph_t * g)
{
    return dtsize(g->n_id);
}

int agnedges(Agraph_t * g)
{
    Agnode_t *n;
    int rv = 0;

    for (n = agfstnode(g); n; n = agnxtnode(g, n))
	rv += agdegree(g, n, 0, 1);	/* must use OUT to get self-arcs */
    return rv;
}

int agnsubg(Agraph_t * g)
{
	return dtsize(g->g_dict);
}

int agisdirected(Agraph_t * g)
{
    return g->desc.directed;
}

int agisundirected(Agraph_t * g)
{
    return !agisdirected(g);
}

int agisstrict(Agraph_t * g)
{
    return g->desc.strict;
}

int agissimple(Agraph_t * g)
{
    return (g->desc.strict && g->desc.no_loop);
}

static int cnt(Dict_t * d, Dtlink_t ** set)
{
	int rv;
    dtrestore(d, *set);
    rv = dtsize(d);
    *set = dtextract(d);
	return rv;
}

int agcountuniqedges(Agraph_t * g, Agnode_t * n, int want_in, int want_out)
{
    Agedge_t *e;
    Agsubnode_t *sn;
    int rv = 0;

    sn = agsubrep(g, n);
    if (want_out) rv = cnt(g->e_seq,&(sn->out_seq));
    if (want_in) {
		if (!want_out) rv += cnt(g->e_seq,&(sn->in_seq));	/* cheap */
		else {	/* less cheap */
			for (e = agfstin(g, n); e; e = agnxtin(g, e))
				if (e->node != n) rv++;  /* don't double count loops */
		}
    }
    return rv;
}

int agdegree(Agraph_t * g, Agnode_t * n, int want_in, int want_out)
{
    Agsubnode_t *sn;
    int rv = 0;

    sn = agsubrep(g, n);
    if (sn) {
	if (want_out) rv += cnt(g->e_seq,&(sn->out_seq));
	if (want_in) rv += cnt(g->e_seq,&(sn->in_seq));
    }
	return rv;
}

static int agraphidcmpf(Dict_t * d, void *arg0, void *arg1, Dtdisc_t * disc)
{
    (void)d; /* unused */
    (void)disc; /* unused */
    Agraph_t *sg0 = arg0;
    Agraph_t *sg1 = arg1;
    if (AGID(sg0) < AGID(sg1)) {
	return -1;
    }
    if (AGID(sg0) > AGID(sg1)) {
	return 1;
    }
    return 0;
}

Dtdisc_t Ag_subgraph_id_disc = {
    .link = offsetof(Agraph_t, link), // link offset
    .comparf = agraphidcmpf,
    .memoryf = agdictobjmem,
};

Agdesc_t Agdirected = {.directed = true, .maingraph = true};
Agdesc_t Agstrictdirected = {.directed = true, .strict = true, .maingraph = true};
Agdesc_t Agundirected = {.maingraph = true};
Agdesc_t Agstrictundirected = {.strict = true, .maingraph = true};

Agdisc_t AgDefaultDisc = { &AgIdDisc, &AgIoDisc };

/**
 * @dir lib/cgraph
 * @brief abstract graph C library, API cgraph.h
 *
 * [man 3 cgraph](https://graphviz.org/pdf/cgraph.3.pdf)
 */
