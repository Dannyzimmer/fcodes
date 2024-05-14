/**
 * @file
 * @brief biconnected components filter for graphs
 */

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
 * Generate biconnected components
 *
 * Written by Emden Gansner
 */
#include "config.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>

#include <stdlib.h>
#include <cgraph/agxbuf.h>
#include <cgraph/alloc.h>
#include <cgraph/cgraph.h>
#include <cgraph/exit.h>
#include <cgraph/stack.h>
#include <cgraph/unreachable.h>

typedef struct {
    Agrec_t h;
    Agraph_t *next;
} Agraphinfo_t;

typedef struct {
    Agrec_t h;
} Agedgeinfo_t;

typedef struct {
    Agrec_t h;
    int low;
    int val;
    int isCut;
} Agnodeinfo_t;

#define Low(n)  (((Agnodeinfo_t*)(n->base.data))->low)
#define Cut(n)  (((Agnodeinfo_t*)(n->base.data))->isCut)
#define N(n)  (((Agnodeinfo_t*)(n->base.data))->val)
#define NEXTBLK(g)  (((Agraphinfo_t*)(g->base.data))->next)

#include <ingraphs/ingraphs.h>

#define min(a,b) ((a) < (b) ? (a) :  (b))

char **Files;
int verbose;
int silent;
char *outfile = 0;
char *path = 0;
char *suffix = 0;
int external;			/* emit blocks as root graphs */
int doTree;			/* emit block-cutpoint tree */

typedef struct {
    int count;
    int nComp;
    gv_stack_t stk;
    Agraph_t *blks;
} bcstate;

static char *blockName(agxbuf *xb, char *gname, int d) {
    if (*gname == '%') /* anonymous graph */
	agxbprint(xb, "_%s_bcc_%d", gname, d);
    else
	agxbprint(xb, "%s_bcc_%d", gname, d);
    return agxbuse(xb);
}

/* getName:
 * Generate name for output using input template.
 * Has form path_<g>_<i>.suffix, for ith write for the gth graph.
 * If isTree, use path_<g>_t.suffix.
 * If sufcnt is zero and graph 0, use outfile
 */
static char *getName(int ng, int nb)
{
    agxbuf name = {0};

    if (ng == 0 && nb == 0)
	agxbput(&name, outfile);
    else {
	if (suffix) {
	    if (nb < 0)
		agxbprint(&name, "%s_%d_T.%s", path, ng, suffix);
	    else
		agxbprint(&name, "%s_%d_%d.%s", path, ng, nb, suffix);
	} else {
	    if (nb < 0)
		agxbprint(&name, "%s_%d_T", path, ng);
	    else
		agxbprint(&name, "%s_%d_%d", path, ng, nb);
	}
    }
    return agxbdisown(&name);
}

static void gwrite(Agraph_t * g, int ng, int nb)
{
    FILE *outf;
    char *name;

    if (silent)
	return;
    if (!outfile) {
	agwrite(g, stdout);
	fflush(stdout);
    } else {
	name = getName(ng, nb);
	outf = fopen(name, "w");
	if (!outf) {
	    fprintf(stderr, "Could not open %s for writing\n", name);
	    perror("bcomps");
	    free(name);
	    graphviz_exit(1);
	}
	free(name);
	agwrite(g, outf);
	fclose(outf);
    }
}

static Agraph_t *mkBlock(Agraph_t * g, bcstate * stp)
{
    Agraph_t *sg;

    stp->nComp++;
    agxbuf xb = {0};
    sg = agsubg(g, blockName(&xb, agnameof(g), stp->nComp), 1);
    agxbfree(&xb);
    agbindrec(sg, "info", sizeof(Agraphinfo_t), true);
    NEXTBLK(sg) = stp->blks;
    stp->blks = sg;
    return sg;
}

static void
dfs(Agraph_t * g, Agnode_t * u, bcstate * stp, Agnode_t * parent)
{
    Agnode_t *v;
    Agedge_t *e;
    Agedge_t *ep;
    Agraph_t *sg;

    stp->count++;
    Low(u) = N(u) = stp->count;
    for (e = agfstedge(g, u); e; e = agnxtedge(g, e, u)) {
	if ((v = aghead(e)) == u)
	    v = agtail(e);
	if (v == u)
	    continue;
	if (N(v) == 0) {
	    stack_push(&stp->stk, e);
	    dfs(g, v, stp, u);
	    Low(u) = min(Low(u), Low(v));
	    if (Low(v) >= N(u)) {	/* u is an articulation point */
		Cut(u) = 1;
		sg = mkBlock(g, stp);
		do {
		    ep = stack_pop(&stp->stk);
		    agsubnode(sg, aghead(ep), 1);
		    agsubnode(sg, agtail(ep), 1);
		} while (ep != e);
	    }
	} else if (parent != v) {
	    Low(u) = min(Low(u), N(v));
	    if (N(v) < N(u))
		stack_push(&stp->stk, e);
	}
    }
}

static void nodeInduce(Agraph_t * g, Agraph_t * eg)
{
    Agnode_t *n;
    Agedge_t *e;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(eg, n); e; e = agnxtout(eg, e)) {
	    if (agsubnode(g, aghead(e), 0)) {
		agsubedge(g, e, 1);
	    }
	}
    }
}

static void addCutPts(Agraph_t * tree, Agraph_t * blk)
{
    Agnode_t *n;
    Agnode_t *bn;
    Agnode_t *cn;

    bn = agnode(tree, agnameof(blk), 1);
    for (n = agfstnode(blk); n; n = agnxtnode(blk, n)) {
	if (Cut(n)) {
	    cn = agnode(tree, agnameof(n), 1);
	    agedge(tree, bn, cn, 0, 1);
	}
    }
}

static int process(Agraph_t * g, int gcnt)
{
    Agnode_t *n;
    bcstate state;
    Agraph_t *blk;
    Agraph_t *tree;
    int bcnt;

    aginit(g, AGNODE, "info", sizeof(Agnodeinfo_t), true);
    aginit(g, AGEDGE, "info", sizeof(Agedgeinfo_t), true);
    aginit(g, AGRAPH, "info", sizeof(Agraphinfo_t), true);

    state.count = 0;
    state.nComp = 0;
    state.stk = (gv_stack_t){0};
    state.blks = 0;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (N(n) == 0)
	    dfs(g, n, &state, 0);
    }
    for (blk = state.blks; blk; blk = NEXTBLK(blk)) {
	nodeInduce(blk, g);
    }
    if (external) {
	bcnt = 0;
	for (blk = state.blks; blk; blk = NEXTBLK(blk)) {
	    gwrite(blk, gcnt, bcnt++);
	}
    } else
	gwrite(g, gcnt, 0);
    if (doTree) {
	tree = agopen("blkcut_tree", Agstrictundirected, 0);
	for (blk = state.blks; blk; blk = NEXTBLK(blk))
	    addCutPts(tree, blk);
	gwrite(tree, gcnt, -1);
	agclose(tree);
    }
    if (verbose) {
	int cuts = 0;
	bcnt = 0;
	for (blk = state.blks; blk; blk = NEXTBLK(blk))
	    bcnt++;
	for (n = agfstnode(g); n; n = agnxtnode(g, n))
	    if (Cut(n))
		cuts++;
	fprintf(stderr, "%s: %d blocks %d cutpoints\n", agnameof(g), bcnt,
		cuts);
    }
    stack_reset(&state.stk);
    if (state.blks && NEXTBLK(state.blks))
	return 1;		/* >= 2 blocks */
    else
	return 0;
}

static char *useString =
    "Usage: bcomps [-stvx?] [-o<out template>] <files>\n\
  -o - output file template\n\
  -s - don't print components\n\
  -t - emit block-cutpoint tree\n\
  -v - verbose\n\
  -x - external\n\
  -? - print usage\n\
If no files are specified, stdin is used\n";

static void usage(int v)
{
    printf("%s",useString);
    graphviz_exit(v);
}

static void split(char *name)
{
    char *sfx = 0;

    sfx = strrchr(name, '.');
    if (sfx) {
	size_t size = (size_t)(sfx - name);
	suffix = sfx + 1;
	path = gv_strndup(name, size);
    } else {
	path = name;
    }
}

static void init(int argc, char *argv[])
{
    int c;

    opterr = 0;
    while ((c = getopt(argc, argv, ":o:xstv?")) != -1) {
	switch (c) {
	case 'o':
	    outfile = optarg;
	    split(outfile);
	    break;
	case 's':
	    verbose = 1;
	    silent = 1;
	    break;
	case 'v':
	    verbose = 1;
	    break;
	case 't':
	    doTree = 1;
	    break;
	case 'x':
	    external = 1;
	    break;
	case ':':
	    fprintf(stderr, "bcomps: option -%c missing argument - ignored\n", optopt);
	    break;
	case '?':
	    if (optopt == '\0' || optopt == '?')
		usage(0);
	    else {
		fprintf(stderr,
			"bcomps: option -%c unrecognized\n", optopt);
		usage(1);
	    }
	    break;
	default:
	    UNREACHABLE();
	}
    }
    argv += optind;
    argc -= optind;

    if (argc > 0)
	Files = argv;
}

int main(int argc, char *argv[])
{
    Agraph_t *g;
    ingraph_state ig;
    int r = 0;
    int gcnt = 0;

    init(argc, argv);
    newIngraph(&ig, Files);

    while ((g = nextGraph(&ig)) != 0) {
	r |= process(g, gcnt);
	agclose(g);
	gcnt++;
    }

    graphviz_exit(r);
}
