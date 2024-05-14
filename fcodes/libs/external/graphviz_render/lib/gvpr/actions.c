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
 *  Code for main functions in gpr
 */

#include <gvpr/actions.h>
#include <ast/error.h>
#include <ast/ast.h>
#include <gvpr/compile.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <cgraph/agxbuf.h>
#include <cgraph/alloc.h>
#include <cgraph/likely.h>
#include <cgraph/strcasecmp.h>
#include <cgraph/unreachable.h>
#include <cgraph/unused.h>
#include <unistd.h>

#define KINDS(p) ((AGTYPE(p) == AGRAPH) ? "graph" : (AGTYPE(p) == AGNODE) ? "node" : "edge")

static int iofread(void *chan, char *buf, int bufsize) {
  return (int)read(fileno(chan), buf, bufsize);
}

static int ioputstr(void *chan, const char *str) {
  return fputs(str, chan);
}

static int ioflush(void *chan) {
  return fflush(chan);
}

static Agiodisc_t gprIoDisc = { iofread, ioputstr, ioflush };

/* sameG:
 * Return common root if objects belong to same root graph.
 * NULL otherwise
 */
Agraph_t *sameG(void *p1, void *p2, char *fn, char *msg)
{
    Agobj_t *obj1 = OBJ(p1);
    Agobj_t *obj2 = OBJ(p2);
    Agraph_t *root;

    root = agroot(agraphof(obj1));
    if (root != agroot(agraphof(obj2))) {
	if (msg)
	    error(ERROR_WARNING, "%s in %s() belong to different graphs",
		  msg, fn);
	else
	    error(ERROR_WARNING,
		  "%s and %s in %s() belong to different graphs",
		  KINDS(obj1), KINDS(obj2), fn);
	return 0;
    } else
	return root;
}

/* indexOf:
 * Return index of leftmost string s2 in string s1, or -1
 */
int indexOf(char *s1, char *s2)
{
    char *s = strstr(s1, s2);
    return s == NULL ? -1 : (int)(s - s1);
}

/* rindexOf:
 * Return index of rightmost string s2 in string s1, or -1
 */
long rindexOf(char *s1, char *s2)
{
    char c1 = *s2;
    char *p;
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);

    if (c1 == '\0') {
	assert(len1 <= LONG_MAX);
	return (long)len1;
    }
    if (len2 > len1)
	return -1;
    p = s1 + (len1 - len2);
    while (p >= s1) {
	if (strncmp(p, s2, len2) == 0)
	    return p - s1;
	else
	    p--;
    }
    return -1;
}

/* match:
 * Return index of pattern pat in string str, or -1
 */
int match(char *str, char *pat)
{
    int sub[2];

    if (strgrpmatch(str, pat, sub, 1, STR_MAXIMAL)) {
	return (sub[0]);
    } else
	return -1;
}

/* nodeInduce:
 * Add all edges in root graph connecting two nodes in 
 * selected to selected.
 */
void nodeInduce(Agraph_t * selected)
{
    Agnode_t *n;
    Agedge_t *e;
    Agraph_t *base;

    if (!selected)
	return;
    base = agroot(selected);
    if (base == selected)
	return;
    for (n = agfstnode(selected); n; n = agnxtnode(selected, n)) {
	for (e = agfstout(base, n); e; e = agnxtout(base, e)) {
	    if (agsubnode(selected, aghead(e), 0))
		agsubedge(selected, e, 1);
	}
    }
}

/* copyAttr:
 * Copy attributes from src to tgt. Overrides currently
 * defined values.
 * FIX: we should probably use the default value of the source
 * graph when initializing the attribute, rather than "".
 * NOTE: We do not assume src and tgt have the same kind.
 */
int copyAttr(Agobj_t * src, Agobj_t * tgt)
{
    Agraph_t *srcg;
    Agraph_t *tgtg;
    Agsym_t *sym = 0;
    Agsym_t *tsym = 0;
    int skind = AGTYPE(src);
    int tkind = AGTYPE(tgt);
    char* val;

    srcg = agraphof(src);
    tgtg = agraphof(tgt);
    while ((sym = agnxtattr(srcg, skind, sym))) {
	tsym = agattrsym(tgt, sym->name);
	if (!tsym)
	    tsym = agattr(tgtg, tkind, sym->name, sym->defval);
	val = agxget(src, sym);
	if (aghtmlstr (val)) {
	    val = agstrdup_html (tgtg, val);
	    agxset(tgt, tsym, val);
	    agstrfree (tgtg, val);
	}
	else
	    agxset(tgt, tsym, val);
    }
    return 0;
}

/* copy:
 * Create new object of type AGTYPE(obj) with all of its
 * attributes.
 * If obj is an edge, only create end nodes if necessary.
 * If obj is a graph, if g is null, create a top-level
 * graph. Otherwise, create a subgraph of g.
 * Assume obj != NULL.
 */
Agobj_t *copy(Agraph_t * g, Agobj_t * obj)
{
    Agobj_t *nobj = 0;
    Agedge_t *e;
    Agnode_t *h;
    Agnode_t *t;
    int kind = AGTYPE(obj);
    char *name;

    if (kind != AGRAPH && !g) {
	exerror("NULL graph with non-graph object in copy()");
	return 0;
    }

    switch (kind) {
    case AGNODE:
	name = agnameof(obj);
	nobj = (Agobj_t *) openNode(g, name);
	break;
    case AGRAPH:
	name = agnameof(obj);
	if (g)
	    nobj = (Agobj_t *) openSubg(g, name);
	else
	    nobj = (Agobj_t *) openG(name, ((Agraph_t *) obj)->desc);
	break;
    case AGINEDGE:
    case AGOUTEDGE:
	e = (Agedge_t *) obj;
	t = openNode(g, agnameof(agtail(e)));
	h = openNode(g, agnameof(aghead(e)));
	name = agnameof (AGMKOUT(e));
	nobj = (Agobj_t *) openEdge(g, t, h, name);
	break;
    default:
	UNREACHABLE();
    }
    if (nobj)
	copyAttr(obj, nobj);

    return nobj;
}

typedef struct {
    Dtlink_t link;
    Agedge_t *key;
    Agedge_t *val;
} edgepair_t;

static Agedge_t*
mapEdge (Dt_t* emap, Agedge_t* e)
{
     edgepair_t* ep = dtmatch (emap, &e);     
     if (ep) return ep->val;
     else return NULL;
}

/* cloneSubg:
 * Clone subgraph sg in tgt.
 */
static Agraph_t *cloneSubg(Agraph_t * tgt, Agraph_t * g, Dt_t* emap)
{
    Agraph_t *ng;
    Agraph_t *sg;
    Agnode_t *t;
    Agnode_t *newt;
    Agedge_t *e;
    Agedge_t *newe;
    char* name;

    ng = (Agraph_t *) copy(tgt, OBJ(g));
    if (!ng)
	return 0;
    for (t = agfstnode(g); t; t = agnxtnode(g, t)) {
	newt = agnode(tgt, agnameof(t), 0);
	if (!newt) {
	    exerror("node %s not found in cloned graph %s",
		  agnameof(t), agnameof(tgt));
	    return 0;
	}
	else
	    agsubnode(ng, newt, 1);
    }
    for (t = agfstnode(g); t; t = agnxtnode(g, t)) {
	for (e = agfstout(g, t); e; e = agnxtout(g, e)) {
	    newe = mapEdge (emap, e);
	    if (!newe) {
		name = agnameof(AGMKOUT(e));
		if (name)
		    exerror("edge (%s,%s)[%s] not found in cloned graph %s",
		      agnameof(agtail(e)), agnameof(aghead(e)),
		      name, agnameof(tgt));
		else
		    exerror("edge (%s,%s) not found in cloned graph %s",
		      agnameof(agtail(e)), agnameof(aghead(e)),
		      agnameof(tgt));
		return 0;
	    }
	    else
		agsubedge(ng, newe, 1);
	}
    }
    for (sg = agfstsubg(g); sg; sg = agnxtsubg(sg)) {
	if (!cloneSubg(ng, sg, emap)) {
	    exerror("error cloning subgraph %s from graph %s",
		  agnameof(sg), agnameof(g));
	    return 0;
	}
    }
    return ng;
}

static int cmppair(Dt_t * d, Agedge_t** key1, Agedge_t** key2, Dtdisc_t * disc)
{
    (void)d;
    (void)disc;

    if (*key1 > *key2) return 1;
    else if (*key1 < *key2) return -1;
    else return 0;
}

static Dtdisc_t edgepair = {
    .key = offsetof(edgepair_t, key),
    .size = sizeof(Agedge_t *),
    .link = offsetof(edgepair_t, link),
    .comparf = (Dtcompar_f)cmppair,
};

/* cloneGraph:
 * Clone node, edge and subgraph structure from src to tgt.
 */
static void cloneGraph(Agraph_t * tgt, Agraph_t * src)
{
    Agedge_t *e;
    Agedge_t *ne;
    Agnode_t *t;
    Agraph_t *sg;
    char* name;
    Dt_t* emap = dtopen (&edgepair, Dtoset);
    edgepair_t* data = malloc(sizeof(edgepair_t)*agnedges(src));
    edgepair_t* ep = data;

    for (t = agfstnode(src); t; t = agnxtnode(src, t)) {
	if (!copy(tgt, OBJ(t))) {
	    exerror("error cloning node %s from graph %s",
		  agnameof(t), agnameof(src));
	}
    }
    for (t = agfstnode(src); t; t = agnxtnode(src, t)) {
	for (e = agfstout(src, t); e; e = agnxtout(src, e)) {
	    if (!(ne = (Agedge_t*)copy(tgt, OBJ(e)))) {
		name = agnameof(AGMKOUT(e));
		if (name)
		    exerror("error cloning edge (%s,%s)[%s] from graph %s",
		      agnameof(agtail(e)), agnameof(aghead(e)),
		      name, agnameof(src));
		else
		    exerror("error cloning edge (%s,%s) from graph %s",
		      agnameof(agtail(e)), agnameof(aghead(e)),
		      agnameof(src));
		goto done;
	    }
	    ep->key = e;
	    ep->val = ne;
	    dtinsert (emap, ep++);
	}
    }
    for (sg = agfstsubg(src); sg; sg = agnxtsubg(sg)) {
	if (!cloneSubg(tgt, sg, emap)) {
	    exerror("error cloning subgraph %s from graph %s",
		  agnameof(sg), agnameof(src));
	}
    }

done:
    dtclose (emap);
    free (data);
}

/* cloneG:
 */
Agraph_t *cloneG(Agraph_t * g, char* name)
{
    Agraph_t* ng;

    if (!name || *name == '\0')
	name = agnameof (g);
    ng = openG(name, g->desc);
    if (ng) {
	copyAttr((Agobj_t*)g, (Agobj_t*)ng);
	cloneGraph(ng, g);
    }
    return ng;
}

/* cloneO:
 * Create new object of type AGTYPE(obj) with all of its
 * attributes and substructure.
 * If obj is an edge, end nodes are cloned if necessary.
 * If obj is a graph, if g is null, create a clone top-level
 * graph. Otherwise, create a clone subgraph of g.
 * Assume obj != NULL.
 */
Agobj_t *cloneO(Agraph_t * g, Agobj_t * obj)
{
    Agobj_t *nobj = 0;
    Agedge_t *e;
    Agnode_t *h;
    Agnode_t *t;
    int kind = AGTYPE(obj);
    char *name;

    if (kind != AGRAPH && !g) {
	exerror("NULL graph with non-graph object in clone()");
	return 0;
    }

    switch (kind) {
    case AGNODE:		/* same as copy node */
	name = agnameof(obj);
	nobj = (Agobj_t *) openNode(g, name);
	if (nobj)
	    copyAttr(obj, nobj);
	break;
    case AGRAPH:
	name = agnameof(obj);
	if (g)
	    nobj = (Agobj_t *) openSubg(g, name);
	else
	    nobj = (Agobj_t *) openG(name, ((Agraph_t *) obj)->desc);
	if (nobj) {
	    copyAttr(obj, nobj);
	    cloneGraph((Agraph_t *) nobj, (Agraph_t *) obj);
	}
	break;
    case AGINEDGE:
    case AGOUTEDGE:
	e = (Agedge_t *) obj;
	t = (Agnode_t *) cloneO(g, OBJ(agtail(e)));
	h = (Agnode_t *) cloneO(g, OBJ(aghead(e)));
	name = agnameof (AGMKOUT(e));
	nobj = (Agobj_t *) openEdge(g, t, h, name);
	if (nobj)
	    copyAttr(obj, nobj);
	break;
    default:
	UNREACHABLE();
    }

    return nobj;
}

#define CCMARKED(n)  (((nData(n))->iu.integer)&2)
#define CCMARK(n)    (((nData(n))->iu.integer) |= 2)
#define CCUNMARK(n)  (((nData(n))->iu.integer) &= ~2)

static void cc_dfs(Agraph_t* g, Agraph_t * comp, Agnode_t * n)
{
    Agedge_t *e;
    Agnode_t *other;

    CCMARK(n);
    agidnode(comp, AGID(n), 1);
    for (e = agfstedge(g, n); e; e = agnxtedge(g, e, n)) {
	if (agtail(e) == n)
	    other = aghead(e);
	else
	    other = agtail(e);
	if (!CCMARKED(other))
	    cc_dfs(g, comp, other);
    }
}

/* compOf:
 * Return connected component of node.
 */
Agraph_t *compOf(Agraph_t * g, Agnode_t * n)
{
    Agraph_t *cg;
    Agnode_t *np;
    static int id;
    char name[64];

    if (!(n = agidnode(g, AGID(n), 0)))
	return 0;		/* n not in g */
    for (np = agfstnode(g); np; np = agnxtnode(g, np))
	CCUNMARK(np);

    snprintf(name, sizeof(name), "_cc_%d", id++);
    cg = openSubg(g, name);
    cc_dfs(g, cg, n);

    return cg;
}

/* isEdge:
 * Return edge, if any, between t and h with given key.
 * Edge is in g.
 */
Agedge_t *isEdge(Agraph_t* g, Agnode_t * t, Agnode_t * h, char *key)
{
    Agraph_t *root;

    root = sameG(t, h, "isEdge", "tail and head node");
    if (!root)
	return 0;
    if (g) {
	if (root != agroot(g)) return 0;
    }
    else
	g = root;

    return agedge(g, t, h, key, 0);
}

/* addNode:
 * Insert node n into subgraph g.
 * Return image of n
 */
Agnode_t *addNode(Agraph_t * gp, Agnode_t * np, int doAdd)
{
    if (!sameG(gp, np, "addNode", 0))
	return 0;
    return agsubnode(gp, np, doAdd);
}

/* addEdge:
 * Insert edge e into subgraph g.
 * Return image of e
 */
Agedge_t *addEdge(Agraph_t * gp, Agedge_t * ep, int doAdd)
{
    if (!sameG(gp, ep, "addEdge", 0))
	return 0;
    return agsubedge(gp, ep, doAdd);
}

/* lockGraph:
 * Set lock so that graph g will not be deleted.
 * g must be a root graph.
 * If v > 0, set lock
 * If v = 0, unset lock and delete graph is necessary.
 * If v < 0, no op
 * Always return previous lock state.
 * Return -1 on error.
 */
int lockGraph(Agraph_t * g, int v)
{
    gdata *data;
    int oldv;

    if (g != agroot(g)) {
	error(ERROR_WARNING,
	      "Graph argument to lock() is not a root graph");
	return -1;
    }
    data = gData(g);
    oldv = data->lock & 1;
    if (v > 0)
	data->lock |= 1;
    else if (v == 0 && oldv) {
	if (data->lock & 2)
	    agclose(g);
	else
	    data->lock = 0;
    }
    return oldv;
}

/* deleteObj:
 * Remove obj from g.
 * obj may belong to a subgraph of g, so we first must map
 * obj to its version in g.
 * If g is null, remove object from root graph.
 * If obj is a (sub)graph, close it. The g parameter is unused.
 * Return 0 on success, non-zero on failure.
 */
int deleteObj(Agraph_t * g, Agobj_t * obj)
{
    gdata *data;
    if (AGTYPE(obj) == AGRAPH) {
	g = (Agraph_t *) obj;
	if (g != agroot(g))
	    return agclose(g);
	data = gData(g);
	if (data->lock & 1) {
	    error(ERROR_WARNING, "Cannot delete locked graph %s",
		  agnameof(g));
	    data->lock |= 2;
	    return -1;
	} else
	    return agclose(g);
    }

    /* node or edge */
    if (!g)
	g = agroot(agraphof(obj));
    if (obj)
	return agdelete(g, obj);
    else
	return -1;
}

/* sfioWrite:
 * If the graph is passed in from a library, its output discipline
 * might not use sfio. In this case, we push an sfio discipline on
 * the graph, write it, and then pop it off.
 */
int sfioWrite(Agraph_t *g, FILE *fp) {
    int rv;

    Agiodisc_t* saveio = g->clos->disc.io;
    g->clos->disc.io = &gprIoDisc;
    rv = agwrite (g, fp);
    g->clos->disc.io = saveio;
    return rv;
}

/* writeFile:
 * Write graph into file f.
 * Return 0 on success
 */
int writeFile(Agraph_t *g, char *f) {
    int rv;

    if (!f) {
	exerror("NULL string passed to writeG");
	return 1;
    }
    FILE *fp = fopen(f, "w");
    if (!fp) {
	exwarn("Could not open %s for writing in writeG", f);
	return 1;
    }
    rv = sfioWrite(g, fp);
    fclose(fp);
    return rv;
}

/* readFile:
 * Read graph from file f.
 * Return 0 on failure
 */
Agraph_t *readFile(char *f)
{
    Agraph_t *gp;

    if (!f) {
	exerror("NULL string passed to readG");
	return 0;
    }
    FILE *fp = fopen(f, "r");
    if (!fp) {
	exwarn("Could not open %s for reading in readG", f);
	return 0;
    }
    gp = readG(fp);
    fclose(fp);

    return gp;
}

int fwriteFile(Expr_t *ex, Agraph_t *g, int fd) {
    FILE *sp;

    if (fd < 0 ||
        (elementsof(ex->file) <= INT_MAX && fd >= (int)elementsof(ex->file))
	|| !((sp = ex->file[fd]))) {
	exerror("fwriteG: %d: invalid descriptor", fd);
	return 0;
    }
    return sfioWrite(g, sp);
}

Agraph_t *freadFile(Expr_t * ex, int fd)
{
    FILE *sp;

    if (fd < 0 ||
        (elementsof(ex->file) <= INT_MAX && fd >= (int)elementsof(ex->file))
	|| !((sp = ex->file[fd]))) {
	exerror("freadG: %d: invalid descriptor", fd);
	return 0;
    }
    return readG(sp);
}

int openFile(Expr_t * ex, const char *fname, const char *mode)
{
    int idx;

    /* find open index */
    for (idx = 3; idx < elementsof(ex->file); idx++)
	if (!ex->file[idx])
	    break;
    if (idx == elementsof(ex->file)) {
	exerror("openF: no available descriptors");
	return -1;
    }
    ex->file[idx] = fopen(fname, mode);
    if (ex->file[idx])
	return idx;
    else
	return -1;
}

int closeFile(Expr_t * ex, int fd)
{
    int rv;

    if (0 <= fd && fd <= 2) {
	exerror("closeF: cannot close standard stream %d", fd);
	return -1;
    }
    if (!ex->file[fd]) {
	exerror("closeF: stream %d not open", fd);
	return -1;
    }
    rv = fclose(ex->file[fd]);
    if (!rv)
	ex->file[fd] = 0;
    return rv;
}

/*
 * Read single line from stream.
 * Return "" on EOF.
 */
char *readLine(Expr_t * ex, int fd)
{
    FILE *sp;
    int c;
    char *line;

    if (fd < 0 || fd >= elementsof(ex->file) || !((sp = ex->file[fd]))) {
	exerror("readL: %d: invalid descriptor", fd);
	return "";
    }

    agxbuf tmps = {0};
    while ((c = getc(sp)) > 0 && c != '\n')
	agxbputc(&tmps, (char)c);
    if (c == '\n')
	agxbputc(&tmps, (char)c);
    line = exstring(ex, agxbuse(&tmps));
    agxbfree(&tmps);
    return line;
}

/* compare:
 * Lexicographic ordering of objects.
 */
int compare(Agobj_t * l, Agobj_t * r)
{
    char lkind, rkind;
    if (l == NULL) {
	if (r == NULL)
	    return 0;
	else
	    return -1;
    } else if (r == NULL) {
	return 1;
    }
    if (AGID(l) < AGID(r))
	return -1;
    else if (AGID(l) > AGID(r))
	return 1;
    lkind = AGTYPE(l);
    rkind = AGTYPE(r);
    if (lkind == 3)
	lkind = 2;
    if (rkind == 3)
	rkind = 2;
    if (lkind == rkind)
	return 0;
    else if (lkind < rkind)
	return -1;
    else
	return 1;
}

/* toLower:
 * Convert characters to lowercase
 */
char *toLower(Expr_t *pgm, char *src) {

  const size_t len = strlen(src);
  char *dst = exstralloc(pgm, len + 1);
  if (UNLIKELY(dst == NULL)) {
    return NULL;
  }

  for (size_t i = 0; i < len; ++i) {
    dst[i] = (char)tolower((int)src[i]);
  }

  dst[len] = '\0';
  return dst;
}

/* toUpper:
 * Convert characters to uppercase
 */
char *toUpper(Expr_t *pgm, char *src) {

  const size_t len = strlen(src);
  char *dst = exstralloc(pgm, len + 1);
  if (UNLIKELY(dst == NULL)) {
    return NULL;
  }

  for (size_t i = 0; i < len; ++i) {
    dst[i] = (char)toupper((int)src[i]);
  }

  dst[len] = '\0';
  return dst;
}

/* toHtml:
 * Create a string marked as HTML
 */
char *toHtml(Agraph_t* g, char *arg)
{
    return agstrdup_html (g, arg);
}

/* canon:
 * Canonicalize a string for printing.
 */
char *canon(Expr_t * pgm, char *arg)
{
    char *p;

    p = agcanonStr(arg);
    if (p != arg)
	p = exstring(pgm, p);

    return p;
}

#undef S

// force the upcoming ../common/colxlate.c functions to not be exported
#ifdef COLORPROCS_API
#undef COLORPROCS_API
#endif
#ifdef GVDLL
#undef GVDLL
#endif
#ifdef GVC_EXPORTS
#undef GVC_EXPORTS
#endif
#define COLORPROCS_API static UNUSED

#include "../common/colxlate.c"

/* colorx:
 * RGB, RGBA, HSV, HSVA, CMYK
 */
char *colorx(Expr_t *ex, char *incolor, char *fmt) {
    gvcolor_t color = {{{0}},0};
    color_type_t type;
    int rc;
    int alpha;

    if (*fmt == '\0' || *incolor == '\0')
	return "";
    if (*fmt == 'R') {
	type = RGBA_BYTE;
	if (!strcmp (fmt, "RGBA")) 
	    alpha = 1;
	else
	    alpha = 0;
    }
    else if (*fmt == 'H') {
	type = HSVA_DOUBLE;
	if (!strcmp (fmt, "HSVA")) 
	    alpha = 1;
	else
	    alpha = 0;
    }
    else if (*fmt == 'C') {
	type = CMYK_BYTE;
	alpha = 0;
    }
    else
	return "";

    rc = colorxlate (incolor, &color, type);
    if (rc != COLOR_OK)
	return "";

    agxbuf fp = {0};

    switch (type) {
    case HSVA_DOUBLE :
	agxbprint(&fp, "%.03f %.03f %.03f",
	    color.u.HSVA[0], color.u.HSVA[1], color.u.HSVA[2]);
	if (alpha)
	    agxbprint(&fp, " %.03f", color.u.HSVA[3]);
	break;
    case RGBA_BYTE :
	agxbprint(&fp, "#%02x%02x%02x",
	    color.u.rgba[0], color.u.rgba[1], color.u.rgba[2]);
	if (alpha)
	    agxbprint(&fp, "%02x", color.u.rgba[3]);
	break;
    case CMYK_BYTE :
	agxbprint(&fp, "#%02x%02x%02x%02x",
	    color.u.cmyk[0], color.u.cmyk[1], color.u.cmyk[2], color.u.cmyk[3]);
	break;
    default :
	break;
    }

    char *result = exstring(ex, agxbuse(&fp));
    agxbfree(&fp);
    return result;
}

#ifndef _WIN32

#include        <unistd.h>
#include        <sys/types.h>
#include        <sys/times.h>
#include        <sys/param.h>



#ifndef HZ
#define HZ 60
#endif
typedef struct tms mytime_t;
#define GET_TIME(S) times(&(S))
#define DIFF_IN_SECS(S,T) ((S.tms_utime + S.tms_stime - T.tms_utime - T.tms_stime)/(double)HZ)

#else

#include        <time.h>

typedef clock_t mytime_t;
#define GET_TIME(S) S = clock()
#define DIFF_IN_SECS(S,T) ((S - T) / (double)CLOCKS_PER_SEC)

#endif

static mytime_t T;

void gvstart_timer(void)
{
    GET_TIME(T);
}

double gvelapsed_sec(void)
{
    mytime_t S;
    double rv;

    GET_TIME(S);
    rv = DIFF_IN_SECS(S, T);
    return rv;
}

