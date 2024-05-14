/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include "config.h"

#include <label/index.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <common/arith.h>
#include <label/rectangle.h>
#include <cgraph/cgraph.h>
#include <cgraph/exit.h>

#define Undefined(x) ((x)->boundary[0] > (x)->boundary[NUMDIMS])

/*-----------------------------------------------------------------------------
| Initialize a rectangle to have all 0 coordinates.
-----------------------------------------------------------------------------*/
void InitRect(Rect_t * r)
{
    for (size_t i = 0; i < NUMSIDES; i++)
	r->boundary[i] = 0;
}

/*-----------------------------------------------------------------------------
| Return a rect whose first low side is higher than its opposite side -
| interpreted as an undefined rect.
-----------------------------------------------------------------------------*/
Rect_t NullRect(void)
{
    Rect_t r = {{0}};

    r.boundary[0] = 1;
    r.boundary[NUMDIMS] = -1;
    return r;
}

#ifdef RTDEBUG
/*-----------------------------------------------------------------------------
| Print rectangle lower upper bounds by dimension
-----------------------------------------------------------------------------*/
void PrintRect(Rect_t * r)
{
    assert(r);
    fprintf(stderr, "rect:");
    for (size_t i = 0; i < NUMDIMS; i++)
	fprintf(stderr, "\t%d\t%d\n", r->boundary[i],
		r->boundary[i + NUMDIMS]);
}
#endif

/*-----------------------------------------------------------------------------
| Calculate the n-dimensional area of a rectangle
-----------------------------------------------------------------------------*/

uint64_t RectArea(const Rect_t *r) {
  assert(r);

    if (Undefined(r))
	return 0;

    uint64_t area = 1;
    for (size_t i = 0; i < NUMDIMS; i++) {
      unsigned int dim = r->boundary[i + NUMDIMS] - r->boundary[i];
      if (dim == 0) return 0;
      if (UINT64_MAX / dim < area) {
	agerr (AGERR, "label: area too large for rtree\n");
	graphviz_exit(EXIT_FAILURE);
      }
      area *= dim;
    }
    return area;
}

/*-----------------------------------------------------------------------------
| Combine two rectangles, make one that includes both.
-----------------------------------------------------------------------------*/
Rect_t CombineRect(const Rect_t *r, const Rect_t *rr) {
    Rect_t new;
    assert(r && rr);

    if (Undefined(r))
	return *rr;
    if (Undefined(rr))
	return *r;

    for (size_t i = 0; i < NUMDIMS; i++) {
	new.boundary[i] = MIN(r->boundary[i], rr->boundary[i]);
	size_t j = i + NUMDIMS;
	new.boundary[j] = MAX(r->boundary[j], rr->boundary[j]);
    }
    return new;
}

/*-----------------------------------------------------------------------------
| Decide whether two rectangles overlap.
-----------------------------------------------------------------------------*/
bool Overlap(const Rect_t *r, const Rect_t *s) {
    assert(r && s);

    for (size_t i = 0; i < NUMDIMS; i++) {
	size_t j = i + NUMDIMS;	/* index for high sides */
	if (r->boundary[i] > s->boundary[j] || s->boundary[i] > r->boundary[j])
	    return false;
    }
    return true;
}
