/**
 * @file
 * @brief <a href=https://math.nist.gov/MatrixMarket/>Matrix Market</a>-DOT converter
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

#include "config.h"
#include <cgraph/alloc.h>
#include <cgraph/unreachable.h>

#define STANDALONE
#include <cgraph/cgraph.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "mmio.h"
#include <cgraph/agxbuf.h>
#include <sparse/SparseMatrix.h>
#include "matrix_market.h"
#include <getopt.h>

#define BUFS         1024

typedef struct {
    Agrec_t h;
    int id;
} Agnodeinfo_t;

#define ND_id(n)  (((Agnodeinfo_t*)(n->base.data))->id)

static char *cmd;

static double Hue2RGB(double v1, double v2, double H)
{
    if (H < 0.0)
	H += 1.0;
    if (H > 1.0)
	H -= 1.0;
    if (6.0 * H < 1.0)
	return (v1 + (v2 - v1) * 6.0 * H);
    if (2.0 * H < 1.0)
	return v2;
    if (3.0 * H < 2.0)
	return v1 + (v2 - v1) * (2.0 / 3.0 - H) * 6.0;
    return v1;
}

static char *hue2rgb(double hue, agxbuf *xb) {
    double v1, v2, lightness = .5, saturation = 1;
    int red, blue, green;

    v2 = lightness + saturation - saturation * lightness;

    v1 = 2.0 * lightness - v2;

    red = (int) (255.0 * Hue2RGB(v1, v2, hue + 1.0 / 3.0) + 0.5);
    green = (int) (255.0 * Hue2RGB(v1, v2, hue) + 0.5);
    blue = (int) (255.0 * Hue2RGB(v1, v2, hue - 1.0 / 3.0) + 0.5);
    agxbprint(xb, "#%02x%02x%02x", red, green, blue);
    return agxbuse(xb);
}

static Agraph_t *makeDotGraph(SparseMatrix A, char *name, int dim,
			      double * x, int with_color, int with_label, int with_val)
{
    Agraph_t *g;
    Agnode_t *n;
    Agnode_t *h;
    Agedge_t *e;
    int i, j;
    agxbuf xb;
    char string[BUFS];
    Agsym_t *sym = NULL, *sym2 = NULL, *sym3 = NULL;
    int *ia = A->ia;
    int *ja = A->ja;
    double *val = A->a;
    Agnode_t **arr = gv_calloc(A->m, sizeof(Agnode_t*));
    double *color = NULL;

    name = strip_dir(name);

    if (with_color) {
	if (A->type == MATRIX_TYPE_REAL && !val) {
	    fprintf (stderr, "Warning: input matrix has no values, -c flag ignored\n");
	    with_color = 0;
	}
	else if (A->type != MATRIX_TYPE_REAL && !x) {
	    fprintf (stderr, "Warning: input has no coordinates, -c flag ignored\n");
	    with_color = 0;
	}
    }

    if (SparseMatrix_known_undirected(A)) {
	g = agopen("G", Agundirected, NULL);
    } else {
	g = agopen("G", Agdirected, NULL);
    }

    if (with_val) {
	sym = agattr(g, AGEDGE, "len", "1");
    }
    agxbinit (&xb, BUFS, string);
    if (with_label) {
	agxbprint (&xb, "%s. %d nodes, %d edges.", name, A->m, A->nz);
	agattr(g, AGRAPH, "label", agxbuse (&xb));
    }

    for (i = 0; i < A->m; i++) {
	agxbprint(&xb, "%d", i);
	n = agnode(g, agxbuse(&xb), 1);
	agbindrec(n, "nodeinfo", sizeof(Agnodeinfo_t), true);
	ND_id(n) = i;
	arr[i] = n;
    }

    if (with_color) {
	double maxdist = 0.;
	double mindist = 0.;
	bool first = true;

	sym2 = agattr(g, AGEDGE, "color", "");
	sym3 = agattr(g, AGEDGE, "wt", "");
	agattr(g, AGRAPH, "bgcolor", "black");
	color = gv_calloc(A->nz, sizeof(double));
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    i = ND_id(n);
	    if (A->type != MATRIX_TYPE_REAL) {
		for (j = ia[i]; j < ia[i + 1]; j++) {
		    color[j] = distance(x, dim, i, ja[j]);
		    if (i != ja[j]) {
			if (first) {
			    mindist = color[j];
			    first = false;
			} else {
			    mindist = fmin(mindist, color[j]);
			}
		    }
		    maxdist = fmax(color[j], maxdist);
		}
	    } else {
		for (j = ia[i]; j < ia[i + 1]; j++) {
		    if (val) color[j] = fabs(val[j]);
		    else color[j] = 1;
		    if (i != ja[j]) {
			if (first) {
			    mindist = color[j];
			    first = false;
			} else {
			    mindist = fmin(mindist, color[j]);
			}
		    }
		    maxdist = fmax(color[j], maxdist);
		}
	    }
	}
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    i = ND_id(n);
	    for (j = ia[i]; j < ia[i + 1]; j++) {
		color[j] = (color[j] - mindist) / fmax(maxdist - mindist, 0.000001);
	    }
	}
    }

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	i = ND_id(n);
	for (j = ia[i]; j < ia[i + 1]; j++) {
	    h = arr[ja[j]];
	    e = agedge(g, n, h, NULL, 1);
	    if (sym && val) {
		agxbprint(&xb, "%f", val[j]);
		agxset(e, sym, agxbuse(&xb));
	    }
	    if (with_color) {
		agxset(e, sym2, hue2rgb(.65 * color[j], &xb));
		agxbprint(&xb, "%f", color[j]);
		agxset(e, sym3, agxbuse(&xb));
	    }
	}
    }

    agxbfree (&xb);
    free(color);
    free(arr);
    return g;
}

static char* useString = "Usage: %s [-uvcl] [-o file] matrix_market_filename\n\
  -u   - make graph undirected\n\
  -U i - treat non-square matrix as a bipartite graph\n\
         i = 0   never\n\
         i = 1   if pattern unsymmetric (default)\n\
         i = 2   if matrix unsymmetric\n\
         i = 3   always\n\
  -v   - assign len to edges\n\
  -c   - assign color and wt to edges\n\
  -l   - add label\n\
  -o <file> - output file \n";

static void usage(int eval)
{
    fprintf(stderr, useString, cmd);
    graphviz_exit(eval);
}

static FILE *openF(char *fname, char *mode)
{
    FILE *f = fopen(fname, mode);
    if (!f) {
	fprintf(stderr, "Could not open %s for %s\n", fname,
		*mode == 'r' ? "reading" : "writing");
	graphviz_exit(1);
    }
    return f;
}

typedef struct {
    FILE *inf;
    FILE *outf;
    char *infile;
    int undirected;
    int with_label;
    int with_color;
    int with_val;
  int bipartite;
} parms_t;

static void init(int argc, char **argv, parms_t * p)
{
    int c;

    cmd = argv[0];
    opterr = 0;
    while ((c = getopt(argc, argv, ":o:uvclU:?")) != -1) {
	switch (c) {
	case 'o':
	    p->outf = openF(optarg, "w");
	    break;
	case 'l':
	    p->with_label = 1;
	    break;
	case 'u':
	    p->undirected = 1;
	    break;
	case 'v':
	    p->with_val = 1;
	    break;
	case 'c':
	    p->with_color = 1;
	    break;
	case 'U':{
	  int u;
	  if (sscanf(optarg,"%d",&u) <= 0 || u < 0 || u > BIPARTITE_ALWAYS) {
	    usage(1);
	  } else {
	    p->bipartite = u;
	  }
	  break;
	}
 	case ':':
	    fprintf(stderr, "%s: option -%c missing argument - ignored\n", cmd, optopt);
	    break;
 	case '?':
	    if (optopt == '\0' || optopt == '?')
		usage(0);
	    else {
		fprintf(stderr,
			"%s: option -%c unrecognized\n", cmd,
			optopt);
		usage(1);
	    }
	    break;
 	default:
	    UNREACHABLE();
	}
    }
    argv += optind;
    argc -= optind;

    if (argc > 0) {
	p->infile = argv[0];
	p->inf = openF(argv[0], "r");
    }
}

int main(int argc, char *argv[])
{
    Agraph_t *g = 0;
    SparseMatrix A = NULL;
    int dim=0;
    parms_t pv;

    /* ======================= set parameters ==================== */
    pv.inf = stdin;
    pv.outf = stdout;
    pv.infile = "stdin";
    pv.undirected = 0;
    pv.with_label = 0;
    pv.with_color = 0;
    pv.with_val = 0;
    pv.bipartite = BIPARTITE_PATTERN_UNSYM;
    init(argc, argv, &pv);

    /* ======================= read graph ==================== */

    A = SparseMatrix_import_matrix_market(pv.inf);
    if (!A) {
	fprintf (stderr, "Unable to read input file \"%s\"\n", pv.infile); 
	usage(1);
    }

    A = SparseMatrix_to_square_matrix(A, pv.bipartite);

    if (!A) {
	fprintf(stderr, "cannot import from file %s\n", pv.infile);
	graphviz_exit(1);
    }

    if (pv.undirected) {
	SparseMatrix B;
	B = SparseMatrix_make_undirected(A);
	SparseMatrix_delete(A);
	A = B;
    }
    g = makeDotGraph(A, pv.infile, dim, NULL, pv.with_color, pv.with_label, pv.with_val);

    agwrite(g, pv.outf);

    graphviz_exit(0);
}

