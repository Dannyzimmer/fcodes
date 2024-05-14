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
 * gpr: graph pattern recognizer
 *
 * Written by Emden Gansner
 */

#include <assert.h>
#include <unistd.h>
#include "builddate.h"
#include <gvpr/gprstate.h>
#include <cgraph/agxbuf.h>
#include <cgraph/alloc.h>
#include <cgraph/cgraph.h>
#include <cgraph/exit.h>
#include <cgraph/stack.h>
#include <common/globals.h>
#include <ingraphs/ingraphs.h>
#include <gvpr/compile.h>
#include <gvpr/queue.h>
#include <gvpr/gvpr.h>
#include <gvpr/actions.h>
#include <ast/error.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <getopt.h>

#ifndef DFLT_GVPRPATH
#define DFLT_GVPRPATH    "."
#endif

static char *Info[] = {
    "gvpr",                     /* Program */
    PACKAGE_VERSION,            /* Version */
    BUILDDATE                   /* Build Date */
};

static const char *usage =
    " [-o <ofile>] [-a <args>] ([-f <prog>] | 'prog') [files]\n\
   -c         - use source graph for output\n\
   -f <pfile> - find program in file <pfile>\n\
   -i         - create node induced subgraph\n\
   -a <args>  - string arguments available as ARGV[0..]\n\
   -o <ofile> - write output to <ofile>; stdout by default\n\
   -n         - no read-ahead of input graphs\n\
   -q         - turn off warning messages\n\
   -V         - print version info\n\
   -?         - print usage info\n\
If no files are specified, stdin is used\n";

typedef struct {
    char *cmdName;              /* command name */
    FILE *outFile;		/* output stream; stdout default */
    char *program;              /* program source */
    int useFile;		/* true if program comes from a file */
    int compflags;
    int readAhead;
    char **inFiles;
    int argc;
    char **argv;
    int state;                  /* > 0 : continue; <= 0 finish */
    int verbose;
} options;

static FILE *openOut(char *name) {
    FILE *outs = fopen(name, "w");
    if (outs == 0) {
	error(ERROR_ERROR, "could not open %s for writing", name);
    }
    return outs;
}

/* gettok:
 * Tokenize a string. Tokens consist of either a non-empty string
 * of non-space characters, or all characters between a pair of
 * single or double quotes. As usual, we map 
 *   \c -> c
 * for all c
 * Return next argument token, returning NULL if none.
 * sp is updated to point to next character to be processed.
 * NB. There must be white space between tokens. Otherwise, they
 * are concatenated.
 */
static char *gettok(char **sp)
{
    char *s = *sp;
    char *ws = s;
    char *rs = s;
    char c;
    char q = '\0';		/* if non-0, in quote mode with quote char q */

    while (isspace((int)*rs))
	rs++;
    if ((c = *rs) == '\0')
	return NULL;
    while ((c = *rs)) {
	if (q && (q == c)) {	/* end quote */
	    q = '\0';
	} else if (!q && ((c == '"') || (c == '\''))) {
	    q = c;
	} else if (c == '\\') {
	    rs++;
	    c = *rs;
	    if (c)
		*ws++ = c;
	    else {
		error(ERROR_WARNING,
		      "backslash in argument followed by no character - ignored");
		rs--;
	    }
	} else if (q || !isspace(c))
	    *ws++ = c;
	else
	    break;
	rs++;
    }
    if (*rs)
	rs++;
    else if (q)
	error(ERROR_WARNING, "no closing quote for argument %s", s);
    *sp = rs;
    *ws = '\0';
    return s;
}

#define NUM_ARGS 100

/* parseArgs:
 * Split s into whitespace separated tokens, allowing quotes.
 * Append tokens to argument list and return new number of arguments.
 * argc is the current number of arguments, with the arguments
 * stored in *argv.
 */
static int parseArgs(char *s, int argc, char ***argv)
{
    int i, cnt = 0;
    char *args[NUM_ARGS];
    char *t;
    char **av;

    assert(argc >= 0);

    while ((t = gettok(&s))) {
	if (cnt == NUM_ARGS) {
	    error(ERROR_WARNING,
		  "at most %d arguments allowed per -a flag - ignoring rest",
		  NUM_ARGS);
	    break;
	}
	args[cnt++] = t;
    }

    if (cnt) {
	int oldcnt = argc;
	argc = oldcnt + cnt;
	av = gv_recalloc(*argv, (size_t)oldcnt, (size_t)argc, sizeof(char*));
	for (i = 0; i < cnt; i++)
	    av[oldcnt + i] = gv_strdup(args[i]);
	*argv = av;
    }
    return argc;
}


#if defined(_WIN32) && !defined(__MINGW32__)
#define PATHSEP '\\'
#define LISTSEP ';'
#else
#define PATHSEP '/'
#define LISTSEP ':'
#endif

static char*
concat (char* pfx, char* sfx)
{
  agxbuf sp = {0};
  agxbprint(&sp, "%s%s", pfx, sfx);
  return agxbdisown(&sp);
}

/* resolve:
 * Translate -f arg parameter into a pathname.
 * If arg contains '/', return arg.
 * Else search directories in GVPRPATH for arg.
 * Return NULL on error.
 * 
 * FIX - use pathinclude/pathfind
 */
static char *resolve(char *arg, int verbose) {
    char *path;
    char *s;
    char *cp;
    char c;
    char *fname = 0;
    char *pathp = NULL;
    size_t sz;

    if (strchr(arg, PATHSEP))
	return gv_strdup(arg);

    path = getenv("GVPRPATH");
    if (!path)
	path = getenv("GPRPATH");  // deprecated
    if (path && (c = *path)) {
	if (c == LISTSEP) {
	    pathp = path = concat(DFLT_GVPRPATH, path);
	}
	else if ((c = path[strlen(path)-1]) == LISTSEP) {
	    pathp = path = concat(path, DFLT_GVPRPATH);
	}
    }
    else
	path = DFLT_GVPRPATH;
    if (verbose)
	fprintf (stderr, "PATH: %s\n", path);
    agxbuf fp = {0};

    while (*path && !fname) {
	if (*path == LISTSEP) {	/* skip colons */
	    path++;
	    continue;
	}
	cp = strchr(path, LISTSEP);
	if (cp) {
	    sz = (size_t) (cp - path);
	    agxbput_n(&fp, path, sz);
	    path = cp + 1;	/* skip past current colon */
	} else {
	    sz = agxbput(&fp, path);
	    path += sz;
	}
	agxbprint(&fp, "%c%s", PATHSEP, arg);
	s = agxbuse(&fp);

	if (access(s, R_OK) == 0) {
	    fname = gv_strdup(s);
	}
    }

    if (!fname)
	error(ERROR_ERROR, "Could not find file \"%s\" in GVPRPATH", arg);

    agxbfree(&fp);
    free(pathp);
    if (verbose)
	fprintf (stderr, "file %s resolved to %s\n", arg, fname);
    return fname;
}

static char*
getOptarg (int c, char** argp, int* argip, int argc, char** argv)
{
    char* rv; 
    char* arg = *argp;
    int argi = *argip;

    if (*arg) {
	rv = arg;
	while (*arg) arg++; 
	*argp = arg;
    }
    else if (argi < argc) {
	rv = argv[argi++];
	*argip = argi;
    } 
    else {
	rv = NULL;
	error(ERROR_WARNING, "missing argument for option -%c", c);
    }
    return rv;
}

/* doFlags:
 * Process a command-line argument starting with a '-'.
 * argi is the index of the next available item in argv[].
 * argc has its usual meaning.
 *
 * return > 0 given next argi value
 *        = 0 for exit with 0
 *        < 0 for error
 */
static int
doFlags(char* arg, int argi, int argc, char** argv, options* opts)
{
    int c;

    while ((c = *arg++)) {
	switch (c) {
	case 'c':
	    opts->compflags |= SRCOUT;
	    break;
	case 'C':
	    opts->compflags |= (SRCOUT|CLONE);
	    break;
	case 'f':
	    if ((optarg = getOptarg(c, &arg, &argi, argc, argv)) && (opts->program = resolve(optarg, opts->verbose))) {
		opts->useFile = 1;
	    }
	    else return -1;
	    break;
	case 'i':
	    opts->compflags |= INDUCE;
	    break;
	case 'n':
	    opts->readAhead = 0;
	    break;
	case 'a':
	    if ((optarg = getOptarg(c, &arg, &argi, argc, argv))) {
		opts->argc = parseArgs(optarg, opts->argc, &(opts->argv));
	    }
	    else return -1;
	    break;
	case 'o':
	    if (!(optarg = getOptarg(c, &arg, &argi, argc, argv)) || !(opts->outFile = openOut(optarg)))
		return -1;
	    break;
	case 'q':
	    setTraceLevel (ERROR_ERROR);  /* Don't emit warning messages */
	    break;
	case 'v':
	    opts->verbose = 1;
	    break;
	case 'V':
	    fprintf(stderr, "%s version %s (%s)\n", Info[0], Info[1], Info[2]);
	    return 0;
	    break;
	case '?':
	    if (optopt == '\0' || optopt == '?')
		fprintf(stderr, "Usage: gvpr%s", usage);
	    else {
		error(ERROR_USAGE|ERROR_WARNING, "%s", usage);
	    }
	    return 0;
	    break;
	default :
	    error(ERROR_WARNING, "option -%c unrecognized", c);
	    break;
	}
    }
    return argi;
}

static void freeOpts(options opts) {
    if (opts.outFile != NULL && opts.outFile != stdout)
	fclose(opts.outFile);
    free(opts.inFiles);
    if (opts.useFile)
	free(opts.program);
    for (int i = 0; i < opts.argc; i++)
	free(opts.argv[i]);
    free(opts.argv);
}

/* scanArgs:
 * Parse command line options.
 */
static options scanArgs(int argc, char **argv) {
    int i, nfiles;
    char* arg;
    options opts = {0};

    opts.cmdName = argv[0];
    opts.state = 1;
    opts.readAhead = 1;
    setErrorId(opts.cmdName);
    opts.verbose = 0;

    /* estimate number of file names */
    nfiles = 0;
    for (i = 1; i < argc; i++)
	if (argv[i] && argv[i][0] != '-')
	    nfiles++;
    char** input_filenames = gv_calloc(nfiles + 1, sizeof(char*));

    /* loop over arguments */
    nfiles = 0;
    for (i = 1; i < argc; ) {
	arg = argv[i++];
	if (*arg == '-') {
	    i = doFlags(arg+1, i, argc, argv, &opts);
	    if (i <= 0) {
		opts.state = i;
		goto opts_done;
	    }
	} else if (arg)
	    input_filenames[nfiles++] = arg;
    }

    /* Handle additional semantics */
    if (opts.useFile == 0) {
	if (nfiles == 0) {
	    error(ERROR_ERROR,
		  "No program supplied via argument or -f option");
	    opts.state = -1;
	} else {
	    opts.program = input_filenames[0];
	    for (i = 1; i <= nfiles; i++)
		input_filenames[i-1] = input_filenames[i];
	    nfiles--;
	}
    }
    if (nfiles == 0) {
	opts.inFiles = 0;
	free (input_filenames);
	input_filenames = 0;
    }
    else
	opts.inFiles = input_filenames;

    if (!opts.outFile)
	opts.outFile = stdout;

  opts_done:
    if (opts.state <= 0) {
	if (opts.state < 0)
	    error(ERROR_USAGE|ERROR_ERROR, "%s", usage);
	free (input_filenames);
    }

    return opts;
}

static Agobj_t* evalEdge(Gpr_t * state, Expr_t* prog, comp_block * xprog, Agedge_t * e)
{
    int i;
    case_stmt *cs;
    bool okay;

    state->curobj = (Agobj_t *) e;
    for (i = 0; i < xprog->n_estmts; i++) {
	cs = xprog->edge_stmts + i;
	if (cs->guard)
	    okay = exeval(prog, cs->guard, state).integer != 0;
	else
	    okay = true;
	if (okay) {
	    if (cs->action)
		exeval(prog, cs->action, state);
	    else
		agsubedge(state->target, e, 1);
	}
    }
    return state->curobj;
}

static Agobj_t* evalNode(Gpr_t * state, Expr_t* prog, comp_block * xprog, Agnode_t * n)
{
    int i;
    case_stmt *cs;
    bool okay;

    state->curobj = (Agobj_t *) n;
    for (i = 0; i < xprog->n_nstmts; i++) {
	cs = xprog->node_stmts + i;
	if (cs->guard)
	    okay = exeval(prog, cs->guard, state).integer != 0;
	else
	    okay = true;
	if (okay) {
	    if (cs->action)
		exeval(prog, cs->action, state);
	    else
		agsubnode(state->target, n, 1);
	}
    }
    return (state->curobj);
}

typedef struct {
    Agnode_t *oldroot;
    Agnode_t *prev;
} nodestream;

static Agnode_t *nextNode(Gpr_t * state, nodestream * nodes)
{
    Agnode_t *np;

    if (state->tvroot != nodes->oldroot) {
	np = nodes->oldroot = state->tvroot;
    } else if (state->flags & GV_NEXT_SET) {
	np = nodes->oldroot = state->tvroot = state->tvnext;
	state->flags &= ~GV_NEXT_SET;
    } else if (nodes->prev) {
	np = nodes->prev = agnxtnode(state->curgraph, nodes->prev);
    } else {
	np = nodes->prev = agfstnode(state->curgraph);
    }
    return np;
}

#define MARKED(x)  (((x)->iu.integer)&1)
#define MARK(x)  (((x)->iu.integer) = 1)
#define ONSTACK(x)  (((x)->iu.integer)&2)
#define PUSH(x,e)  (((x)->iu.integer)|=2,(x)->ine=(e))
#define POP(x)  (((x)->iu.integer)&=(~2))

typedef Agedge_t *(*fstedgefn_t) (Agraph_t *, Agnode_t *);
typedef Agedge_t *(*nxttedgefn_t) (Agraph_t *, Agedge_t *, Agnode_t *);

#define PRE_VISIT 1
#define POST_VISIT 2

typedef struct {
    fstedgefn_t fstedge;
    nxttedgefn_t nxtedge;
    unsigned char undirected;
    unsigned char visit;
} trav_fns;

static trav_fns DFSfns = { agfstedge, agnxtedge, 1, 0 };
static trav_fns FWDfns = { agfstout, (nxttedgefn_t) agnxtout, 0, 0 };
static trav_fns REVfns = { agfstin, (nxttedgefn_t) agnxtin, 0, 0 };

static void travBFS(Gpr_t * state, Expr_t* prog, comp_block * xprog)
{
    nodestream nodes;
    queue *q;
    ndata *nd;
    Agnode_t *n;
    Agedge_t *cure;
    Agedge_t *nxte;
    Agraph_t *g = state->curgraph;

    q = mkQueue();
    nodes.oldroot = 0;
    nodes.prev = 0;
    while ((n = nextNode(state, &nodes))) {
	nd = nData(n);
	if (MARKED(nd))
	    continue;
	PUSH(nd, 0);
	push(q, n);
	while ((n = pull(q))) {
	    nd = nData(n);
	    MARK(nd);
	    POP(nd);
	    state->tvedge = nd->ine;
	    if (!evalNode(state, prog, xprog, n)) continue;
	    for (cure = agfstedge(g, n); cure; cure = nxte) {
		nxte = agnxtedge(g, cure, n);
		nd = nData(cure->node);
		if (MARKED(nd))
		    continue;
		if (!evalEdge(state, prog, xprog, cure)) continue;
		if (!ONSTACK(nd)) {
		    push(q, cure->node);
		    PUSH(nd,cure);
		}
	    }
	}
    }
    state->tvedge = 0;
    freeQ(q);
}

static void travDFS(Gpr_t * state, Expr_t* prog, comp_block * xprog, trav_fns * fns)
{
    Agnode_t *n;
    gv_stack_t stk = {0};
    Agnode_t *curn;
    Agedge_t *cure;
    Agedge_t *entry;
    int more;
    ndata *nd;
    nodestream nodes;
    Agedgepair_t seed;

    nodes.oldroot = 0;
    nodes.prev = 0;
    while ((n = nextNode(state, &nodes))) {
	nd = nData(n);
	if (MARKED(nd))
	    continue;
	seed.out.node = n;
	seed.in.node = 0;
	curn = n;
	entry = &(seed.out);
	state->tvedge = cure = 0;
	MARK(nd);
	PUSH(nd,0);
	if (fns->visit & PRE_VISIT)
	    evalNode(state, prog, xprog, n);
	more = 1;
	while (more) {
	    if (cure)
		cure = fns->nxtedge(state->curgraph, cure, curn);
	    else
		cure = fns->fstedge(state->curgraph, curn);
	    if (cure) {
		if (entry == agopp(cure))	/* skip edge used to get here */
		    continue;
		nd = nData(cure->node);
		if (MARKED(nd)) {
		    /* For undirected DFS, visit an edge only if its head
		     * is on the stack, to avoid visiting it twice.
		     * This is no problem in directed DFS.
		     */
		    if (fns->undirected) {
			if (ONSTACK(nd))
			    evalEdge(state, prog, xprog, cure);
		    } else
			evalEdge(state, prog, xprog, cure);
		} else {
		    evalEdge(state, prog, xprog, cure);
		    stack_push(&stk, entry);
		    state->tvedge = entry = cure;
		    curn = cure->node;
		    cure = 0;
		    if (fns->visit & PRE_VISIT)
			evalNode(state, prog, xprog, curn);
		    MARK(nd);
		    PUSH(nd, entry);
		}
	    } else {
		if (fns->visit & POST_VISIT)
		    evalNode(state, prog, xprog, curn);
		nd = nData(curn);
		POP(nd);
		cure = entry;
		entry = stack_is_empty(&stk) ? NULL : stack_pop(&stk);
		if (entry == &(seed.out))
		    state->tvedge = 0;
		else
		    state->tvedge = entry;
		if (entry)
		    curn = entry->node;
		else
		    more = 0;
	    }
	}
    }
    state->tvedge = 0;
    stack_reset(&stk);
}

static void travNodes(Gpr_t * state, Expr_t* prog, comp_block * xprog)
{
    Agnode_t *n;
    Agnode_t *next;
    Agraph_t *g = state->curgraph;
    for (n = agfstnode(g); n; n = next) {
	next =  agnxtnode(g, n);
	evalNode(state, prog, xprog, n);
    }
}

static void travEdges(Gpr_t * state, Expr_t* prog, comp_block * xprog)
{
    Agnode_t *n;
    Agnode_t *next;
    Agedge_t *e;
    Agedge_t *nexte;
    Agraph_t *g = state->curgraph;
    for (n = agfstnode(g); n; n = next) {
	next = agnxtnode(g, n);
	for (e = agfstout(g, n); e; e = nexte) {
	    nexte = agnxtout(g, e);
	    evalEdge(state, prog, xprog, e);
	}
    }
}

static void travFlat(Gpr_t * state, Expr_t* prog, comp_block * xprog)
{
    Agnode_t *n;
    Agnode_t *next;
    Agedge_t *e;
    Agedge_t *nexte;
    Agraph_t *g = state->curgraph;
    for (n = agfstnode(g); n; n = next) {
	next =  agnxtnode(g, n);
	if (!evalNode(state, prog, xprog, n)) continue;
	if (xprog->n_estmts > 0) {
	    for (e = agfstout(g, n); e; e = nexte) {
		nexte = agnxtout(g, e);
		evalEdge(state, prog, xprog, e);
	    }
	}
    }
}

/* doCleanup:
 * Reset node traversal data
 */
static void doCleanup (Agraph_t* g)
{
    Agnode_t *n;
    ndata *nd;

    for (n = agfstnode(g); n; n =  agnxtnode(g, n)) {
	nd = nData(n);
	nd->ine = NULL;
	nd->iu.integer = 0;
    }
}

/* traverse:
 * return 1 if traversal requires cleanup
 */
static int traverse(Gpr_t * state, Expr_t* prog, comp_block * bp, int cleanup)
{
    if (!state->target) {
	char *target;
	agxbuf tmp = {0};

	if (state->name_used) {
	    agxbprint(&tmp, "%s%d", state->tgtname, state->name_used);
	    target = agxbuse(&tmp);
	} else
	    target = state->tgtname;
	state->name_used++;
	/* make sure target subgraph does not exist */
	while (agsubg (state->curgraph, target, 0)) {
	    state->name_used++;
	    agxbprint(&tmp, "%s%d", state->tgtname, state->name_used);
	    target = agxbuse(&tmp);
	}
	state->target = openSubg(state->curgraph, target);
	agxbfree(&tmp);
    }
    if (!state->outgraph)
	state->outgraph = state->target;

    switch (state->tvt) {
    case TV_flat:
	travFlat(state, prog, bp);
	break;
    case TV_bfs:
	if (cleanup) doCleanup (state->curgraph);
	travBFS(state, prog, bp);
	cleanup = 1;
	break;
    case TV_dfs:
	if (cleanup) doCleanup (state->curgraph);
	DFSfns.visit = PRE_VISIT;
	travDFS(state, prog, bp, &DFSfns);
	cleanup = 1;
	break;
    case TV_fwd:
	if (cleanup) doCleanup (state->curgraph);
	FWDfns.visit = PRE_VISIT;
	travDFS(state, prog, bp, &FWDfns);
	cleanup = 1;
	break;
    case TV_rev:
	if (cleanup) doCleanup (state->curgraph);
	REVfns.visit = PRE_VISIT;
	travDFS(state, prog, bp, &REVfns);
	cleanup = 1;
	break;
    case TV_postdfs:
	if (cleanup) doCleanup (state->curgraph);
	DFSfns.visit = POST_VISIT;
	travDFS(state, prog, bp, &DFSfns);
	cleanup = 1;
	break;
    case TV_postfwd:
	if (cleanup) doCleanup (state->curgraph);
	FWDfns.visit = POST_VISIT;
	travDFS(state, prog, bp, &FWDfns);
	cleanup = 1;
	break;
    case TV_postrev:
	if (cleanup) doCleanup (state->curgraph);
	REVfns.visit = POST_VISIT;
	travDFS(state, prog, bp, &REVfns);
	cleanup = 1;
	break;
    case TV_prepostdfs:
	if (cleanup) doCleanup (state->curgraph);
	DFSfns.visit = POST_VISIT | PRE_VISIT;
	travDFS(state, prog, bp, &DFSfns);
	cleanup = 1;
	break;
    case TV_prepostfwd:
	if (cleanup) doCleanup (state->curgraph);
	FWDfns.visit = POST_VISIT | PRE_VISIT;
	travDFS(state, prog, bp, &FWDfns);
	cleanup = 1;
	break;
    case TV_prepostrev:
	if (cleanup) doCleanup (state->curgraph);
	REVfns.visit = POST_VISIT | PRE_VISIT;
	travDFS(state, prog, bp, &REVfns);
	cleanup = 1;
	break;
    case TV_ne:
	travNodes(state, prog, bp);
	travEdges(state, prog, bp);
	break;
    case TV_en:
	travEdges(state, prog, bp);
	travNodes(state, prog, bp);
	break;
    }
    return cleanup;
}

/* addOutputGraph:
 * Append output graph to option struct.
 * We know uopts and state->outgraph are non-NULL.
 */
static void
addOutputGraph (Gpr_t* state, gvpropts* uopts)
{
    Agraph_t* g = state->outgraph;

    if ((agroot(g) == state->curgraph) && !uopts->ingraphs)
	g = (Agraph_t*)cloneO (0, (Agobj_t *)g);

    uopts->outgraphs = gv_recalloc(uopts->outgraphs, (size_t)uopts->n_outgraphs,
                                   (size_t)uopts->n_outgraphs + 1,
                                   sizeof(Agraph_t*));
    uopts->n_outgraphs++;
    uopts->outgraphs[uopts->n_outgraphs-1] = g;
}

static void chkClose(Agraph_t * g)
{
    gdata *data;

    data = gData(g);
    if (data->lock & 1)
	data->lock |= 2;
    else
	agclose(g);
}

static Agraph_t *ing_read(void *fp)
{
    return readG(fp);
}

static jmp_buf jbuf;

/* gvexitf:
 * Only used if GV_USE_EXIT not set during exeval.
 * This implies setjmp/longjmp set up.
 */
static void 
gvexitf (Expr_t *handle, Exdisc_t *discipline, int v)
{
    (void)handle;
    (void)discipline;

    longjmp (jbuf, v);
}

static int 
gverrorf (Expr_t *handle, Exdisc_t *discipline, int level, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    errorv((discipline
	    && handle) ? *((char **) handle) : (char *) handle, level, fmt, ap);
    va_end(ap);

    if (level >= ERROR_ERROR) {
	Gpr_t *state = (Gpr_t*)(discipline->user);
	if (state->flags & GV_USE_EXIT)
            graphviz_exit(1);
	else if (state->flags & GV_USE_JUMP)
	    longjmp (jbuf, 1);
    }

    return 0;
}

/// collective managed state used in \p gvpr_core
typedef struct {
  parse_prog *prog;
  ingraph_state *ing;
  comp_prog *xprog;
  Gpr_t *state;
  options opts;
} gvpr_state_t;

/* gvpr_core:
 * Return 0 on success; non-zero on error.
 *
 * FIX/TODO:
 *  - close non-source/non-output graphs
 *  - flag to clone target graph?
 *  - remove assignment in boolean warning if wrapped in ()
 *  - do automatic cast for array indices if type is known
 *  - array initialization
 */
static int gvpr_core(int argc, char *argv[], gvpropts *uopts,
                     gvpr_state_t *gs) {
    gpr_info info;
    int rv = 0;
    int cleanup, i, incoreGraphs;

    setErrorErrors (0);

    gs->opts = scanArgs(argc, argv);
    if (gs->opts.state <= 0) {
	return gs->opts.state;
    }

    if (gs->opts.verbose)
	gvstart_timer ();
    gs->prog = parseProg(gs->opts.program, gs->opts.useFile);
    if (gs->prog == NULL) {
	return 1;
    }
    info.outFile = gs->opts.outFile;
    info.argc = gs->opts.argc;
    info.argv = gs->opts.argv;
    info.errf = gverrorf;
    if (uopts) 
	info.flags = uopts->flags; 
    else
	info.flags = 0;
    if ((uopts->flags & GV_USE_EXIT))
	info.exitf = 0;
    else
	info.exitf = gvexitf;
    gs->state = openGPRState(&info);
    if (gs->state == NULL) {
	return 1;
    }
    if (uopts->bindings)
	addBindings(gs->state, uopts->bindings);
    gs->xprog = compileProg(gs->prog, gs->state, gs->opts.compflags);
    if (gs->xprog == NULL) {
	return 1;
    }

    initGPRState(gs->state, gs->xprog->prog->vm);
    
    if ((uopts->flags & GV_USE_OUTGRAPH)) {
	uopts->outgraphs = 0;
	uopts->n_outgraphs = 0;
    }

    if (!(uopts->flags & GV_USE_EXIT)) {
	gs->state->flags |= GV_USE_JUMP;
	if ((rv = setjmp (jbuf))) {
	    return rv;
	}
    }

    if (uopts && uopts->ingraphs)
	incoreGraphs = 1;
    else
	incoreGraphs = 0;

    if (gs->opts.verbose)
	fprintf(stderr, "Parse/compile/init: %.2f secs.\n", gvelapsed_sec());
    /* do begin */
    if (gs->xprog->begin_stmt != NULL)
	exeval(gs->xprog->prog, gs->xprog->begin_stmt, gs->state);

    /* if program is not null */
    if (usesGraph(gs->xprog)) {
	if (uopts && uopts->ingraphs)
	    gs->ing = newIngGraphs(0, uopts->ingraphs, ing_read);
	else
	    gs->ing = newIng(0, gs->opts.inFiles, ing_read);
	
	if (gs->opts.verbose) gvstart_timer ();
	Agraph_t* nextg = NULL;
	for (gs->state->curgraph = nextGraph(gs->ing); gs->state->curgraph;
	     gs->state->curgraph = nextg) {
	    if (gs->opts.verbose) fprintf(stderr, "Read graph: %.2f secs.\n", gvelapsed_sec());
	    gs->state->infname = fileName(gs->ing);
	    if (gs->opts.readAhead)
		nextg = gs->state->nextgraph = nextGraph(gs->ing);
	    cleanup = 0;

	    for (i = 0; i < gs->xprog->n_blocks; i++) {
		comp_block* bp = gs->xprog->blocks + i;

		/* begin graph */
		if (incoreGraphs && (gs->opts.compflags & CLONE))
		    gs->state->curgraph = (Agraph_t*)cloneO(0, (Agobj_t*)gs->state->curgraph);
		gs->state->curobj = (Agobj_t*)gs->state->curgraph;
		gs->state->tvroot = 0;
		if (bp->begg_stmt)
		    exeval(gs->xprog->prog, bp->begg_stmt, gs->state);

		/* walk graph */
		if (walksGraph(bp)) {
		    cleanup = traverse(gs->state, gs->xprog->prog, bp, cleanup);
		}
	    }

	    /* end graph */
	    gs->state->curobj = (Agobj_t*)gs->state->curgraph;
	    if (gs->xprog->endg_stmt != NULL)
		exeval(gs->xprog->prog, gs->xprog->endg_stmt, gs->state);
	    if (gs->opts.verbose) fprintf(stderr, "Finish graph: %.2f secs.\n", gvelapsed_sec());

	    /* if $O == $G and $T is empty, delete $T */
	    if (gs->state->outgraph == gs->state->curgraph &&
		gs->state->target != NULL && !agnnodes(gs->state->target))
		agdelete(gs->state->curgraph, gs->state->target);

	    /* output graph, if necessary
	     * For this, the outgraph must be defined, and either
	     * be non-empty or the -c option was used.
	     */
	    if (gs->state->outgraph != NULL && (agnnodes(gs->state->outgraph)
				    || (gs->opts.compflags & SRCOUT))) {
		if (uopts && (uopts->flags & GV_USE_OUTGRAPH))
		    addOutputGraph(gs->state, uopts);
		else
		    sfioWrite(gs->state->outgraph, gs->opts.outFile);
	    }

	    if (!incoreGraphs)
		chkClose(gs->state->curgraph);
	    gs->state->target = 0;
	    gs->state->outgraph = 0;
	
	    if (gs->opts.verbose) gvstart_timer ();
	    if (!gs->opts.readAhead)
		nextg = nextGraph(gs->ing);
	    if (gs->opts.verbose && nextg != NULL) {
		fprintf(stderr, "Read graph: %.2f secs.\n", gvelapsed_sec());
	    }
	}
    }

	/* do end */
    gs->state->curgraph = 0;
    gs->state->curobj = 0;
    if (gs->xprog->end_stmt != NULL)
	exeval(gs->xprog->prog, gs->xprog->end_stmt, gs->state);

    return 0;
}

/** main loop for gvpr
 *
 * \return 0 on success
 */
int gvpr(int argc, char *argv[], gvpropts *uopts) {
  gvpr_state_t gvpr_state = {0};

  // initialize opts to something that makes freeOpts() a no-op if we fail early
  gvpr_state.opts.outFile = stdout;

  int rv = gvpr_core(argc, argv, uopts, &gvpr_state);

  // free all allocated resources
  freeParseProg(gvpr_state.prog);
  freeCompileProg(gvpr_state.xprog);
  closeGPRState(gvpr_state.state);
  if (gvpr_state.ing != NULL) {
    closeIngraph(gvpr_state.ing);
  }
  freeOpts(gvpr_state.opts);

  return rv;
}

/**
 * @dir lib/gvpr
 * @brief graph pattern scanning and processing language, API gvpr.h
 */
