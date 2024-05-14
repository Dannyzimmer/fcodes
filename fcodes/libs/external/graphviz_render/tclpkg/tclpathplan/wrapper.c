/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <cgraph/alloc.h>
#include <stdio.h>
#include <stdlib.h>
#include "simple.h"

/* this is all out of pathgeom.h and will eventually get included */

typedef struct Pxy_t {
    double x, y;
} Pxy_t;

typedef struct Pxy_t Ppoint_t;
typedef struct Pxy_t Pvector_t;

typedef struct Ppoly_t {
    Ppoint_t *ps;
    int pn;
} Ppoly_t;

typedef Ppoly_t Ppolyline_t;

typedef struct Pedge_t {
    Ppoint_t a, b;
} Pedge_t;


int Plegal_arrangement(Ppoly_t **polys, size_t n_polys) {

    int j, rv;

    struct data input;
    struct intersection ilist[10000];

    struct polygon *polygon_list = gv_calloc(n_polys, sizeof(struct polygon));

    size_t nverts = 0;
    for (size_t i = 0; i < n_polys; i++)
	nverts += polys[i]->pn;

    struct vertex *vertex_list = gv_calloc(nverts, sizeof(struct vertex));

    for (size_t i = 0, vno = 0; i < n_polys; i++) {
	polygon_list[i].start = &vertex_list[vno];
	for (j = 0; j < polys[i]->pn; j++) {
	    vertex_list[vno].pos.x = polys[i]->ps[j].x;
	    vertex_list[vno].pos.y = polys[i]->ps[j].y;
	    vertex_list[vno].poly = &polygon_list[i];
	    vno++;
	}
	polygon_list[i].finish = &vertex_list[vno - 1];
    }

    input.nvertices = nverts;

    find_ints(vertex_list, &input, ilist);


#define EQ_PT(v,w) (((v).x == (w).x) && ((v).y == (w).y))
    rv = 1;
    {
	struct position vft, vsd, avft, avsd;
	for (int i = 0; i < input.ninters; i++) {
	    vft = ilist[i].firstv->pos;
	    avft = after(ilist[i].firstv)->pos;
	    vsd = ilist[i].secondv->pos;
	    avsd = after(ilist[i].secondv)->pos;
	    if ((vft.x != avft.x && vsd.x != avsd.x) ||
		(vft.x == avft.x && !EQ_PT(vft, ilist[i]) && !EQ_PT(avft, ilist[i])) ||
		(vsd.x == avsd.x && !EQ_PT(vsd, ilist[i]) && !EQ_PT(avsd, ilist[i]))) {
		rv = 0;
		fprintf(stderr, "\nintersection %d at %.3f %.3f\n",
			i, ilist[i].x, ilist[i].y);
		fprintf(stderr, "seg#1 : (%.3f, %.3f) (%.3f, %.3f)\n",
			ilist[i].firstv->pos.x
			, ilist[i].firstv->pos.y
			, after(ilist[i].firstv)->pos.x
			, after(ilist[i].firstv)->pos.y);
		fprintf(stderr, "seg#2 : (%.3f, %.3f) (%.3f, %.3f)\n",
			ilist[i].secondv->pos.x
			, ilist[i].secondv->pos.y
			, after(ilist[i].secondv)->pos.x
			, after(ilist[i].secondv)->pos.y);
	    }
	}
    }
    free(polygon_list);
    free(vertex_list);
    return rv;
}
