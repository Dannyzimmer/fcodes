/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/


/*  vladimir@cs.ualberta.ca,  9-Dec-1997
 *	merge edges with specified samehead/sametail onto the same port
 */

#include <cgraph/list.h>
#include <math.h>
#include	<dotgen/dot.h>
#include	<stdbool.h>
#include	<stddef.h>

DEFINE_LIST(edge_list, edge_t*)

typedef struct same_t {
    char *id;			/* group id */
    edge_list_t l; // edges in the group
} same_t;

static void free_same(same_t s) {
  edge_list_free(&s.l);
}

DEFINE_LIST_WITH_DTOR(same_list, same_t, free_same)

static void sameedge(same_list_t *same, edge_t *e, char *id);
static void sameport(node_t *u, edge_list_t l);

void dot_sameports(graph_t * g)
/* merge edge ports in G */
{
    node_t *n;
    edge_t *e;
    char *id;
    same_list_t samehead = {0};
    same_list_t sametail = {0};

    E_samehead = agattr(g, AGEDGE, "samehead", NULL);
    E_sametail = agattr(g, AGEDGE, "sametail", NULL);
    if (!(E_samehead || E_sametail))
	return;
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstedge(g, n); e; e = agnxtedge(g, e, n)) {
	    if (aghead(e) == agtail(e)) continue;  /* Don't support same* for loops */
	    if (aghead(e) == n && E_samehead &&
	        (id = agxget(e, E_samehead))[0])
		sameedge(&samehead, e, id);
	    else if (agtail(e) == n && E_sametail &&
	        (id = agxget(e, E_sametail))[0])
		sameedge(&sametail, e, id);
	}
	for (size_t i = 0; i < same_list_size(&samehead); i++) {
	    if (edge_list_size(&same_list_at(&samehead, i)->l) > 1)
		sameport(n, same_list_get(&samehead, i).l);
	}
	same_list_clear(&samehead);
	for (size_t i = 0; i < same_list_size(&sametail); i++) {
	    if (edge_list_size(&same_list_at(&sametail, i)->l) > 1)
		sameport(n, same_list_get(&sametail, i).l);
	}
	same_list_clear(&sametail);
    }

    same_list_free(&samehead);
    same_list_free(&sametail);
}

/// register \p e in the \p same structure of the originating node under \p id
static void sameedge(same_list_t *same, edge_t *e, char *id) {
    for (size_t i = 0; i < same_list_size(same); i++)
	if (streq(same_list_get(same, i).id, id)) {
	    edge_list_append(&same_list_at(same, i)->l, e);
	    return;
	}

    same_t to_append = {.id = id};
    edge_list_append(&to_append.l, e);
    same_list_append(same, to_append);
}

static void sameport(node_t *u, edge_list_t l)
/* make all edges in L share the same port on U. The port is placed on the
   node boundary and the average angle between the edges. FIXME: this assumes
   naively that the edges are straight lines, which is wrong if they are long.
   In that case something like concentration could be done.

   An arr_port is also computed that's ARR_LEN away from the node boundary.
   It's used for edges that don't themselves have an arrow.
*/
{
    node_t *v;
    edge_t *f;
    double x = 0, y = 0, x1, y1, x2, y2, r;
    port prt;

    /* Compute the direction vector (x,y) of the average direction. We compute
       with direction vectors instead of angles because else we have to first
       bring the angles within PI of each other. av(a,b)!=av(a,b+2*PI) */
    for (size_t i = 0; i < edge_list_size(&l); i++) {
	edge_t *e = edge_list_get(&l, i);
	if (aghead(e) == u)
	    v = agtail(e);
	else
	    v = aghead(e);
	x1 = ND_coord(v).x - ND_coord(u).x;
	y1 = ND_coord(v).y - ND_coord(u).y;
	r = hypot(x1, y1);
	x += x1 / r;
	y += y1 / r;
    }
    r = hypot(x, y);
    x /= r;
    y /= r;

    /* (x1,y1),(x2,y2) is a segment that must cross the node boundary */
    x1 = ND_coord(u).x;
    y1 = ND_coord(u).y;	/* center of node */
    r = MAX(ND_lw(u) + ND_rw(u), ND_ht(u) + GD_ranksep(agraphof(u)));	/* far away */
    x2 = x * r + ND_coord(u).x;
    y2 = y * r + ND_coord(u).y;
    {				/* now move (x1,y1) to the node boundary */
	pointf curve[4];		/* bezier control points for a straight line */
	curve[0].x = x1;
	curve[0].y = y1;
	curve[1].x = (2 * x1 + x2) / 3;
	curve[1].y = (2 * y1 + y2) / 3;
	curve[2].x = (2 * x2 + x1) / 3;
	curve[2].y = (2 * y2 + y1) / 3;
	curve[3].x = x2;
	curve[3].y = y2;

	shape_clip(u, curve);
	x1 = curve[0].x - ND_coord(u).x;
	y1 = curve[0].y - ND_coord(u).y;
    }

    /* compute PORT on the boundary */
    prt.p.x = ROUND(x1);
    prt.p.y = ROUND(y1);
    prt.bp = 0;
    prt.order =
	(MC_SCALE * (ND_lw(u) + prt.p.x)) / (ND_lw(u) + ND_rw(u));
    prt.constrained = false;
    prt.defined = true;
    prt.clip = false;
    prt.dyna = false;
    prt.theta = 0;
    prt.side = 0;
    prt.name = NULL;

    /* assign one of the ports to every edge */
    for (size_t i = 0; i < edge_list_size(&l); i++) {
	edge_t *e = edge_list_get(&l, i);
	for (; e; e = ED_to_virt(e)) {	/* assign to all virt edges of e */
	    for (f = e; f;
		 f = ED_edge_type(f) == VIRTUAL &&
		 ND_node_type(aghead(f)) == VIRTUAL &&
		 ND_out(aghead(f)).size == 1 ?
		 ND_out(aghead(f)).list[0] : NULL) {
		if (aghead(f) == u)
		    ED_head_port(f) = prt;
		if (agtail(f) == u)
		    ED_tail_port(f) = prt;
	    }
	    for (f = e; f;
		 f = ED_edge_type(f) == VIRTUAL &&
		 ND_node_type(agtail(f)) == VIRTUAL &&
		 ND_in(agtail(f)).size == 1 ?
		 ND_in(agtail(f)).list[0] : NULL) {
		if (aghead(f) == u)
		    ED_head_port(f) = prt;
		if (agtail(f) == u)
		    ED_tail_port(f) = prt;
	    }
	}
    }

    ND_has_port(u) = true;	/* kinda pointless, because mincross is already done */
}
