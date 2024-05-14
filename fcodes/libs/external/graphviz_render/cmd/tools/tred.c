/**
 * @file
 * @brief transitive reduction filter for directed graphs
 */

/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *************************************************************************/


/*
 * Written by Stephen North
 * Updated by Emden Gansner
 */

/*
 * reads a sequence of graphs on stdin, and writes their
 * transitive reduction on stdout
 */

#include <cgraph/alloc.h>
#include <cgraph/cgraph.h>
#include <cgraph/stack.h>
#include <cgraph/exit.h>
#include <cgraph/unreachable.h>
#include <common/arith.h>
#include <common/types.h>
#include <common/utils.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct {
    bool on_stack: 1;
    unsigned char dist;
} nodeinfo_t;

#define ON_STACK(ninfo,n) (ninfo[AGSEQ(n)].on_stack)
#define DISTANCE(ninfo,n) (ninfo[AGSEQ(n)].dist)
#define agrootof(n) ((n)->root)

#include <ingraphs/ingraphs.h>

#include <getopt.h>

static char **Files;
static char *CmdName;
static int Verbose;
static int PrintRemovedEdges;

static void push(gv_stack_t *sp, Agedge_t *ep, nodeinfo_t *ninfo) {

  // mark this edge on the stack
  ON_STACK(ninfo, aghead(ep)) = true;

  // insert the new edge
  stack_push(sp, ep);
}

static Agedge_t *pop(gv_stack_t *sp, nodeinfo_t *ninfo) {

  if (stack_is_empty(sp)) {
    return NULL;
  }

  // remove the top
  Agedge_t *e = stack_pop(sp);

  // mark it as no longer on the stack
  ON_STACK(ninfo, aghead(e)) = false;

  return e;
}

static Agedge_t *top(gv_stack_t *sp) {

  if (stack_is_empty(sp)) {
    return NULL;
  }

  return stack_top(sp);
}

/* dfs:
 * Main function for transitive reduction.
 * This does a DFS starting at node n. Each node records the length of
 * its largest simple path from n. We only care if the length is > 1. Node
 * n will have distance 0; outneighbors of n will have distance 1 or 2; all
 * others will have distance 2.
 *
 * During the DFS, we only push edges on the stack whose head has distance 0
 * (i.e., hasn't been visited yet), setting its distance to the distance of the
 * tail node plus one. If we find a head node with distance 1, we don't push the
 * edge, since it has already been in a DFS, but we update its distance. We also
 * check for back edges and report these.
 *
 * After the DFS, we check all outedges of n. Those edges whose head has
 * distance 2 we delete. We also delete all but one copy of any edges with the
 * same head.
 */
static int dfs(Agnode_t *n, nodeinfo_t *ninfo, int warn, gv_stack_t *sp) {
    Agraph_t *g = agrootof(n);
    Agedgepair_t dummy;
    Agedge_t* link;
    Agedge_t* next;
    Agedge_t* prev;
    Agedge_t* e;
    Agedge_t* f;
    Agnode_t* v;
    Agnode_t* hd;
    Agnode_t* oldhd;
    int do_delete;

    dummy.out.base.tag.objtype = AGOUTEDGE;
    dummy.out.node = n;
    dummy.in.base.tag.objtype = AGINEDGE;
    dummy.in.node = NULL;

    push(sp, &dummy.out, ninfo);
    prev = 0;

    while ((link = top(sp))) {
	v = aghead(link);
	if (prev)
	    next = agnxtout(g, prev);
	else
	    next = agfstout(g, v);
	for (; next; next = agnxtout(g, next)) {
	    hd = aghead(next);
	    if (hd == v) continue; // Skip a loop
	    if (ON_STACK(ninfo,hd)) {
		if (!warn) {
		    warn++;
		    fprintf(stderr,
			"warning: %s has cycle(s), transitive reduction not unique\n",
			agnameof(g));
		    fprintf(stderr, "cycle involves edge %s -> %s\n",
			agnameof(v), agnameof(hd));
		}
	    }
	    else if (DISTANCE(ninfo,hd) == 0) {
		DISTANCE(ninfo,hd) = MIN(1,DISTANCE(ninfo,v))+1;
	        break;
	    }
	    else if (DISTANCE(ninfo,hd) == 1) {
		DISTANCE(ninfo,hd) = MIN(1,DISTANCE(ninfo,v))+1;
	    }
	}
	if (next) {
	    push(sp, next, ninfo);
            prev = 0;
	}
	else {
	    prev = pop(sp, ninfo);
	}
    }
    oldhd = NULL;
    for (e = agfstout(g, n); e; e = f) {
        do_delete = 0;
	f = agnxtout(g, e);
	hd = aghead(e);
        if (oldhd == hd)
	    do_delete = 1;
        else {
            oldhd = hd;
            if (DISTANCE(ninfo, hd)>1) do_delete = 1;
        }
        if(do_delete) {
            if(PrintRemovedEdges) fprintf(stderr,"removed edge: %s: \"%s\" -> \"%s\"\n"
                          , agnameof(g), agnameof(aghead(e)), agnameof(agtail(e)));
            agdelete(g, e);
        }
    }
    return warn;
}

static char *useString = "Usage: %s [-vr?] <files>\n\
  -v - verbose (to stderr)\n\
  -r - print removed edges to stderr\n\
  -? - print usage\n\
If no files are specified, stdin is used\n";

static void usage(int v)
{
    printf(useString, CmdName);
    graphviz_exit(v);
}

static void init(int argc, char *argv[])
{
    int c;

    CmdName = argv[0];
    opterr = 0;
    while ((c = getopt(argc, argv, "vr?")) != -1) {
	switch (c) {
	case 'v':
	    Verbose = 1;
	    break;
	case 'r':
        PrintRemovedEdges = 1;
        break;
	case '?':
	    if (optopt == '\0' || optopt == '?')
		usage(0);
	    else {
		fprintf(stderr, "%s: option -%c unrecognized\n",
			CmdName, optopt);
		usage(1);
	    }
	    break;
	default:
	    UNREACHABLE();
	}
    }
    argv += optind;
    argc -= optind;

    if (argc)
	Files = argv;
}

/* process:
 * Do a DFS for each vertex in graph g, so the time
 * complexity is O(|V||E|).
 */
static void process(Agraph_t *g, gv_stack_t *sp) {
    Agnode_t *n;
    int cnt = 0;
    int warn = 0;
    double secs;
    double total_secs = 0;
    nodeinfo_t* ninfo;
    size_t infosize;

    infosize = (agnnodes(g)+1)*sizeof(nodeinfo_t);
    ninfo = gv_alloc(infosize);

    if (Verbose)
	fprintf(stderr, "Processing graph %s\n", agnameof(g));
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	memset(ninfo, 0, infosize);
	if (Verbose) start_timer();
	warn = dfs(n, ninfo, warn, sp);
	if (Verbose) {
	    secs = elapsed_sec();
            total_secs += secs;
	    cnt++;
	    if ((cnt%1000) == 0) fprintf (stderr, "[%d]\n", cnt);
	}
    }
    if (Verbose)
	fprintf(stderr, "Finished graph %s: %.02f secs.\n", agnameof(g), total_secs);
    free (ninfo);
    agwrite(g, stdout);
    fflush(stdout);
}

int main(int argc, char **argv)
{
    Agraph_t *g;
    ingraph_state ig;
    gv_stack_t estk = {0};

    init(argc, argv);
    newIngraph(&ig, Files);

    while ((g = nextGraph(&ig)) != 0) {
	if (agisdirected(g))
	    process(g, &estk);
	agclose(g);
    }

    stack_reset(&estk);

    graphviz_exit(0);
}

