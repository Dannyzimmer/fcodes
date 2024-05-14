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
#include <stdlib.h>
#include <inttypes.h>
#include <label/index.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <label/node.h>

/* Make a new node and initialize to have all branch cells empty.
*/
Node_t *RTreeNewNode(void) {
    Node_t *n = gv_alloc(sizeof(Node_t));
    InitNode(n);
    return n;
}

/* Initialize a Node structure.
*/
void InitNode(Node_t * n)
{
    n->count = 0;
    n->level = -1;
    for (size_t i = 0; i < NODECARD; i++)
	InitBranch(&(n->branch[i]));
}

/* Initialize one branch cell in a node.
*/
void InitBranch(Branch_t * b)
{
    InitRect(&(b->rect));
    b->child = NULL;
}

#ifdef RTDEBUG
/* Print out the data in a node.
*/
void PrintNode(Node_t * n)
{
    assert(n);

    fprintf(stderr, "node");
    if (n->level == 0)
	fprintf(stderr, " LEAF");
    else if (n->level > 0)
	fprintf(stderr, " NONLEAF");
    else
	fprintf(stderr, " TYPE=?");
    fprintf(stderr, "  level=%d  count=%d  child address=%p\n",
	    n->level, n->count, n);

    for (size_t i = 0; i < NODECARD; i++) {
	if (n->branch[i].child != NULL)
	    PrintBranch(i, &n->branch[i]);
    }
}

void PrintBranch(int i, Branch_t * b)
{
    fprintf(stderr, "  child[%d] X%X\n", i, (unsigned int) b->child);
    PrintRect(&(b->rect));
}
#endif

/* Find the smallest rectangle that includes all rectangles in
** branches of a node.
*/
Rect_t NodeCover(Node_t * n)
{
    Rect_t r;
    assert(n);

    InitRect(&r);
    bool flag = true;
    for (size_t i = 0; i < NODECARD; i++)
	if (n->branch[i].child) {
	    if (flag) {
		r = n->branch[i].rect;
		flag = false;
	    } else
		r = CombineRect(&r, &(n->branch[i].rect));
	}
    return r;
}

/* Pick a branch.  Pick the one that will need the smallest increase
** in area to accommodate the new rectangle.  This will result in the
** least total area for the covering rectangles in the current node.
** In case of a tie, pick the one which was smaller before, to get
** the best resolution when searching.
*/
int PickBranch(Rect_t * r, Node_t * n)
{
    uint64_t bestIncr = 0;
    uint64_t bestArea = 0;
    int best=0;
    bool bestSet = false;
    assert(r && n);

    for (int i = 0; i < NODECARD; i++) {
	if (n->branch[i].child) {
	    Rect_t *rr = &n->branch[i].rect;
	    uint64_t area = RectArea(rr);
	    Rect_t rect = CombineRect(r, rr);
	    uint64_t increase = RectArea(&rect) - area;
	    if (!bestSet || increase < bestIncr) {
		best = i;
		bestArea = area;
		bestIncr = increase;
		bestSet = true;
	    } else if (increase == bestIncr && area < bestArea) {
		best = i;
		bestArea = area;
		bestIncr = increase;
	    }
#			ifdef RTDEBUG
	    fprintf(stderr,
		    "i=%d  area before=%" PRIu64 "  area after=%" PRIu64 "  increase=%" PRIu64
		    "\n", i, area, area + increase, increase);
#			endif
	}
    }
#	ifdef RTDEBUG
    fprintf(stderr, "\tpicked %d\n", best);
#	endif
    return best;
}

/* Add a branch to a node.  Split the node if necessary.
** Returns 0 if node not split.  Old node updated.
** Returns 1 if node split, sets *new to address of new node.
** Old node updated, becomes one of two.
*/
int AddBranch(RTree_t * rtp, Branch_t * b, Node_t * n, Node_t ** new)
{
    assert(b);
    assert(n);

    if (n->count < NODECARD) {	/* split won't be necessary */
	size_t i;
	for (i = 0; i < NODECARD; i++) {	/* find empty branch */
	    if (n->branch[i].child == NULL) {
		n->branch[i] = *b;
		n->count++;
		break;
	    }
	}
	assert(i < NODECARD);
	return 0;
    } else {
	assert(new);
	SplitNode(rtp, n, b, new);
	return 1;
    }
}

/* Disconnect a dependent node.
*/
void DisconBranch(Node_t * n, int i)
{
    assert(n && i >= 0 && i < NODECARD);
    assert(n->branch[i].child);

    InitBranch(&(n->branch[i]));
    n->count--;
}
