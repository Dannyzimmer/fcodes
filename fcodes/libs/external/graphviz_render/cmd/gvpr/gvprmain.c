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
 * gvpr: graph pattern recognizer
 *
 * Written by Emden Gansner
 */


#include "config.h"
#include <limits.h>
#include <stddef.h>
#include <stdio.h>

#include <cgraph/cgraph.h>
#include <cgraph/exit.h>
#include <gvpr/gvpr.h>

#ifdef DEBUG
static ssize_t outfn (void* sp, const char *buf, size_t nbyte, void* dp)
{
  if (nbyte > (size_t)INT_MAX) {
    return -1;
  }
  return printf("<stdout>%.*s", (int)nbyte, buf);
}

static ssize_t errfn (void* sp, const char *buf, size_t nbyte, void* dp)
{
  if (nbyte > (size_t)INT_MAX) {
    return -1;
  }
  return fprintf(stderr, "<stderr>%.*s", (int)nbyte, buf);
}

static int iofread(void *chan, char *buf, int bufsize)
{
  return (int)fread(buf, 1, (size_t)bufsize, chan);
}

static int ioputstr(void *chan, const char *str)
{
  return fputs(str, chan);
}

static int ioflush(void *chan)
{
  return fflush(chan);
}

static Agiodisc_t gprIoDisc = { iofread, ioputstr, ioflush };

static Agdisc_t gprDisc = { &AgIdDisc, &gprIoDisc };

int
main (int argc, char* argv[])
{
    Agraph_t* gs[2];
    Agraph_t* g = agread(stdin, &gprDisc);
    int rv;
    gvpropts opts;

    gs[0] = g;
    gs[1] = 0;
    opts.ingraphs = gs;
    opts.out = outfn;
    opts.err = errfn;
    opts.flags = GV_USE_OUTGRAPH;
    opts.bindings = 0;
    
    rv = gvpr (argc, argv, &opts);

    fprintf(stderr, "rv %d\n", rv);

    rv = gvpr (argc, argv, &opts);

    graphviz_exit(rv);
}

#else
int
main (int argc, char* argv[])
{
    gvpropts opts;
    opts.ingraphs = 0;
    opts.out = 0;
    opts.err = 0;
    opts.flags = GV_USE_EXIT;
    opts.bindings = 0;
    
    graphviz_exit(gvpr(argc, argv, &opts));
}

#endif

/**
 * @dir cmd/gvpr
 * @brief graph pattern scanning and processing language
 */
