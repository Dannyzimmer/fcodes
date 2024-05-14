/**
 * @file
 * @brief main rendering program for various layouts of graphs and output formats
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
 * Written by Stephen North and Eleftherios Koutsofios.
 */

#include "config.h"

#include <cgraph/cgraph.h>
#include <cgraph/exit.h>
#include <gvc/gvc.h>
#include <gvc/gvio.h>

#include <common/globals.h>

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

static GVC_t *Gvc;
static graph_t * G;

#ifndef _WIN32
#ifndef NO_FPERR
static void fperr(int s)
{
    fprintf(stderr, "caught SIGFPE %d\n", s);
    /* signal (s, SIG_DFL); raise (s); */
    graphviz_exit(1);
}
#endif
#endif

int main(int argc, char **argv)
{
    graph_t *prev = NULL;
    int r, rc = 0;

    Gvc = gvContextPlugins(lt_preloaded_symbols, DEMAND_LOADING);
    GvExitOnUsage = 1;
    gvParseArgs(Gvc, argc, argv);
#ifndef _WIN32
    signal(SIGUSR1, gvToggle);
#ifndef NO_FPERR
    signal(SIGFPE, fperr);
#endif
#endif

    if (MemTest) {
        // TODO: fully remove `MemTest` and associated `-m` parsing in future
        fprintf(stderr, "The -m command-line option is no longer supported.\n");
    }
    else if ((G = gvPluginsGraph(Gvc))) {
	    gvLayoutJobs(Gvc, G);  /* take layout engine from command line */
	    gvRenderJobs(Gvc, G);
    }
    else {
	while ((G = gvNextInputGraph(Gvc))) {
	    if (prev) {
		gvFreeLayout(Gvc, prev);
		agclose(prev);
	    }
	    gvLayoutJobs(Gvc, G);  /* take layout engine from command line */
	    gvRenderJobs(Gvc, G);
	    r = agreseterrors();
	    rc = MAX(rc,r);
	    prev = G;
	}
    }
    gvFinalize(Gvc);
    r = gvFreeContext(Gvc);
    graphviz_exit(MAX(rc,r));
}

/**
 * @dir .
 * @brief main rendering program for various layouts of graphs and output formats
 */
