/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/


/*
 * Queue implementation using cdt
 *
 */

#include <cgraph/alloc.h>
#include <gvpr/queue.h>
#include <ast/ast.h>

typedef struct {
    Dtlink_t link;
    void *np;
} nsitem;

static void *makef(nsitem *obj, Dtdisc_t *disc) {
    (void)disc;

    nsitem *p = gv_alloc(sizeof(nsitem));
    p->np = obj->np;
    return p;
}

static void freef(nsitem *obj, Dtdisc_t *disc) {
    (void)disc;

    free(obj);
}

static Dtdisc_t ndisc = {
    offsetof(nsitem, np),
    sizeof(void *),
    offsetof(nsitem, link),
    (Dtmake_f) makef,
    (Dtfree_f) freef,
    0,
    0,
};

queue *mkQ(Dtmethod_t * meth)
{
    queue *nq;

    nq = dtopen(&ndisc, meth);
    return nq;
}

void push(queue * nq, void *n)
{
    nsitem obj;

    obj.np = n;
    dtinsert(nq, &obj);
}

void *pop(queue *nq) {
    nsitem *obj;
    void *n;

    obj = dtfirst(nq);
    if (obj) {
	n = obj->np;
	dtdelete(nq, 0);
	return n;
    } else
	return 0;
}

void freeQ(queue * nq)
{
    dtclose(nq);
}
