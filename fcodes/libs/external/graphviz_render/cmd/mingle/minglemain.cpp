
/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 *************************************************************************/

#include "config.h"

#include <algorithm>
#include <cgraph/cgraph.h>
#include <cgraph/exit.h>
#include <ingraphs/ingraphs.h>
#include <common/pointset.h>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include <sparse/DotIO.h>
#include <mingle/edge_bundling.h>
#include <mingle/nearest_neighbor_graph.h>

typedef enum {
	FMT_GV,
	FMT_SIMPLE,
} fmt_t;

typedef struct {
	Agrec_t hdr;
	int idx;
} etoi_t;

#define ED_idx(e) (((etoi_t*)AGDATA(e))->idx)

typedef struct {
	int outer_iter;
	int method;
	int compatibility_method;
	double K;
	fmt_t fmt;
	int nneighbors;
	int max_recursion;
	double angle_param;
	double angle;
} opts_t;

static char *fname;
static FILE *outfile;

static FILE *openFile(const char *name, const char* cmd)
{
    FILE *fp;

	fp = fopen(name, "w");
	if (!fp) {
		std::cerr << cmd << ": could not open file " << name << " for writing\n";
		graphviz_exit(-1);
	}
	return (fp);
}

static const char use_msg[] =
"Usage: mingle <options> <file>\n\
    -a t - max. turning angle [0-180] (40)\n\
    -c i - compatability measure; 0 : distance, 1: full (default)\n\
    -i iter: number of outer iterations/subdivisions (4)\n\
    -k k - number of neighbors in the nearest neighbor graph of edges (10)\n\
    -K k - the force constant\n\
    -m method - method used. 0 (force directed), 1 (agglomerative ink saving, default), 2 (cluster+ink saving)\n\
    -o fname - write output to file fname (stdout)\n\
    -p t - balance for avoiding sharp angles\n\
           The larger the t, the more sharp angles are allowed\n\
    -r R - max. recursion level with agglomerative ink saving method (100)\n\
    -T fmt - output format: gv (default) or simple\n\
    -v - verbose\n";

static void
usage (int eval)
{
	std::cerr << use_msg;
	graphviz_exit(eval);
}

/* checkG:
 * Return non-zero if g has loops or multiedges.
 * Relies on multiedges occurring consecutively in edge list.
 */
static int
checkG (Agraph_t* g)
{
	Agedge_t* e;
	Agnode_t* n;
	Agnode_t* h;
	Agnode_t* prevh = nullptr;

	for (n = agfstnode (g); n; n = agnxtnode (g, n)) {
		for (e = agfstout (g, n); e; e = agnxtout (g, e)) {
			if ((h = aghead(e)) == n) return 1;   // loop
			if (h == prevh) return 1;            // multiedge
			prevh = h;
		}
		prevh = nullptr;  // reset
	}
	return 0;
}

static void init(int argc, char *argv[], opts_t* opts)
{
	int c;
	char* cmd = argv[0];
	double s;
	int i;

	opterr = 0;
	opts->outer_iter = 4;
	opts->method = METHOD_INK_AGGLOMERATE;
	opts->compatibility_method = COMPATIBILITY_FULL;
	opts->K = -1;
	opts->fmt = FMT_GV;
	opts->nneighbors = 10;
	opts->max_recursion = 100;
	opts->angle_param = -1;
	opts->angle = 40.0/180.0*M_PI;

	while ((c = getopt(argc, argv, ":a:c:i:k:K:m:o:p:r:T:v:?")) != -1) {
		switch (c) {
		case 'a':
			if ((sscanf(optarg,"%lf",&s) > 0) && (s >= 0))
				opts->angle =  M_PI*s/180;
			else
				std::cerr << "-a arg " << optarg << " must be positive real - ignored\n";
			break;
		case 'c':
			if ((sscanf(optarg,"%d",&i) > 0) && (0 <= i) && (i <= COMPATIBILITY_FULL))
				opts->compatibility_method =  i;
			else
				std::cerr << "-c arg " << optarg << " must be an integer in [0,"
					<< COMPATIBILITY_FULL << "] - ignored\n";
			break;
		case 'i':
			if ((sscanf(optarg,"%d",&i) > 0) && (i >= 0))
				opts->outer_iter =  i;
			else
				std::cerr << "-i arg " << optarg << " must be a non-negative integer - "
					"ignored\n";
			break;
		case 'k':
			if ((sscanf(optarg,"%d",&i) > 0) && (i >= 2))
				opts->nneighbors =  i;
			else
				std::cerr << "-k arg " << optarg << " must be an integer >= 2 - ignored\n";
			break;
		case 'K':
			if ((sscanf(optarg,"%lf",&s) > 0) && (s > 0))
				opts->K =  s;
			else
				std::cerr << "-K arg " << optarg << " must be positive real - ignored\n";
			break;
		case 'm':
			if ((sscanf(optarg,"%d",&i) > 0) && (0 <= i) && (i <= METHOD_INK))
				opts->method =  i;
			else
				std::cerr << "-k arg " << optarg << " must be an integer >= 2 - ignored\n";
			break;
		case 'o':
			outfile = openFile(optarg, cmd);
			break;
		case 'p':
			if ((sscanf(optarg,"%lf",&s) > 0))
				opts->angle_param =  s;
			else
				std::cerr << "-p arg " << optarg << " must be real - ignored\n";
			break;
		case 'r':
			if ((sscanf(optarg,"%d",&i) > 0) && (i >= 0))
				opts->max_recursion =  i;
			else
				std::cerr << "-r arg " << optarg << " must be a non-negative integer - "
					"ignored\n";
			break;
		case 'T':
			if (!strcmp(optarg, "gv"))
				opts->fmt = FMT_GV;
			else if (!strcmp(optarg,"simple"))
				opts->fmt = FMT_SIMPLE;
			else
				std::cerr << "-T arg " << optarg << " must be \"gv\" or \"simple\" - "
					"ignored\n";
			break;
		case 'v':
			Verbose = 1;
			if ((sscanf(optarg,"%d",&i) > 0) && (i >= 0))
				Verbose = static_cast<unsigned char>(i);
			else
				optind--;
			break;
		case ':':
			if (optopt == 'v')
				Verbose = 1;
			else {
				std::cerr << cmd << ": option -" << optopt << " missing argument\n";
				usage(1);
			}
			break;
		case '?':
			if (optopt == '\0' || optopt == '?')
				usage(0);
			else {
				std::cerr << cmd << ": option -" << optopt << " unrecognized\n";
				usage(1);
			}
			break;
		default:
			break;
		}
    }
    argv += optind;
    argc -= optind;

    if (argc > 0)
		Files = argv;
    if (!outfile) outfile = stdout;
    if (Verbose) {
       std::cerr
         << "Mingle params:\n"
         << "  outer_iter = " << opts->outer_iter << '\n'
         << "  method = " << opts->method << '\n'
         << "  compatibility_method = " << opts->compatibility_method << '\n'
         << "  K = " << std::setprecision(2) << opts->K << '\n'
         << "  fmt = " << (opts->fmt ? "simple" : "gv") << '\n'
         << "  nneighbors = " << opts->nneighbors << '\n'
         << "  max_recursion = " <<  opts->max_recursion << '\n'
         << "  angle_param = " << std::setprecision(2) <<  opts->angle_param << '\n'
         << "  angle = " << std::setprecision(2) << (180 * opts->angle / M_PI) << '\n';
    }
}

/* genBundleSpline:
 * We have ninterval+1 points. We drop the ninterval-1 internal points, and add 4 points to the first
 * and last intervals, and 3 to the rest, giving the needed 3*ninterval+4 points.
 */
static void genBundleSpline(pedge edge, std::ostream &os) {
	int k, j, mm, kk;
	int dim = edge->dim;
	double* x = edge->x;
	double tt1[3]={0.15,0.5,0.85};
	double tt2[4]={0.15,0.4,0.6,0.85};
	double t, *tt;

	for (j = 0; j < edge->npoints; j++){
		if (j != 0) {
			os << ' ';
			if ((j == 1) || (j == edge->npoints - 1)) {
				tt = tt2;
				mm = 4;
			} else {
				tt = tt1;
				mm = 3;
			}
			for (kk = 1; kk <= mm; kk++){
				t = tt[kk-1];
				for (k = 0; k < dim; k++) {
					if (k != 0) os << ',';
					os << std::setprecision(3) << (x[(j-1)*dim+k]*(1-t)+x[j*dim+k]*t);
				}
				os << ' ';
			}
		}
		if ((j == 0) || (j == edge->npoints - 1)) {
			for (k = 0; k < dim; k++) {
				if (k != 0) os << ',';
				os << std::setprecision(3) << x[j * dim + k];
			}
		}
    }
}

static void genBundleInfo(pedge edge, std::ostream &os) {
	int k, j;
	int dim = edge->dim;
	double* x = edge->x;

	for (j = 0; j < edge->npoints; j++){
		if (j != 0) os << ':';
		for (k = 0; k < dim; k++) {
			if (k != 0) os << ',';
			os << std::setprecision(3) << x[j * dim + k];
		}

		if (j < edge->npoints - 1 && !edge->wgts.empty())  {
			os << ';' << std::setprecision(3) << edge->wgts[j];
		}
	}
}

static void genBundleColors(pedge edge, std::ostream &os, double maxwgt) {
	int k, j, r, g, b;
	double len, t, len_total0 = 0;
	int dim = edge->dim;
	double* x = edge->x;
	std::vector<double> lens(edge->npoints);

	for (j = 0; j < edge->npoints - 1; j++){
		len = 0;
		for (k = 0; k < dim; k++){
			len += (x[dim*j+k] - x[dim*(j+1)+k])*(x[dim*j+k] - x[dim*(j+1)+k]);
		}
		lens[j] = len = sqrt(len/k);
		len_total0 += len;
	}
	for (j = 0; j < edge->npoints - 1; j++){
		t = edge->wgts[j]/maxwgt;
		/* interpolate between red (t = 1) to blue (t = 0) */
		r = 255*t; g = 0; b = 255*(1-t);
		if (j != 0) os << ':';
		os << std::hex << std::setw(2) << std::setfill('0') << '#' << r << g << b
			<< 85;
		if (j < edge->npoints-2) {
			os << ';' << (lens[j] / len_total0);
		}
	}
	os << std::dec << std::setw(0); // reset stream characteristics
}

static void
export_dot (FILE* fp, int ne, pedge *edges, Agraph_t* g)
{
	Agsym_t* epos = agattr(g, AGEDGE, const_cast<char*>("pos"), "");
	Agsym_t* esects = agattr(g, AGEDGE, const_cast<char*>("bundle"), "");
	Agsym_t* eclrs = nullptr;
	Agnode_t* n;
	Agedge_t* e;
	int i, j;
	double maxwgt = 0;
	pedge edge;

	  /* figure out max number of bundled original edges in a pedge */
	for (i = 0; i < ne; i++){
		edge = edges[i];
		if (!edge->wgts.empty()) {
			for (j = 0; j < edge->npoints - 1; j++){
				maxwgt = MAX(maxwgt, edge->wgts[j]);
			}
		}
	}

	std::ostringstream buf;
	for (n = agfstnode (g); n; n = agnxtnode (g, n)) {
		for (e = agfstout (g, n); e; e = agnxtout (g, e)) {
			edge = edges[ED_idx(e)];

			genBundleSpline(edge, buf);
			agxset(e, epos, buf.str().c_str());
			buf.str("");

			genBundleInfo(edge, buf);
			agxset(e, esects, buf.str().c_str());
			buf.str("");

			if (!edge->wgts.empty()) {
				if (!eclrs) eclrs = agattr(g, AGEDGE, const_cast<char*>("color"), "");
				genBundleColors(edge, buf, maxwgt);
				agxset(e, eclrs, buf.str().c_str());
				buf.str("");
			}
		}
	}
	agwrite(g, fp);
}

static int
bundle (Agraph_t* g, opts_t* opts)
{
	double *x = nullptr;
	int dim = 2;
    int i;
	int rv = 0;

	if (checkG(g)) {
		agerr(AGERR, "Graph %s (%s) contains loops or multiedges\n", agnameof(g),
		      fname);
		return 1;
	}
    initDotIO(g);
	SparseMatrix A = SparseMatrix_import_dot(g, dim, &x, FORMAT_CSR);
	if (!A){
		agerr (AGERR, "Error: could not convert graph %s (%s) into matrix\n", agnameof(g), fname);
		return 1;
    }
    if (x == nullptr) {
		agerr (AGPREV, " in file %s\n",  fname);
		return 1;
    }

	A = SparseMatrix_symmetrize(A, true);
	if (opts->fmt == FMT_GV) {
		PointMap* pm = newPM();    /* map from node id pairs to edge index */
		Agnode_t* n;
		Agedge_t* e;
		int idx = 0;

		const int *ia = A->ia;
		const int *ja = A->ja;
		for (i = 0; i < A->m; i++){
			for (int j = ia[i]; j < ia[i+1]; j++){
				if (ja[j] > i){
					insertPM (pm, i, ja[j], idx++);
				}
			}
		}
		for (i = 0, n = agfstnode(g); n; n = agnxtnode(g,n)) {
			setDotNodeID(n, i++);
		}
		for (i = 0, n = agfstnode(g); n; n = agnxtnode(g,n)) {
			for (e = agfstout (g, n); e; e = agnxtout (g, e)) {
				i = getDotNodeID (agtail(e));
				int j = getDotNodeID (aghead(e));
				if (j < i) {
					std::swap(i, j);
				}
				int k = insertPM (pm, i, j, -1);
				assert (k >= 0);
				agbindrec (e, "info", sizeof(etoi_t), true);
				ED_idx(e) = k;
			}
		}
		freePM (pm);
	}
		
	const int *ia = A->ia;
	const int *ja = A->ja;
	int nz = A->nz;
	std::vector<double> xx(nz * 4);
	nz = 0;
	dim = 4;
	for (i = 0; i < A->m; i++){
		for (int j = ia[i]; j < ia[i+1]; j++){
			if (ja[j] > i){
				xx[nz*dim] = x[i*2];
				xx[nz*dim+1] = x[i*2+1];
				xx[nz*dim+2] = x[ja[j]*2];
				xx[nz*dim+3] = x[ja[j]*2+1];
				nz++;
			}
		}
	}
	if (Verbose)
		std::cerr << "n = " << A->m << " nz = " << nz << '\n';

	SparseMatrix B = nearest_neighbor_graph(nz, std::min(opts->nneighbors, nz), xx.data());

	SparseMatrix_delete(A);
	A = B;
	free(x);
	x = xx.data();

	pedge *edges = edge_bundling(A, 2, x, opts->outer_iter, opts->K, opts->method,
	                             opts->nneighbors, opts->compatibility_method,
	                             opts->max_recursion, opts->angle_param,
	                             opts->angle);
	
	if (opts->fmt == FMT_GV) {
	    	export_dot (outfile, A->m, edges, g);
	}
	else {
		pedge_export_gv(outfile, A->m, edges);
	}
	return rv;
}

int main(int argc, char *argv[])
{
	Agraph_t *g;
	Agraph_t *prev = nullptr;
	ingraph_state ig;
	int rv = 0;
	opts_t opts;

	init(argc, argv, &opts);
	newIngraph(&ig, Files);

	while ((g = nextGraph(&ig)) != 0) {
		if (prev)
		    agclose(prev);
		prev = g;
		fname = fileName(&ig);
		if (Verbose)
		    std::cerr << "Process graph " << agnameof(g) << " in file " << fname << '\n';
		rv |= bundle (g, &opts);
	}

	graphviz_exit(rv);
}

/**
 * @dir .
 * @brief fast edge bundling
 */
