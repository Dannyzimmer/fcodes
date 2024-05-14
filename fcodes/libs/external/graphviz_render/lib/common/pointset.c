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
#include <common/render.h>
#include <common/pointset.h>

typedef struct {
    Dtlink_t link;
    point id;
} pair;

static pair *mkPair(point p)
{
    pair *pp = gv_alloc(sizeof(pair));
    pp->id = p;
    return pp;
}

static void freePair(pair *pp, Dtdisc_t *disc) {
    (void)disc;

    free (pp);
}

static int cmppair(Dt_t * d, point * key1, point * key2, Dtdisc_t * disc)
{
    (void)d;
    (void)disc;

    if (key1->x > key2->x)
	return 1;
    else if (key1->x < key2->x)
	return -1;
    else if (key1->y > key2->y)
	return 1;
    else if (key1->y < key2->y)
	return -1;
    else
	return 0;
}

static Dtdisc_t intPairDisc = {
    offsetof(pair, id),
    sizeof(point),
    offsetof(pair, link),
    0,
    (Dtfree_f) freePair,
    (Dtcompar_f) cmppair,
    0,
};

PointSet *newPS(void)
{
    return (dtopen(&intPairDisc, Dtoset));
}

void freePS(PointSet * ps)
{
    dtclose(ps);
}

void insertPS(PointSet * ps, point pt)
{
    pair *pp;

    pp = mkPair(pt);
    if (dtinsert(ps, pp) != pp)
        free(pp);
}

void addPS(PointSet * ps, int x, int y)
{
    point pt;
    pair *pp;

    pt.x = x;
    pt.y = y;
    pp = mkPair(pt);
    if (dtinsert(ps, pp) != pp)
        free(pp);
}

int inPS(PointSet * ps, point pt)
{
    pair p;
    p.id = pt;
    return dtsearch(ps, &p) ? 1 : 0;
}

int isInPS(PointSet * ps, int x, int y)
{
    pair p;
    p.id.x = x;
    p.id.y = y;
    return dtsearch(ps, &p) ? 1 : 0;
}

int sizeOf(PointSet * ps)
{
    return dtsize(ps);
}

point *pointsOf(PointSet * ps)
{
    int n = dtsize(ps);
    point *pts = N_NEW(n, point);
    pair *p;
    point *pp = pts;

    for (p = (pair *) dtflatten(ps); p;
	 p = (pair *)dtlink(ps, p)) {
	*pp++ = p->id;
    }

    return pts;
}

typedef struct {
    Dtlink_t link;
    point id;
    int v;
} mpair;

typedef struct {
    Dtdisc_t disc;
    mpair *flist;
} MPairDisc;

static mpair *mkMPair(mpair *obj, MPairDisc *disc) {
    mpair *ap;

    if (disc->flist) {
	ap = disc->flist;
	disc->flist = (mpair *) (ap->link.right);
    } else
	ap = gv_alloc(sizeof(mpair));
    ap->id = obj->id;
    ap->v = obj->v;
    return ap;
}

static void freeMPair(mpair *ap, MPairDisc *disc) {
    ap->link.right = (Dtlink_t *) (disc->flist);
    disc->flist = ap;
}

static Dtdisc_t intMPairDisc = {
    offsetof(mpair, id),
    sizeof(point),
    offsetof(mpair, link),
    (Dtmake_f) mkMPair,
    (Dtfree_f) freeMPair,
    (Dtcompar_f) cmppair,
    0,
};

PointMap *newPM(void)
{
    MPairDisc *dp = gv_alloc(sizeof(MPairDisc));

    dp->disc = intMPairDisc;
    dp->flist = 0;

    return (dtopen(&(dp->disc), Dtoset));
}

void clearPM(PointMap * ps)
{
    dtclear(ps);
}

void freePM(PointMap * ps)
{
    MPairDisc *dp = (MPairDisc *) (ps->disc);
    mpair *p;
    mpair *next;

    dtclose(ps);
    for (p = dp->flist; p; p = next) {
	next = (mpair *) (p->link.right);
	free(p);
    }
    free(dp);
}

int insertPM(PointMap * pm, int x, int y, int value)
{
    mpair *p;
    mpair dummy;

    dummy.id.x = x;
    dummy.id.y = y;
    dummy.v = value;
    p = dtinsert(pm, &dummy);
    return p->v;
}
