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
#include <cgraph/exit.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define XLABEL_INT
#include <label/xlabels.h>

static int icompare(Dt_t *, void *, void *, Dtdisc_t *);

Dtdisc_t Hdisc = { offsetof(HDict_t, key), sizeof(int), -1, 0, 0,
    icompare, 0
};

static int icompare(Dt_t * dt, void * v1, void * v2, Dtdisc_t * disc)
{
    (void)dt;
    (void)disc;
    int k1 = *((int *) v1), k2 = *((int *) v2);
    if (k1 < k2) {
      return -1;
    }
    if (k1 > k2) {
      return 1;
    }
    return 0;
}

static XLabels_t *xlnew(object_t *objs, size_t n_objs, xlabel_t *lbls,
                        size_t n_lbls, label_params_t *params) {
    XLabels_t *xlp = gv_alloc(sizeof(XLabels_t));

    /* used to load the rtree in hilbert space filling curve order */
    if (!(xlp->hdx = dtopen(&Hdisc, Dtobag))) {
	fprintf(stderr, "out of memory\n");
	graphviz_exit(EXIT_FAILURE);
    }

    /* for querying intersection candidates */
    if (!(xlp->spdx = RTreeOpen())) {
	fprintf(stderr, "out of memory\n");
	graphviz_exit(EXIT_FAILURE);
    }
    /* save arg pointers in the handle */
    xlp->objs = objs;
    xlp->n_objs = n_objs;
    xlp->lbls = lbls;
    xlp->n_lbls = n_lbls;
    xlp->params = params;

    return xlp;
}

static void xlfree(XLabels_t * xlp)
{
    RTreeClose(xlp->spdx);
    free(xlp);
}

/***************************************************************************/

/*
 * determine the order(depth) of the hilbert sfc so that we satisfy the
 * precondition of hd_hil_s_from_xy()
 */
static unsigned int xlhorder(XLabels_t * xlp)
{
    double maxx = xlp->params->bb.UR.x, maxy = xlp->params->bb.UR.y;
    return (unsigned)floor(log2(round(fmax(maxx, maxy)))) + 1;
}

/* from http://www.hackersdelight.org/ site for the book by Henry S Warren */
/*
 * precondition
 * pow(2, n) >= max(p.x, p.y)
 */
/* adapted from lams1.c
Given the "order" n of a Hilbert curve and coordinates x and y, this
program computes the length s of the curve from the origin to (x, y).
The square that the Hilbert curve traverses is of size 2**n by 2**n.
   The method is that given in [Lam&Shap], described by the following
table.  Here i = n-1 for the most significant bit of x and y, and i = 0
for the least significant bits.

		    x[i]  y[i] | s[2i+1:2i]   x   y
		    -----------|-------------------
		     0     0   |     00       y   x
		     0     1   |     01       x   y
		     1     0   |     11      ~y  ~x
		     1     1   |     10       x   y

To use this table, start at the most significant bits of x and y
(i = n - 1).  If they are both 0 (first row), set the most significant
two bits of s to 00 and interchange x and y.  (Actually, it is only
necessary to interchange the remaining bits of x and y.)  If the most
significant bits of x and y are 10 (third row), output 11, interchange x
and y, and complement x and y.
   Then, consider the next most significant bits of x and y (which may
have been changed by this process), and select the appropriate row of
the table to determine the next two bits of s, and how to change x and
y.  Continue until the least significant bits of x and y have been
processed. */

static unsigned int hd_hil_s_from_xy(point p, int n)
{
    int x = p.x, y = p.y;

    unsigned s = 0;			/* Initialize. */
    for (int i = n - 1; i >= 0; i--) {
	int xi = (x >> i) & 1;	/* Get bit i of x. */
	int yi = (y >> i) & 1;	/* Get bit i of y. */
	s = 4 * s + 2 * (unsigned)xi + ((unsigned)xi ^ (unsigned)yi); // Append two bits to s.

	x = x ^ y;		/* These 3 lines swap */
	y = y ^ (x & (yi - 1));	/* x and y if yi = 0. */
	x = x ^ y;
	x = x ^ (-xi & (yi - 1));	/* Complement x and y if */
	y = y ^ (-xi & (yi - 1));	/* xi = 1 and yi = 0. */
    }
    return s;
}

/* intersection test from
 * from Real-Time Collision Detection 4.2.1 by Christer Ericson
 * intersection area from
 * http://stackoverflow.com/questions/4549544/total-area-of-intersecting-rectangles
*/
static double aabbaabb(Rect_t * r, Rect_t * s)
{
    if (!Overlap(r, s))
	return 0;

    /* if we get here we have an intersection */

    /* rightmost left edge of the 2 rectangles */
    double iminx =
	r->boundary[0] > s->boundary[0] ? r->boundary[0] : s->boundary[0];
    /* upmost bottom edge */
    double iminy =
	r->boundary[1] > s->boundary[1] ? r->boundary[1] : s->boundary[1];
    /* leftmost right edge */
    double imaxx =
	r->boundary[2] < s->boundary[2] ? r->boundary[2] : s->boundary[2];
    /* downmost top edge */
    double imaxy =
	r->boundary[3] < s->boundary[3] ? r->boundary[3] : s->boundary[3];
    return (imaxx - iminx) * (imaxy - iminy);
}

/*
 * test if objp1, a size 0 object is enclosed in the xlabel
 * associated with objp
 */
static bool lblenclosing(object_t *objp, object_t *objp1) {
  xlabel_t * xlp = objp->lbl;;

  assert(objp1->sz.x == 0 && objp1->sz.y == 0);

  if(! xlp) return false;

  return objp1->pos.x > xlp->pos.x &&
	 objp1->pos.x < (xlp->pos.x + xlp->sz.x) &&
	 objp1->pos.y > xlp->pos.y &&
	 objp1->pos.y < (xlp->pos.y + xlp->sz.y);
}

/*fill in rectangle from the object */
static Rect_t objp2rect(const object_t *op) {
  Rect_t r = {0};
  r.boundary[0] = op->pos.x;
  r.boundary[1] = op->pos.y;
  r.boundary[2] = op->pos.x + op->sz.x;
  r.boundary[3] = op->pos.y + op->sz.y;
  return r;
}

/*fill in rectangle from the objects xlabel */
static Rect_t objplp2rect(const object_t *objp) {
  Rect_t r = {0};
  const xlabel_t *lp = objp->lbl;
  r.boundary[0] = lp->pos.x;
  r.boundary[1] = lp->pos.y;
  r.boundary[2] = lp->pos.x + lp->sz.x;
  r.boundary[3] = lp->pos.y + lp->sz.y;
  return r;
}

/* compute boundary that encloses all possible label boundaries */
static Rect_t objplpmks(object_t * objp)
{
    Rect_t rect;
    pointf p;

    p.x = p.y = 0.0;
    if (objp->lbl)
	p = objp->lbl->sz;

    rect.boundary[0] = (int) floor(objp->pos.x - p.x);
    rect.boundary[1] = (int) floor(objp->pos.y - p.y);

    rect.boundary[2] = (int) ceil(objp->pos.x + objp->sz.x + p.x);
    assert(rect.boundary[2] < INT_MAX);
    rect.boundary[3] = (int) ceil(objp->pos.y + objp->sz.y + p.y);
    assert(rect.boundary[3] < INT_MAX);

    return rect;
}

/* determine the position clp will occupy in intrsx[] */
static int getintrsxi(object_t * op, object_t * cp)
{
    xlabel_t *lp = op->lbl, *clp = cp->lbl;
    assert(lp != clp);

    if (lp->set == 0 || clp->set == 0)
	return -1;
    if ((op->pos.x == 0.0 && op->pos.y == 0.0) ||
	(cp->pos.x == 0.0 && cp->pos.y == 0.0))
	return -1;

    if (cp->pos.y < op->pos.y) {
	if (cp->pos.x < op->pos.x)
	    return XLPXPY;
	if (cp->pos.x > op->pos.x)
	    return XLNXPY;
	return XLCXPY;
    }
    if (cp->pos.y > op->pos.y) {
	if (cp->pos.x < op->pos.x)
	    return XLPXNY;
	if (cp->pos.x > op->pos.x)
	    return XLNXNY;
	return XLCXNY;
    }
    if (cp->pos.x < op->pos.x)
	return XLPXCY;
    if (cp->pos.x > op->pos.x)
	return XLNXCY;

    return -1;

}

/* record the intersecting objects label */
static double
recordointrsx(object_t * op, object_t * cp, Rect_t * rp,
	      double a, object_t * intrsx[XLNBR])
{
    int i = getintrsxi(op, cp);
    if (i < 0)
	i = 5;
    if (intrsx[i] != NULL) {
	double sa, maxa = 0.0;
	/* keep maximally overlapping object */
	Rect_t srect = objp2rect(intrsx[i]);
	sa = aabbaabb(rp, &srect);
	if (sa > a)
	    maxa = sa;
	/*keep maximally overlapping label */
	if (intrsx[i]->lbl) {
	    srect = objplp2rect(intrsx[i]);
	    sa = aabbaabb(rp, &srect);
	    if (sa > a)
		maxa = fmax(sa, maxa);
	}
	if (maxa > 0.0)
	    return maxa;
	/*replace overlapping label/object pair */
	intrsx[i] = cp;
	return a;
    }
    intrsx[i] = cp;
    return a;
}

/* record the intersecting label */
static double
recordlintrsx(object_t * op, object_t * cp, Rect_t * rp,
	      double a, object_t * intrsx[XLNBR])
{
    int i = getintrsxi(op, cp);
    if (i < 0)
	i = 5;
    if (intrsx[i] != NULL) {
	double sa, maxa = 0.0;
	/* keep maximally overlapping object */
	Rect_t srect = objp2rect(intrsx[i]);
	sa = aabbaabb(rp, &srect);
	if (sa > a)
	    maxa = sa;
	/*keep maximally overlapping label */
	if (intrsx[i]->lbl) {
	    srect = objplp2rect(intrsx[i]);
	    sa = aabbaabb(rp, &srect);
	    if (sa > a)
		maxa = fmax(sa, maxa);
	}
	if (maxa > 0.0)
	    return maxa;
	/*replace overlapping label/object pair */
	intrsx[i] = cp;
	return a;
    }
    intrsx[i] = cp;
    return a;
}

/* find the objects and labels intersecting lp */
static BestPos_t
xlintersections(XLabels_t * xlp, object_t * objp, object_t * intrsx[XLNBR])
{
    BestPos_t bp;

    assert(objp->lbl);

    bp.n = 0;
    bp.area = 0.0;
    bp.pos = objp->lbl->pos;

    for (size_t i = 0; i < xlp->n_objs; i++) {
      if(objp == &xlp->objs[i]) continue;
      if(xlp->objs[i].sz.x > 0 && xlp->objs[i].sz.y > 0) continue;
      if(lblenclosing(objp, &xlp->objs[i]) ) {
	bp.n++;
      }
    }

    Rect_t rect = objplp2rect(objp);

    LeafList_t *llp = RTreeSearch(xlp->spdx, xlp->spdx->root, &rect);
    if (!llp)
	return bp;

    for (LeafList_t *ilp = llp; ilp; ilp = ilp->next) {
	double a, ra;
	object_t *cp = ilp->leaf->data;

	if (cp == objp)
	    continue;

	/*label-object intersect */
	Rect_t srect = objp2rect(cp);
	a = aabbaabb(&rect, &srect);
	if (a > 0.0) {
	  ra = recordointrsx(objp, cp, &rect, a, intrsx);
	  bp.n++;
	  bp.area += ra;
	}
	/*label-label intersect */
	if (!cp->lbl || !cp->lbl->set)
	    continue;
	srect = objplp2rect(cp);
	a = aabbaabb(&rect, &srect);
	if (a > 0.0) {
	  ra = recordlintrsx(objp, cp, &rect, a, intrsx);
	  bp.n++;
	  bp.area += ra;
	}
    }
    RTreeLeafListFree(llp);
    return bp;
}

/*
 * xladjust - find a label position
 * the individual tests at the top are intended to place a preference order
 * on the position
 */
static BestPos_t xladjust(XLabels_t * xlp, object_t * objp)
{
    xlabel_t *lp = objp->lbl;
    double xincr = (2 * lp->sz.x + objp->sz.x) / XLXDENOM;
    double yincr = (2 * lp->sz.y + objp->sz.y) / XLYDENOM;
    object_t *intrsx[XLNBR] = {0};
    BestPos_t bp, nbp;

    assert(objp->lbl);

    /*x left */
    lp->pos.x = objp->pos.x - lp->sz.x;
    /*top */
    lp->pos.y = objp->pos.y + objp->sz.y;
    bp = xlintersections(xlp, objp, intrsx);
    if (bp.n == 0)
	return bp;
    /*mid */
    lp->pos.y = objp->pos.y;
    nbp = xlintersections(xlp, objp, intrsx);
    if (nbp.n == 0)
	return nbp;
    if (nbp.area < bp.area)
	bp = nbp;
    /*bottom */
    lp->pos.y = objp->pos.y - lp->sz.y;
    nbp = xlintersections(xlp, objp, intrsx);
    if (nbp.n == 0)
	return nbp;
    if (nbp.area < bp.area)
	bp = nbp;

    /*x mid */
    lp->pos.x = objp->pos.x;
    /*top */
    lp->pos.y = objp->pos.y + objp->sz.y;
    nbp = xlintersections(xlp, objp, intrsx);
    if (nbp.n == 0)
	return nbp;
    if (nbp.area < bp.area)
	bp = nbp;
    /*bottom */
    lp->pos.y = objp->pos.y - lp->sz.y;
    nbp = xlintersections(xlp, objp, intrsx);
    if (nbp.n == 0)
	return nbp;
    if (nbp.area < bp.area)
	bp = nbp;

    /*x right */
    lp->pos.x = objp->pos.x + objp->sz.x;
    /*top */
    lp->pos.y = objp->pos.y + objp->sz.y;
    nbp = xlintersections(xlp, objp, intrsx);
    if (nbp.n == 0)
	return nbp;
    if (nbp.area < bp.area)
	bp = nbp;
    /*mid */
    lp->pos.y = objp->pos.y;
    nbp = xlintersections(xlp, objp, intrsx);
    if (nbp.n == 0)
	return nbp;
    if (nbp.area < bp.area)
	bp = nbp;
    /*bottom */
    lp->pos.y = objp->pos.y - lp->sz.y;
    nbp = xlintersections(xlp, objp, intrsx);
    if (nbp.n == 0)
	return nbp;
    if (nbp.area < bp.area)
	bp = nbp;

    /*sliding from top left */
    if (intrsx[XLPXNY] || intrsx[XLCXNY] || intrsx[XLNXNY] || intrsx[XLPXCY] || intrsx[XLPXPY]) {	/* have to move */
	if (!intrsx[XLCXNY] && !intrsx[XLNXNY]) {	/* some room right? */
	    /* slide along upper edge */
	    for (lp->pos.x = objp->pos.x - lp->sz.x,
		 lp->pos.y = objp->pos.y + objp->sz.y;
		 lp->pos.x <= (objp->pos.x + objp->sz.x);
		 lp->pos.x += xincr) {
		nbp = xlintersections(xlp, objp, intrsx);
		if (nbp.n == 0)
		    return nbp;
		if (nbp.area < bp.area)
		    bp = nbp;
	    }
	}
	if (!intrsx[XLPXCY] && !intrsx[XLPXPY]) {	/* some room down? */
	    /* slide down left edge */
	    for (lp->pos.x = objp->pos.x - lp->sz.x,
		 lp->pos.y = objp->pos.y + objp->sz.y;
		 lp->pos.y >= (objp->pos.y - lp->sz.y);
		 lp->pos.y -= yincr) {
		nbp = xlintersections(xlp, objp, intrsx);
		if (nbp.n == 0)
		    return nbp;
		if (nbp.area < bp.area)
		    bp = nbp;

	    }
	}
    }

    /*sliding from bottom right */
    lp->pos.x = objp->pos.x + objp->sz.x;
    lp->pos.y = objp->pos.y - lp->sz.y;
    if (intrsx[XLNXPY] || intrsx[XLCXPY] || intrsx[XLPXPY] || intrsx[XLNXCY] || intrsx[XLNXNY]) {	/* have to move */
	if (!intrsx[XLCXPY] && !intrsx[XLPXPY]) {	/* some room left? */
	    /* slide along lower edge */
	    for (lp->pos.x = objp->pos.x + objp->sz.x,
		 lp->pos.y = objp->pos.y - lp->sz.y;
		 lp->pos.x >= (objp->pos.x - lp->sz.x);
		 lp->pos.x -= xincr) {
		nbp = xlintersections(xlp, objp, intrsx);
		if (nbp.n == 0)
		    return nbp;
		if (nbp.area < bp.area)
		    bp = nbp;
	    }
	}
	if (!intrsx[XLNXCY] && !intrsx[XLNXNY]) {	/* some room up? */
	    /* slide up right edge */
	    for (lp->pos.x = objp->pos.x + objp->sz.x,
		 lp->pos.y = objp->pos.y - lp->sz.y;
		 lp->pos.y <= (objp->pos.y + objp->sz.y);
		 lp->pos.y += yincr) {
		nbp = xlintersections(xlp, objp, intrsx);
		if (nbp.n == 0)
		    return nbp;
		if (nbp.area < bp.area)
		    bp = nbp;
	    }
	}
    }
    return bp;
}

/* load the hilbert sfc keyed tree */
static int xlhdxload(XLabels_t * xlp)
{
    int order = xlhorder(xlp);

    for (size_t i = 0; i < xlp->n_objs; i++) {
	HDict_t *hp;
	point pi;

	hp = gv_alloc(sizeof(HDict_t));

	hp->d.data = &xlp->objs[i];
	hp->d.rect = objplpmks(&xlp->objs[i]);
	/* center of the labeling area */
	pi.x = hp->d.rect.boundary[0] +
	    (hp->d.rect.boundary[2] - hp->d.rect.boundary[0]) / 2;
	pi.y = hp->d.rect.boundary[1] +
	    (hp->d.rect.boundary[3] - hp->d.rect.boundary[1]) / 2;

	hp->key = hd_hil_s_from_xy(pi, order);

	if (!dtinsert(xlp->hdx, hp))
	    return -1;
    }
    return 0;
}

static void xlhdxunload(XLabels_t * xlp)
{
  int size=dtsize(xlp->hdx), freed=0;
  while(dtsize(xlp->hdx) ) {
    void*vp=dtfinger(xlp->hdx);
    assert(vp);
    if(vp) {
      dtdetach(xlp->hdx, vp);
      free(vp);
      freed++;
    }
  }
  assert(size==freed);
  (void)size;
}

static void xlspdxload(XLabels_t *xlp) {
    for (HDict_t *op = dtfirst(xlp->hdx); op; op = dtnext(xlp->hdx, op)) {
	/*          tree       rectangle    data        node             lvl */
	RTreeInsert(xlp->spdx, &op->d.rect, op->d.data, &xlp->spdx->root, 0);
    }
}

static int xlinitialize(XLabels_t * xlp)
{
    int r=0;
    if ((r = xlhdxload(xlp)) < 0)
	return r;
    xlspdxload(xlp);
    xlhdxunload(xlp);
    return dtclose(xlp->hdx);
}

int placeLabels(object_t *objs, size_t n_objs, xlabel_t *lbls, size_t n_lbls,
                label_params_t *params) {
    int r;
    BestPos_t bp;
    XLabels_t *xlp = xlnew(objs, n_objs, lbls, n_lbls, params);
    if ((r = xlinitialize(xlp)) < 0)
	return r;

    /* Place xlabel_t* lp near lp->obj so that the rectangle whose lower-left
     * corner is lp->pos, and size is lp->sz does not intersect any object
     * in objs (by convention, an object consisting of a single point
     * intersects nothing) nor any other label, if possible. On input,
     * lp->set is 0.
     *
     * On output, any label with a position should have this stored in
     * lp->pos and have lp->set non-zero.
     *
     * If params->force is true, all labels must be positioned, even if
     * overlaps are necessary.
     *
     * Return 0 if all labels could be placed without overlap;
     * non-zero otherwise.
     */
    r = 0;
    for (size_t i = 0; i < n_objs; i++) {
	if (objs[i].lbl == 0)
	    continue;
	bp = xladjust(xlp, &objs[i]);
	if (bp.n == 0) {
	    objs[i].lbl->set = 1;
	} else if(bp.area == 0) {
	    objs[i].lbl->pos.x = bp.pos.x;
	    objs[i].lbl->pos.y = bp.pos.y;
	    objs[i].lbl->set = 1;
	} else if (params->force == 1) {
	    objs[i].lbl->pos.x = bp.pos.x;
	    objs[i].lbl->pos.y = bp.pos.y;
	    objs[i].lbl->set = 1;
	} else {
	    r = 1;
	}
    }
    xlfree(xlp);
    return r;
}
