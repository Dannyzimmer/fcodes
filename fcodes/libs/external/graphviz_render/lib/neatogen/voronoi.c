/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <neatogen/mem.h>
#include <neatogen/geometry.h>
#include <neatogen/edges.h>
#include <neatogen/hedges.h>
#include <neatogen/heap.h>
#include <neatogen/voronoi.h>


void voronoi(Site * (*nextsite) (void)) {
    Site *newsite, *bot, *top, *temp, *p;
    Site *v;
    Point newintstar = {0};
    char pm;
    Halfedge *lbnd, *rbnd, *llbnd, *rrbnd, *bisector;
    Edge *e;

    edgeinit();
    siteinit();
    PQinitialize();
    bottomsite = nextsite();
#ifdef STANDALONE
    out_site(bottomsite);
#endif
    ELinitialize();

    newsite = nextsite();
    while (1) {
	if (!PQempty())
	    newintstar = PQ_min();

	if (newsite != NULL &&
      (PQempty() || newsite->coord.y < newintstar.y ||
       (newsite->coord.y ==newintstar.y && newsite->coord.x < newintstar.x))) {
	    /* new site is smallest */
#ifdef STANDALONE
	    out_site(newsite);
#endif
	    lbnd = ELleftbnd(&newsite->coord);
	    rbnd = ELright(lbnd);
	    bot = rightreg(lbnd);
	    e = gvbisect(bot, newsite);
	    bisector = HEcreate(e, le);
	    ELinsert(lbnd, bisector);
	    if ((p = hintersect(lbnd, bisector)) != NULL) {
		PQdelete(lbnd);
		PQinsert(lbnd, p, dist(p, newsite));
	    }
	    lbnd = bisector;
	    bisector = HEcreate(e, re);
	    ELinsert(lbnd, bisector);
	    if ((p = hintersect(bisector, rbnd)) != NULL)
		PQinsert(bisector, p, dist(p, newsite));
	    newsite = nextsite();
	} else if (!PQempty()) {
	    /* intersection is smallest */
	    lbnd = PQextractmin();
	    llbnd = ELleft(lbnd);
	    rbnd = ELright(lbnd);
	    rrbnd = ELright(rbnd);
	    bot = leftreg(lbnd);
	    top = rightreg(rbnd);
#ifdef STANDALONE
	    out_triple(bot, top, rightreg(lbnd));
#endif
	    v = lbnd->vertex;
	    makevertex(v);
	    endpoint(lbnd->ELedge, lbnd->ELpm, v);
	    endpoint(rbnd->ELedge, rbnd->ELpm, v);
	    ELdelete(lbnd);
	    PQdelete(rbnd);
	    ELdelete(rbnd);
	    pm = le;
	    if (bot->coord.y > top->coord.y) {
		temp = bot;
		bot = top;
		top = temp;
		pm = re;
	    }
	    e = gvbisect(bot, top);
	    bisector = HEcreate(e, pm);
	    ELinsert(llbnd, bisector);
	    endpoint(e, re - pm, v);
	    deref(v);
	    if ((p = hintersect(llbnd, bisector)) != NULL) {
		PQdelete(llbnd);
		PQinsert(llbnd, p, dist(p, bot));
	    }
	    if ((p = hintersect(bisector, rrbnd)) != NULL) {
		PQinsert(bisector, p, dist(p, bot));
	    }
	} else
	    break;
    }

    for (lbnd = ELright(ELleftend); lbnd != ELrightend; lbnd = ELright(lbnd)) {
	e = lbnd->ELedge;
	clip_line(e);
#ifdef STANDALONE
	out_ep(e);
#endif
    }
}
