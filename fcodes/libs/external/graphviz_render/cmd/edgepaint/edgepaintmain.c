/**
 * @dir .
 * @brief %edge coloring to disambiguate crossing edges
 */

/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 *************************************************************************/

#include "config.h"

#include <cgraph/alloc.h>
#include <cgraph/cgraph.h>
#include <cgraph/agxbuf.h>
#include <cgraph/exit.h>
#include <cgraph/startswith.h>
#include <cgraph/unreachable.h>
#include <ingraphs/ingraphs.h>
#include <common/pointset.h>
#include <getopt.h>

#include <sparse/general.h>
#include <sparse/SparseMatrix.h>
#include <sparse/DotIO.h>
#include <edgepaint/node_distinct_coloring.h>
#include <edgepaint/edge_distinct_coloring.h>
#include <sparse/color_palette.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static char *fname;
static FILE *outfile;

static FILE *openFile(const char *name, const char* cmd)
{
    FILE *fp;

	fp = fopen(name, "w");
	if (!fp) {
		fprintf(stderr, "%s: could not open file %s for writing\n", cmd, name);
		graphviz_exit(EXIT_FAILURE);
	}
	return fp;
}

static void usage (char* cmd, int eval){
  fprintf(stderr, "Usage: %s <options> gv file with 2D coordinates.\n", cmd);
  fprintf(stderr, "Find a color assignment of the edges, such that edges that cross at small angle have as different as possible.\n");
  fprintf(stderr, "Options are: \n");
  fprintf(stderr, " --accuracy=e      : accuracy with which to find the maximally different coloring for each node with regard to its neighbors. Default 0.01.\n");
  fprintf(stderr, " --angle=a         : if edge crossing is less than that angle a, then make the edge colors different. Default 15.\n");
  fprintf(stderr, " --random_seed=s   : random seed to use. s must be an integer. If s is negative, we do -s iterations with different seeds and pick the best.\n");
  fprintf(stderr, " --color_scheme=c  : palette used. The string c should be \"rgb\", \"gray\", \"lab\" (default); or\n");
  fprintf(stderr, "       a comma-separated list of RGB colors in hex (e.g., \"#ff0000,#aabbed,#eeffaa\"); or\n");
  fprintf(stderr, "       a string specifying a Brewer color scheme (e.g., \"accent7\"; see https://graphviz.org/doc/info/colors.html#brewer).\n");
  fprintf(stderr, " --lightness=l1,l2 : only applied for LAB color scheme: l1 must be integer >=0, l2 integer <=100, and l1 <=l2. By default we use 0,70\n");
  fprintf(stderr, " --share_endpoint  :  if this option is specified, edges that shares an end point are not considered in conflict if they are close to\n");
  fprintf(stderr, "       parallel but is on the opposite ends of the shared point (around 180 degree).\n");
  fprintf(stderr, " -v               : verbose\n");
  fprintf(stderr, " -o fname         :  write output to file fname (stdout)\n");
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
	Agnode_t* prevh = NULL;

	for (n = agfstnode (g); n; n = agnxtnode (g, n)) {
		for (e = agfstout (g, n); e; e = agnxtout (g, e)) {
			if ((h = aghead(e)) == n) return 1;   // loop
			if (h == prevh) return 1;            // multiedge
			prevh = h;
		}
		prevh = NULL;  // reset
	}
	return 0;
}

static void init(int argc, char *argv[], double *angle, double *accuracy, int *check_edges_with_same_endpoint, int *seed, char **color_scheme, char **lightness){

  char* cmd = argv[0];
  outfile = NULL;

  Verbose = 0;
  *accuracy = 0.01;
  *angle = 15;/* 10 degree by default*/
  *check_edges_with_same_endpoint = 0;
  *seed = 123;
  *color_scheme = "lab";
  *lightness = NULL;

  while (true) {

    // some constants above the range of valid ASCII to use as getopt markers
    enum {
      OPT_ACCURACY = 128,
      OPT_ANGLE = 129,
      OPT_COLOR_SCHEME = 130,
      OPT_RANDOM_SEED = 131,
      OPT_LIGHTNESS = 132,
      OPT_SHARE_ENDPOINT = 133,
    };

    static const struct option opts[] = {
        // clang-format off
        {"accuracy",       required_argument, 0, OPT_ACCURACY},
        {"angle",          required_argument, 0, OPT_ANGLE},
        {"color_scheme",   required_argument, 0, OPT_COLOR_SCHEME},
        {"random_seed",    required_argument, 0, OPT_RANDOM_SEED},
        {"lightness",      required_argument, 0, OPT_LIGHTNESS},
        {"share_endpoint", no_argument,       0, OPT_SHARE_ENDPOINT},
        {0, 0, 0, 0},
        // clang-format on
    };

    int option_index = 0;
    int c = getopt_long(argc, argv, "a:c:r:l:o:s:v?", opts, &option_index);

    if (c == -1) {
      break;
    }

    const char *arg = optarg;

    // legacy handling of single-dash-prefixed options
    if (c == 'a' && startswith(arg, "ccuracy=")) {
      c = OPT_ACCURACY;
      arg += strlen("ccuracy=");
    } else if (c == 'a' && startswith(arg, "ngle=")) {
      c = OPT_ANGLE;
      arg += strlen("ngle=");
    } else if (c == 'c' && startswith(arg, "olor_scheme=")) {
      c = OPT_COLOR_SCHEME;
      arg += strlen("olor_scheme=");
    } else if (c == 'r' && startswith(arg, "andom_seed=")) {
      c = OPT_RANDOM_SEED;
      arg += strlen("andom_seed=");
    } else if (c == 'l' && startswith(arg, "ightness=")) {
      c = OPT_LIGHTNESS;
      arg += strlen("ightness=");
    } else if (c == 's' && startswith(arg, "hare_endpoint")) {
      c = OPT_SHARE_ENDPOINT;
    }

    switch (c) {

    // any valid use of these options should have been handled as legacy above
    case 'a':
    case 'c':
    case 'r':
    case 'l':
      fprintf(stderr, "option -%c unrecognized.\n", c);
      usage(cmd, EXIT_FAILURE);
      UNREACHABLE();

    case '?':
      if (optopt == '\0' || optopt == '?') {
        usage(cmd, EXIT_SUCCESS);
      }
      fprintf(stderr, "option -%c unrecognized.\n", optopt);
      usage(cmd, EXIT_FAILURE);
      UNREACHABLE();

    case 'o':
      if (outfile != NULL) {
        fclose(outfile);
      }
      outfile = openFile(arg, CmdName);
      break;

    case 'v':
      Verbose = 1;
      break;

    case OPT_ACCURACY:
      if (sscanf(arg, "%lf", accuracy) != 1 || *accuracy <= 0) {
        fprintf(stderr, "--accuracy option must be a positive real number.\n");
        usage(cmd, EXIT_FAILURE);
      }
      break;

    case OPT_ANGLE:
      if (sscanf(arg, "%lf", angle) != 1 || *angle <= 0 || *angle >= 90) {
        fprintf(stderr, "--angle option must be a positive real number "
                        "between 0 and 90.\n");
        usage(cmd, EXIT_FAILURE);
      }
      break;

    case OPT_COLOR_SCHEME:
      if (!knownColorScheme(arg)) {
        fprintf(stderr,
                "--color_scheme option must be a known color scheme.\n");
        usage(cmd, EXIT_FAILURE);
      }
      break;

    case OPT_LIGHTNESS: {
      int l1 = 0;
      int l2 = 70;
      if (sscanf(arg, "%d,%d", &l1, &l2) != 2 || l1 < 0 || l2 > 100 ||
          l1 > l2) {
        fprintf(stderr, "invalid --lightness=%s option.\n", arg);
        usage(cmd, EXIT_FAILURE);
      }
      *lightness = optarg;
      break;
    }

    case OPT_RANDOM_SEED:
      if (sscanf(arg, "%d", seed) != 1) {
        fprintf(stderr, "--random_seed option must be an integer.\n");
        usage(cmd, EXIT_FAILURE);
      }
      break;

    case OPT_SHARE_ENDPOINT:
      *check_edges_with_same_endpoint = 1;
      break;

    default:
      UNREACHABLE();
    }
  }

  if (argc > optind) {
    Files = argv + optind;
  }

  if (outfile == NULL) {
    outfile = stdout;
  }
}


static int clarify(Agraph_t* g, double angle, double accuracy, int check_edges_with_same_endpoint, int seed, char *color_scheme, char *lightness){

  if (checkG(g)) {
    agerr (AGERR, "Graph %s contains loops or multiedges\n", agnameof(g));
    return 1;
  }

  initDotIO(g);
  g = edge_distinct_coloring(color_scheme, lightness, g, angle, accuracy, check_edges_with_same_endpoint, seed);
  if (!g) return 1;

  agwrite (g, stdout);
  return 0;
}

int main(int argc, char *argv[])
{
  double accuracy;
  double angle;
  int check_edges_with_same_endpoint, seed;
  char *color_scheme = NULL;
  char *lightness = NULL;
  Agraph_t *g;
  Agraph_t *prev = NULL;
  ingraph_state ig;
  int rv = EXIT_SUCCESS;

	init(argc, argv, &angle, &accuracy, &check_edges_with_same_endpoint, &seed, &color_scheme, &lightness);
	newIngraph(&ig, Files);

	while ((g = nextGraph(&ig)) != 0) {
		if (prev)
		    agclose(prev);
		prev = g;
		fname = fileName(&ig);
		if (Verbose)
		    fprintf(stderr, "Process graph %s in file %s\n", agnameof(g),
			    fname);
		if (clarify(g, angle, accuracy, check_edges_with_same_endpoint, seed,
		            color_scheme, lightness) != 0) {
		    rv = EXIT_FAILURE;
		}
	}

	graphviz_exit(rv);
}
