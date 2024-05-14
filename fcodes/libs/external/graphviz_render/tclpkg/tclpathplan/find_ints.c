/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include "simple.h"
#include <stdio.h>
#include <stdlib.h>
#include <cgraph/alloc.h>
#include <cgraph/exit.h>

void find_intersection(struct vertex *l, struct vertex *m,
		       struct intersection ilist[], struct data *input);
static int gt(const void *a, const void *b);

void find_ints(struct vertex vertex_list[],
	struct data *input,
	struct intersection ilist[])
{
    int j, k;
    struct active_edge_list all;
    struct active_edge *tempa;
    struct vertex *pt1, *pt2, *templ;

    input->ninters = 0;
    all.first = all.final = NULL;
    all.number = 0;

    struct vertex **pvertex = gv_calloc(input->nvertices, sizeof(struct vertex*));

    for (size_t i = 0; i < input->nvertices; i++)
	pvertex[i] = vertex_list + i;

/* sort vertices by x coordinate	*/
    qsort(pvertex, input->nvertices, sizeof(struct vertex *), gt);

/* walk through the vertices in order of increasing x coordinate	*/
    for (size_t i = 0; i < input->nvertices; i++) {
	pt1 = pvertex[i];
	templ = pt2 = prior(pvertex[i]);
	for (k = 0; k < 2; k++) {	/* each vertex has 2 edges */
	    switch (gt(&pt1, &pt2)) {

	    case -1:		/* forward edge, test and insert      */

		for (tempa = all.first, j = 0; j < all.number;
		     j++, tempa = tempa->next)
		    find_intersection(tempa->name, templ, ilist, input);	/* test */

		struct active_edge *new = gv_alloc(sizeof(struct active_edge));
		if (all.number == 0) {
		    all.first = new;
		    new->last = NULL;
		} /* insert */
		else {
		    all.final->next = new;
		    new->last = all.final;
		}

		new->name = templ;
		new->next = NULL;
		templ->active = new;
		all.final = new;
		all.number++;

		break;		/* end of case -1       */

	    case 1:		/* backward edge, delete        */

		if ((tempa = templ->active) == NULL) {
		    fprintf(stderr,
			    "\n***ERROR***\n trying to delete a non line\n");
		    graphviz_exit(1);
		}
		if (all.number == 1)
		    all.final = all.first = NULL;	/* delete the line */
		else if (tempa == all.first) {
		    all.first = all.first->next;
		    all.first->last = NULL;
		} else if (tempa == all.final) {
		    all.final = all.final->last;
		    all.final->next = NULL;
		} else {
		    tempa->last->next = tempa->next;
		    tempa->next->last = tempa->last;
		}
		free(tempa);
		all.number--;
		templ->active = NULL;
		break;		/* end of case 1        */

	    default:
		break;
	    }			/* end switch   */

	    pt2 = after(pvertex[i]);
	    templ = pvertex[i];	/*second neighbor */
	}			/* end k for loop       */
    }				/* end i for loop       */
    free(pvertex);
}

static int gt(const void *a, const void *b) {
    const struct vertex *const *i = a;
    const struct vertex *const *j = b;
    if ((*i)->pos.x > (*j)->pos.x) {
      return 1;
    }
    if ((*i)->pos.x < (*j)->pos.x) {
      return -1;
    }
    if ((*i)->pos.y > (*j)->pos.y) {
      return 1;
    }
    if ((*i)->pos.y < (*j)->pos.y) {
      return -1;
    }
    return 0;
}
