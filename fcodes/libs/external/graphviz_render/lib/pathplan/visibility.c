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
#include <pathplan/vis.h>
#include <stdbool.h>
#include <stdlib.h>

/* allocArray:
 * Allocate a VxV array of COORD values.
 * (array2 is a pointer to an array of pointers; the array is
 * accessed in row-major order.)
 * The values in the array are initialized to 0.
 * Add extra rows.
 */
static array2 allocArray(int V, int extra)
{
    int i;

    assert(V >= 0);
    array2 arr = gv_calloc(V + extra, sizeof(COORD*));
    COORD *p = gv_calloc((size_t)V * (size_t)V, sizeof(COORD));
    for (i = 0; i < V; i++) {
	arr[i] = p;
	p += V;
    }
    for (i = V; i < V + extra; i++)
	arr[i] = NULL;

    return arr;
}

/* area2:
 * Returns twice the area of triangle abc.
 */
COORD area2(Ppoint_t a, Ppoint_t b, Ppoint_t c)
{
    return (a.y - b.y) * (c.x - b.x) - (c.y - b.y) * (a.x - b.x);
}

/* wind:
 * Returns 1, 0, -1 if the points abc are counterclockwise,
 * collinear, or clockwise.
 */
int wind(Ppoint_t a, Ppoint_t b, Ppoint_t c)
{
    COORD w;

    w = (a.y - b.y) * (c.x - b.x) - (c.y - b.y) * (a.x - b.x);
    /* need to allow for small math errors.  seen with "gcc -O2 -mcpu=i686 -ffast-math" */
    return w > .0001 ? 1 : (w < -.0001 ? -1 : 0);
}

/* inBetween:
 * Return true if c is in (a,b), assuming a,b,c are collinear.
 */
static bool inBetween(Ppoint_t a, Ppoint_t b, Ppoint_t c)
{
    if (a.x != b.x)		/* not vertical */
	return (a.x < c.x && c.x < b.x) || (b.x < c.x && c.x < a.x);
    else
	return (a.y < c.y && c.y < b.y) || (b.y < c.y && c.y < a.y);
}

/* intersect:
 * Returns true if the segment [c,d] blocks a and b from seeing each other.
 * More specifically, returns true iff c or d lies on (a,b) or the two
 * segments intersect as open sets.
 */
static bool intersect(Ppoint_t a, Ppoint_t b, Ppoint_t c, Ppoint_t d)
{
    int a_abc;
    int a_abd;
    int a_cda;
    int a_cdb;

    a_abc = wind(a, b, c);
    if (a_abc == 0 && inBetween(a, b, c)) {
	return true;
    }
    a_abd = wind(a, b, d);
    if (a_abd == 0 && inBetween(a, b, d)) {
	return true;
    }
    a_cda = wind(c, d, a);
    a_cdb = wind(c, d, b);

    /* True if c and d are on opposite sides of ab,
     * and a and b are on opposite sides of cd.
     */
    return a_abc * a_abd < 0 && a_cda * a_cdb < 0;
}

/* in_cone:
 * Returns true iff point b is in the cone a0,a1,a2
 * NB: the cone is considered a closed set
 */
static bool in_cone(Ppoint_t a0, Ppoint_t a1, Ppoint_t a2, Ppoint_t b)
{
    int m = wind(b, a0, a1);
    int p = wind(b, a1, a2);

    if (wind(a0, a1, a2) > 0)
	return m >= 0 && p >= 0;	/* convex at a */
    else
	return m >= 0 || p >= 0;	/* reflex at a */
}

/* dist2:
 * Returns the square of the distance between points a and b.
 */
COORD dist2(Ppoint_t a, Ppoint_t b)
{
    COORD delx = a.x - b.x;
    COORD dely = a.y - b.y;

    return delx * delx + dely * dely;
}

/* dist:
 * Returns the distance between points a and b.
 */
static COORD dist(Ppoint_t a, Ppoint_t b)
{
    return sqrt(dist2(a, b));
}

static bool inCone(int i, int j, Ppoint_t pts[], int nextPt[], int prevPt[])
{
    return in_cone(pts[prevPt[i]], pts[i], pts[nextPt[i]], pts[j]);
}

/* clear:
 * Return true if no polygon line segment non-trivially intersects
 * the segment [pti,ptj], ignoring segments in [start,end).
 */
static bool clear(Ppoint_t pti, Ppoint_t ptj,
		 int start, int end,
		 int V, Ppoint_t pts[], int nextPt[])
{
    int k;

    for (k = 0; k < start; k++) {
	if (intersect(pti, ptj, pts[k], pts[nextPt[k]]))
	    return false;
    }
    for (k = end; k < V; k++) {
	if (intersect(pti, ptj, pts[k], pts[nextPt[k]]))
	    return false;
    }
    return true;
}

/* compVis:
 * Compute visibility graph of vertices of polygons.
 * Only do polygons from index startp to end.
 * If two nodes cannot see each other, the matrix entry is 0.
 * If two nodes can see each other, the matrix entry is the distance
 * between them.
 */
static void compVis(vconfig_t * conf) {
    int V = conf->N;
    Ppoint_t *pts = conf->P;
    int *nextPt = conf->next;
    int *prevPt = conf->prev;
    array2 wadj = conf->vis;
    int j, i, previ;
    COORD d;

    for (i = 0; i < V; i++) {
	/* add edge between i and previ.
	 * Note that this works for the cases of polygons of 1 and 2
	 * vertices, though needless work is done.
	 */
	previ = prevPt[i];
	d = dist(pts[i], pts[previ]);
	wadj[i][previ] = d;
	wadj[previ][i] = d;

	/* Check remaining, earlier vertices */
	if (previ == i - 1)
	    j = i - 2;
	else
	    j = i - 1;
	for (; j >= 0; j--) {
	    if (inCone(i, j, pts, nextPt, prevPt) &&
		inCone(j, i, pts, nextPt, prevPt) &&
		clear(pts[i], pts[j], V, V, V, pts, nextPt)) {
		/* if i and j see each other, add edge */
		d = dist(pts[i], pts[j]);
		wadj[i][j] = d;
		wadj[j][i] = d;
	    }
	}
    }
}

/* visibility:
 * Given a vconfig_t conf, representing polygonal barriers,
 * compute the visibility graph of the vertices of conf. 
 * The graph is stored in conf->vis. 
 */
void visibility(vconfig_t * conf)
{
    conf->vis = allocArray(conf->N, 2);
    compVis(conf);
}

/* polyhit:
 * Given a vconfig_t conf, as above, and a point,
 * return the index of the polygon that contains
 * the point, or else POLYID_NONE.
 */
static int polyhit(vconfig_t * conf, Ppoint_t p)
{
    int i;
    Ppoly_t poly;

    for (i = 0; i < conf->Npoly; i++) {
	poly.ps = &(conf->P[conf->start[i]]);
	poly.pn = conf->start[i + 1] - conf->start[i];
	if (in_poly(poly, p))
	    return i;
    }
    return POLYID_NONE;
}

/* ptVis:
 * Given a vconfig_t conf, representing polygonal barriers,
 * and a point within one of the polygons, compute the point's
 * visibility vector relative to the vertices of the remaining
 * polygons, i.e., pretend the argument polygon is invisible.
 *
 * If pp is NIL, ptVis computes the visibility vector for p
 * relative to all barrier vertices.
 */
COORD *ptVis(vconfig_t * conf, int pp, Ppoint_t p)
{
    const int V = conf->N;
    Ppoint_t *pts = conf->P;
    int *nextPt = conf->next;
    int *prevPt = conf->prev;
    int k;
    int start, end;
    Ppoint_t pk;
    COORD d;

    COORD *vadj = gv_calloc(V + 2, sizeof(COORD));

    if (pp == POLYID_UNKNOWN)
	pp = polyhit(conf, p);
    if (pp >= 0) {
	start = conf->start[pp];
	end = conf->start[pp + 1];
    } else {
	start = V;
	end = V;
    }

    for (k = 0; k < start; k++) {
	pk = pts[k];
	if (in_cone(pts[prevPt[k]], pk, pts[nextPt[k]], p) &&
	    clear(p, pk, start, end, V, pts, nextPt)) {
	    /* if p and pk see each other, add edge */
	    d = dist(p, pk);
	    vadj[k] = d;
	} else
	    vadj[k] = 0;
    }

    for (k = start; k < end; k++)
	vadj[k] = 0;

    for (k = end; k < V; k++) {
	pk = pts[k];
	if (in_cone(pts[prevPt[k]], pk, pts[nextPt[k]], p) &&
	    clear(p, pk, start, end, V, pts, nextPt)) {
	    /* if p and pk see each other, add edge */
	    d = dist(p, pk);
	    vadj[k] = d;
	} else
	    vadj[k] = 0;
    }
    vadj[V] = 0;
    vadj[V + 1] = 0;

    return vadj;

}

/* directVis:
 * Given two points, return true if the points can directly see each other.
 * If a point is associated with a polygon, the edges of the polygon
 * are ignored when checking visibility.
 */
bool directVis(Ppoint_t p, int pp, Ppoint_t q, int qp, vconfig_t * conf)
{
    int V = conf->N;
    Ppoint_t *pts = conf->P;
    int *nextPt = conf->next;
    int k;
    int s1, e1;
    int s2, e2;

    if (pp < 0) {
	s1 = 0;
	e1 = 0;
	if (qp < 0) {
	    s2 = 0;
	    e2 = 0;
	} else {
	    s2 = conf->start[qp];
	    e2 = conf->start[qp + 1];
	}
    } else if (qp < 0) {
	s1 = 0;
	e1 = 0;
	s2 = conf->start[pp];
	e2 = conf->start[pp + 1];
    } else if (pp <= qp) {
	s1 = conf->start[pp];
	e1 = conf->start[pp + 1];
	s2 = conf->start[qp];
	e2 = conf->start[qp + 1];
    } else {
	s1 = conf->start[qp];
	e1 = conf->start[qp + 1];
	s2 = conf->start[pp];
	e2 = conf->start[pp + 1];
    }

    for (k = 0; k < s1; k++) {
	if (intersect(p, q, pts[k], pts[nextPt[k]]))
	    return false;
    }
    for (k = e1; k < s2; k++) {
	if (intersect(p, q, pts[k], pts[nextPt[k]]))
	    return false;
    }
    for (k = e2; k < V; k++) {
	if (intersect(p, q, pts[k], pts[nextPt[k]]))
	    return false;
    }
    return true;
}
