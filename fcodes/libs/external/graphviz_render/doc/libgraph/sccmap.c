/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: See CVS logs. Details at http://www.graphviz.org/
 *************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "cgraph.h"
#include "exit.h"
#include "ingraphs.h"

#include <getopt.h>

#define INF ((unsigned int)(-1))

typedef struct Agraphinfo_t {
	Agrec_t h;
	Agnode_t *rep;
} Agraphinfo_t;

typedef struct Agnodeinfo_t {
	Agrec_t h;
	unsigned int val;
	Agraph_t *scc;
} Agnodeinfo_t;

#define getrep(g)  (((Agraphinfo_t*)(g->base.data))->rep)
#define setrep(g,rep)  (getrep(g) = rep)
#define getscc(n)    (((Agnodeinfo_t*)((n)->base.data))->scc)
#define setscc(n,sub)    (getscc(n) = sub)
#define getval(n)    (((Agnodeinfo_t*)((n)->base.data))->val)
#define setval(n,newval)    (getval(n) = newval)

/********* stack ***********/
typedef struct {
	Agnode_t **data;
	Agnode_t **ptr;
} Stack;

static void initStack(Stack * sp, int sz)
{
	sp->data = malloc(sz * sizeof(Agnode_t *));
	sp->ptr = sp->data;
}

static void freeStack(Stack * sp)
{
	free(sp->data);
}

static void push(Stack * sp, Agnode_t * n)
{
	*(sp->ptr++) = n;
}

static Agnode_t *top(Stack * sp)
{
	return *(sp->ptr - 1);
}

static Agnode_t *pop(Stack * sp)
{
	sp->ptr--;
	return *(sp->ptr);
}

/********* end stack ***********/

typedef struct {
	int Comp;
	int ID;
	int N_nodes_in_nontriv_SCC;
} sccstate;

static int wantDegenerateComp;
static int Silent;
static int StatsOnly;
static int Verbose;
static char *CmdName;
static char **Files;
static FILE *outfp;				/* output; stdout by default */

static void nodeInduce(Agraph_t * g, Agraph_t * map)
{
	Agnode_t *n;
	Agedge_t *e;
	Agraph_t *rootg = agroot(g);


	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
		for (e = agfstout(rootg, n); e; e = agnxtout(rootg, e)) {
			if (agsubnode(g, aghead(e), 0))
				agsubedge(g, e, 1);
			else {
				Agraph_t *tscc = getscc(agtail(e));
				Agraph_t *hscc = getscc(aghead(e));
				if (tscc && hscc)
					agedge(map, getrep(tscc),
						   getrep(hscc), NULL, TRUE);
			}
		}
	}
}

static int visit(Agnode_t * n, Agraph_t * map, Stack * sp, sccstate * st)
{
	unsigned int m, min;
	Agnode_t *t;
	Agraph_t *subg;
	Agedge_t *e;

	min = ++(st->ID);
	setval(n, min);
	push(sp, n);

	for (e = agfstout(n->root, n); e; e = agnxtout(n->root, e)) {
		t = aghead(e);
		if (getval(t) == 0)
			m = visit(t, map, sp, st);
		else
			m = getval(t);
		if (m < min)
			min = m;
	}

	if (getval(n) == min) {
		if (!wantDegenerateComp && (top(sp) == n)) {
			setval(n, INF);
			pop(sp);
		} else {
			char name[32];
			Agraph_t *G = agraphof(n);;
			snprintf(name, sizeof(name), "cluster_%d", (st->Comp)++);
			subg = agsubg(G, name, TRUE);
			agbindrec(subg, "scc_graph", sizeof(Agraphinfo_t), true);
			setrep(subg, agnode(map, name, TRUE));
			do {
				t = pop(sp);
				agsubnode(subg, t, 1);
				setval(t, INF);
				setscc(t, subg);
				st->N_nodes_in_nontriv_SCC++;
			} while (t != n);
			nodeInduce(subg, map);
			if (!StatsOnly)
				agwrite(subg, outfp);
		}
	}
	return min;
}

static int label(Agnode_t * n, int nodecnt, int *edgecnt)
{
	Agedge_t *e;

	setval(n, 1);
	nodecnt++;
	for (e = agfstedge(n->root, n); e; e = agnxtedge(n->root, e, n)) {
		(*edgecnt) += 1;
		if (e->node == n)
			e = agopp(e);
		if (!getval(e->node))
			nodecnt = label(e->node, nodecnt, edgecnt);
	}
	return nodecnt;
}

static int
countComponents(Agraph_t * g, int *max_degree, float *nontree_frac)
{
	int nc = 0;
	int sum_edges = 0;
	int sum_nontree = 0;
	int deg;
	int n_edges;
	int n_nodes;
	Agnode_t *n;

	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
		if (!getval(n)) {
			nc++;
			n_edges = 0;
			n_nodes = label(n, 0, &n_edges);
			sum_edges += n_edges;
			sum_nontree += (n_edges - n_nodes + 1);
		}
	}
	if (max_degree) {
		int maxd = 0;
		for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
			deg = agdegree(g, n, 1, 1);
			if (maxd < deg)
				maxd = deg;
			setval(n, 0);
		}
		*max_degree = maxd;
	}
	if (nontree_frac) {
		if (sum_edges > 0)
			*nontree_frac = (float) sum_nontree / (float) sum_edges;
		else
			*nontree_frac = 0.0;
	}
	return nc;
}

static void process(Agraph_t * G)
{
	Agnode_t *n;
	Agraph_t *map;
	int nc = 0;
	float nontree_frac = 0;
	int Maxdegree = 0;
	Stack stack;
	sccstate state;

	aginit(G, AGRAPH, "scc_graph", sizeof(Agraphinfo_t), true);
	aginit(G, AGNODE, "scc_node", sizeof(Agnodeinfo_t), true);
	state.Comp = state.ID = 0;
	state.N_nodes_in_nontriv_SCC = 0;

	if (Verbose)
		nc = countComponents(G, &Maxdegree, &nontree_frac);

	initStack(&stack, agnnodes(G) + 1);
	map = agopen("scc_map", Agdirected, (Agdisc_t *) 0);
	for (n = agfstnode(G); n; n = agnxtnode(G, n))
		if (getval(n) == 0)
			visit(n, map, &stack, &state);
	freeStack(&stack);
	if (!StatsOnly)
		agwrite(map, outfp);
	agclose(map);

	if (Verbose)
		fprintf(stderr, "%d %d %d %d %.4f %d %.4f\n",
				agnnodes(G), agnedges(G), nc, state.Comp,
				state.N_nodes_in_nontriv_SCC / (double) agnnodes(G),
				Maxdegree, nontree_frac);
	else if (!Silent)
		fprintf(stderr, "%d nodes, %d edges, %d strong components\n",
				agnnodes(G), agnedges(G), state.Comp);

}

static FILE *openFile(const char *name)
{
	FILE *fp;

	fp = fopen(name, "w");
	if (!fp) {
		fprintf(stderr, "gvpack: could not open file %s for writing\n", name);
		graphviz_exit(1);
	}
	return (fp);
}

static char *useString = "Usage: %s [-sdv?] <files>\n\
  -s           - only produce statistics\n\
  -S           - silent\n\
  -d           - allow degenerate components\n\
  -o<outfile>  - write to <outfile> (stdout)\n\
  -v           - verbose\n\
  -?           - print usage\n\
If no files are specified, stdin is used\n";

static void usage(int v)
{
	printf(useString, CmdName);
	graphviz_exit(v);
}

static void scanArgs(int argc, char **argv)
{
	int c;

	CmdName = argv[0];
	opterr = 0;
	while ((c = getopt(argc, argv, ":o:sdvS?")) != EOF) {
		switch (c) {
		case 's':
			StatsOnly = 1;
			break;
		case 'd':
			wantDegenerateComp = 1;
			break;
		case 'o':
			outfp = openFile(optarg);
			break;
		case 'v':
			Verbose = 1;
			break;
		case 'S':
			Verbose = 0;
			Silent = 1;
			break;
		case '?':
			if (optopt == '\0')
				usage(0);
			else {
				fprintf(stderr, "%s: option -%c unrecognized\n",
						CmdName, optopt);
				usage(1);
			}
			break;
		}
	}
	argv += optind;
	argc -= optind;

	if (argc)
		Files = argv;
	if (!outfp)
		outfp = stdout;			/* stdout the default */
}

int main(int argc, char **argv)
{
	Agraph_t *g;
	ingraph_state ig;

	scanArgs(argc, argv);
	newIngraph(&ig, Files);

	while ((g = nextGraph(&ig)) != 0) {
		if (agisdirected(g))
			process(g);
		else
			fprintf(stderr, "Graph %s in %s is undirected - ignored\n",
					agnameof(g), fileName(&ig));
		agclose(g);
	}

	return 0;
}
