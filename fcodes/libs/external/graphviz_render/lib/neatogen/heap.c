/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <cgraph/prisize_t.h>
#include <common/render.h>
#include <stdbool.h>
#include <stdio.h>

#include <neatogen/mem.h>
#include <neatogen/hedges.h>
#include <neatogen/heap.h>


static Halfedge *PQhash;
static int PQhashsize;
static int PQcount;
static int PQmin;

static int PQbucket(Halfedge * he)
{
    int bucket;
    double b;

    b = (he->ystar - ymin) / deltay * PQhashsize;
    if (b < 0)
	bucket = 0;
    else if (b >= PQhashsize)
	bucket = PQhashsize - 1;
    else
	bucket = b;
    if (bucket < PQmin)
	PQmin = bucket;
    return bucket;
}

void PQinsert(Halfedge * he, Site * v, double offset)
{
    Halfedge *last, *next;

    he->vertex = v;
    ref(v);
    he->ystar = v->coord.y + offset;
    last = &PQhash[PQbucket(he)];
    while ((next = last->PQnext) != NULL &&
	   (he->ystar > next->ystar ||
	    (he->ystar == next->ystar
	     && v->coord.x > next->vertex->coord.x))) {
	last = next;
    }
    he->PQnext = last->PQnext;
    last->PQnext = he;
    ++PQcount;
}

void PQdelete(Halfedge * he)
{
    Halfedge *last;

    if (he->vertex != NULL) {
	last = &PQhash[PQbucket(he)];
	while (last->PQnext != he)
	    last = last->PQnext;
	last->PQnext = he->PQnext;
	--PQcount;
	deref(he->vertex);
	he->vertex = NULL;
    }
}


bool PQempty(void)
{
    return PQcount == 0;
}


Point PQ_min(void)
{
    Point answer;

    while (PQhash[PQmin].PQnext == NULL) {
	++PQmin;
    }
    answer.x = PQhash[PQmin].PQnext->vertex->coord.x;
    answer.y = PQhash[PQmin].PQnext->ystar;
    return answer;
}

Halfedge *PQextractmin(void)
{
    Halfedge *curr;

    curr = PQhash[PQmin].PQnext;
    PQhash[PQmin].PQnext = curr->PQnext;
    --PQcount;
    return curr;
}

void PQcleanup(void)
{
    free(PQhash);
    PQhash = NULL;
}

void PQinitialize(void)
{
    int i;

    PQcount = 0;
    PQmin = 0;
    PQhashsize = 4 * sqrt_nsites;
    if (PQhash == NULL)
	PQhash = N_GNEW(PQhashsize, Halfedge);
    for (i = 0; i < PQhashsize; ++i)
	PQhash[i].PQnext = NULL;
}

static void PQdumphe(Halfedge * p)
{
    printf("  [%p] %p %p %d %d %d ",
	   p, p->ELleft, p->ELright, p->ELedge->edgenbr,
	   p->ELrefcnt, p->ELpm);
    if (p->vertex != 0) {
      printf("%" PRISIZE_T, p->vertex->sitenbr);
    } else {
      printf("-1");
    }
    printf(" %f\n", p->ystar);
}

void PQdump(void)
{
    int i;
    Halfedge *p;

    for (i = 0; i < PQhashsize; ++i) {
	printf("[%d]\n", i);
	p = PQhash[i].PQnext;
	while (p != NULL) {
	    PQdumphe(p);
	    p = p->PQnext;
	}
    }

}
