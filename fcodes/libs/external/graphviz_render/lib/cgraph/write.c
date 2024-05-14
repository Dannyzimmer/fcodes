/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>		/* need sprintf() */
#include <ctype.h>
#include <cgraph/cghdr.h>
#include <cgraph/likely.h>
#include <cgraph/strcasecmp.h>
#include <inttypes.h>

#define EMPTY(s)		(((s) == 0) || (s)[0] == '\0')
#define MAX(a,b)     ((a)>(b)?(a):(b))
#define CHKRV(v)     {if ((v) == EOF) return EOF;}

typedef void iochan_t;

static int ioput(Agraph_t * g, iochan_t * ofile, char *str)
{
    return AGDISC(g, io)->putstr(ofile, str);

}

#define MAX_OUTPUTLINE		128
#define MIN_OUTPUTLINE		 60
static int write_body(Agraph_t * g, iochan_t * ofile);
static int Level;
static int Max_outputline = MAX_OUTPUTLINE;
static Agsym_t *Tailport, *Headport;

static int indent(Agraph_t * g, iochan_t * ofile)
{
    int i;
    for (i = Level; i > 0; i--)
	CHKRV(ioput(g, ofile, "\t"));
    return 0;
}

// alphanumeric, '.', '-', or non-ascii; basically, chars used in unquoted ids
static bool is_id_char(char c) {
  return isalnum(c) || c == '.' || c == '-' || !isascii(c);
}

// is the prefix of this string a recognized Graphviz escape sequence?
// https://graphviz.org/docs/attr-types/escString/
static bool is_escape(const char *str) {
  assert(str != NULL);

  if (*str != '\\')
    return false;

  if (str[1] == 'E')
    return true;
  if (str[1] == 'G')
    return true;
  if (str[1] == 'H')
    return true;
  if (str[1] == 'L')
    return true;
  if (str[1] == 'N')
    return true;
  if (str[1] == 'T')
    return true;

  if (str[1] == 'l')
    return true;
  if (str[1] == 'n')
    return true;
  if (str[1] == 'r')
    return true;

  if (str[1] == '\\')
    return true;

  if (str[1] == '"')
    return true;

  return false;
}

/* _agstrcanon:
 * Canonicalize ordinary strings. 
 * Assumes buf is large enough to hold output.
 */
static char *_agstrcanon(char *arg, char *buf)
{
    char *s, *p;
    char uc;
    int cnt = 0, dotcnt = 0;
    bool needs_quotes = false;
    bool part_of_escape = false;
    bool maybe_num;
    bool backslash_pending = false;
    static const char *tokenlist[]	/* must agree with scan.l */
	= { "node", "edge", "strict", "graph", "digraph", "subgraph",
	NULL
    };
    const char **tok;

    if (EMPTY(arg))
	return "\"\"";
    s = arg;
    p = buf;
    *p++ = '\"';
    uc = *s++;
    maybe_num = isdigit(uc) != 0 || uc == '.' || uc == '-';
    while (uc) {
	if (uc == '\"' && !part_of_escape) {
	    *p++ = '\\';
	    needs_quotes = true;
	} else if (!part_of_escape && is_escape(&s[-1])) {
	    needs_quotes = true;
	    part_of_escape = true;
	} else if (maybe_num) {
	    if (uc == '-') {
		if (cnt) {
		    maybe_num = false;
		    needs_quotes = true;
		}
	    }
	    else if (uc == '.') {
		if (dotcnt++) {
		    maybe_num = false;
		    needs_quotes = true;
		}
	    }
	    else if (!isdigit(uc)) {
		maybe_num = false;
		needs_quotes = true;
	    }
	    part_of_escape = false;
	}
	else if (!(isalnum(uc) || uc == '_' || !isascii(uc))) {
	    needs_quotes = true;
	    part_of_escape = false;
	}
	*p++ = uc;
	uc = *s++;
	cnt++;
	
	/* If breaking long strings into multiple lines, only allow breaks after a non-id char, not a backslash, where the next char is an
 	 * id char.
	 */
	if (Max_outputline) {
            if (uc && backslash_pending && !(is_id_char(p[-1]) || p[-1] == '\\') && is_id_char(uc)) {
        	*p++ = '\\';
        	*p++ = '\n';
        	needs_quotes = true;
        	backslash_pending = false;
		cnt = 0;
            } else if (uc && (cnt >= Max_outputline)) {
        	if (!(is_id_char(p[-1]) || p[-1] == '\\') && is_id_char(uc)) {
	            *p++ = '\\';
    	            *p++ = '\n';
	            needs_quotes = true;
		    cnt = 0;
        	} else {
                    backslash_pending = true;
        	}
	    }
	}
    }
    *p++ = '\"';
    *p = '\0';
    if (needs_quotes || (cnt == 1 && (*arg == '.' || *arg == '-')))
	return buf;

    /* Use quotes to protect tokens (example, a node named "node") */
    /* It would be great if it were easier to use flex here. */
    for (tok = tokenlist; *tok; tok++)
	if (!strcasecmp(*tok, arg))
	    return buf;
    return arg;
}

/* agcanonhtmlstr:
 * Canonicalize html strings. 
 */
static char *agcanonhtmlstr(const char *arg, char *buf)
{
    sprintf(buf, "<%s>", arg);
    return buf;
}

/*
 * canonicalize a string for printing.
 * must agree with strings in scan.l
 * Unsafe if buffer is not large enough.
 */
char *agstrcanon(char *arg, char *buf)
{
    if (aghtmlstr(arg))
	return agcanonhtmlstr(arg, buf);
    else
	return _agstrcanon(arg, buf);
}

static char *getoutputbuffer(const char *str)
{
    static char *rv;
    static size_t len = 0;
    size_t req;

    req = MAX(2 * strlen(str) + 2, BUFSIZ);
    if (req > len) {
	char *r = realloc(rv, req);
	if (UNLIKELY(r == NULL))
	    return NULL;
	rv = r;
	len = req;
    }
    return rv;
}

/*
 * canonicalize a string for printing.
 * must agree with strings in scan.l
 * Shared static buffer - unsafe.
 */
char *agcanonStr(char *str)
{
    char *buffer = getoutputbuffer(str);
    if (UNLIKELY(buffer == NULL))
        return NULL;
    return agstrcanon(str, buffer);
}

/*
 * canonicalize a string for printing.
 * If html is true, use HTML canonicalization.
 * Shared static buffer - unsafe.
 */
char *agcanon(char *str, int html)
{
    char* buf = getoutputbuffer(str);
    if (UNLIKELY(buf == NULL))
	return NULL;
    if (html)
	return agcanonhtmlstr(str, buf);
    else
	return _agstrcanon(str, buf);
}

static int _write_canonstr(Agraph_t * g, iochan_t * ofile, char *str,
			   int chk)
{
    if (chk) {
	str = agcanonStr(str);
    } else {
	char *buffer = getoutputbuffer(str);
	if (UNLIKELY(buffer == NULL))
	    return EOF;
	str = _agstrcanon(str, buffer);
    }
    return ioput(g, ofile, str);
}

static int write_canonstr(Agraph_t * g, iochan_t * ofile, char *str)
{
    char *s;
    int r;

    /* str may not have been allocated by agstrdup, so we first need to turn it
     * into a valid refstr
     */
    s = agstrdup(g, str);

    r = _write_canonstr(g, ofile, s, TRUE);

    agstrfree(g, s);
    return r;
}

static int write_dict(Agraph_t * g, iochan_t * ofile, char *name,
		      Dict_t * dict, int top)
{
    int cnt = 0;
    Dict_t *view;
    Agsym_t *sym, *psym;

    if (!top)
	view = dtview(dict, NULL);
    else
	view = 0;
    for (sym = dtfirst(dict); sym; sym = dtnext(dict, sym)) {
	if (EMPTY(sym->defval) && !sym->print) {	/* try to skip empty str (default) */
	    if (view == NULL)
		continue;	/* no parent */
	    psym = dtsearch(view, sym);
	    assert(psym);
	    if (EMPTY(psym->defval) && psym->print)
		continue;	/* also empty in parent */
	}
	if (cnt++ == 0) {
	    CHKRV(indent(g, ofile));
	    CHKRV(ioput(g, ofile, name));
	    CHKRV(ioput(g, ofile, " ["));
	    Level++;
	} else {
	    CHKRV(ioput(g, ofile, ",\n"));
	    CHKRV(indent(g, ofile));
	}
	CHKRV(write_canonstr(g, ofile, sym->name));
	CHKRV(ioput(g, ofile, "="));
	CHKRV(write_canonstr(g, ofile, sym->defval));
    }
    if (cnt > 0) {
	Level--;
	if (cnt > 1) {
	    CHKRV(ioput(g, ofile, "\n"));
	    CHKRV(indent(g, ofile));
	}
	CHKRV(ioput(g, ofile, "];\n"));
    }
    if (!top)
	dtview(dict, view);	/* restore previous view */
    return 0;
}

static int write_dicts(Agraph_t * g, iochan_t * ofile, int top)
{
    Agdatadict_t *def;
    if ((def = agdatadict(g, false))) {
	CHKRV(write_dict(g, ofile, "graph", def->dict.g, top));
	CHKRV(write_dict(g, ofile, "node", def->dict.n, top));
	CHKRV(write_dict(g, ofile, "edge", def->dict.e, top));
    }
    return 0;
}

static int write_hdr(Agraph_t * g, iochan_t * ofile, int top)
{
    char *name, *sep, *kind, *strict;
    bool root = false;
    bool hasName = true;

    strict = "";
    if (!top && agparent(g))
	kind = "sub";
    else {
	root = true;
	if (g->desc.directed)
	    kind = "di";
	else
	    kind = "";
	if (agisstrict(g))
	    strict = "strict ";
	Tailport = agattr(g, AGEDGE, TAILPORT_ID, NULL);
	Headport = agattr(g, AGEDGE, HEADPORT_ID, NULL);
    }
    name = agnameof(g);
    sep = " ";
    if (!name || name[0] == LOCALNAMEPREFIX) {
	sep = name = "";
	hasName = false;
    }
    CHKRV(indent(g, ofile));
    CHKRV(ioput(g, ofile, strict));

    /* output "<kind>graph" only for root graphs or graphs with names */
    if (root || hasName) {
	CHKRV(ioput(g, ofile, kind));
	CHKRV(ioput(g, ofile, "graph "));
    }
    if (hasName)
	CHKRV(write_canonstr(g, ofile, name));
    CHKRV(ioput(g, ofile, sep));
    CHKRV(ioput(g, ofile, "{\n"));
    Level++;
    CHKRV(write_dicts(g, ofile, top));
    AGATTRWF(g) = TRUE;
    return 0;
}

static int write_trl(Agraph_t * g, iochan_t * ofile)
{
    (void)g;
    Level--;
    CHKRV(indent(g, ofile));
    CHKRV(ioput(g, ofile, "}\n"));
    return 0;
}

static bool irrelevant_subgraph(Agraph_t * g)
{
    int i, n;
    Agattr_t *sdata, *pdata, *rdata;
    Agdatadict_t *dd;

    char *name;

    name = agnameof(g);
    if (name && name[0] != LOCALNAMEPREFIX)
	return false;
    if ((sdata = agattrrec(g)) && (pdata = agattrrec(agparent(g)))) {
	rdata = agattrrec(agroot(g));
	n = dtsize(rdata->dict);
	for (i = 0; i < n; i++)
	    if (sdata->str[i] && pdata->str[i]
		&& strcmp(sdata->str[i], pdata->str[i]))
		return false;
    }
    dd = agdatadict(g, false);
    if (!dd)
	return true;
    if (dtsize(dd->dict.n) > 0 || dtsize(dd->dict.e) > 0)
	return false;
    return true;
}

static bool node_in_subg(Agraph_t * g, Agnode_t * n)
{
    Agraph_t *subg;

    for (subg = agfstsubg(g); subg; subg = agnxtsubg(subg)) {
	if (irrelevant_subgraph(subg))
	    continue;
	if (agsubnode(subg, n, 0))
	    return true;
    }
    return false;
}

static bool has_no_edges(Agraph_t * g, Agnode_t * n)
{
    return agfstin(g, n) == NULL && agfstout(g, n) == NULL;
}

static bool has_no_predecessor_below(Agraph_t * g, Agnode_t * n,
				    uint64_t val)
{
    Agedge_t *e;

    if (AGSEQ(n) < val)
	return false;
    for (e = agfstin(g, n); e; e = agnxtin(g, e))
	if (AGSEQ(e->node) < val)
	    return false;
    return true;
}

static bool not_default_attrs(Agraph_t * g, Agnode_t * n)
{
    Agattr_t *data;
    Agsym_t *sym;

    (void)g;
    if ((data = agattrrec(n))) {
	for (sym = dtfirst(data->dict); sym; sym = dtnext(data->dict, sym)) {
	    if (data->str[sym->id] != sym->defval)
		return true;
	}
    }
    return false;
}

static int write_subgs(Agraph_t * g, iochan_t * ofile)
{
    Agraph_t *subg;

    for (subg = agfstsubg(g); subg; subg = agnxtsubg(subg)) {
	if (irrelevant_subgraph(subg)) {
	    write_subgs(subg, ofile);
	}
	else {
	    CHKRV(write_hdr(subg, ofile, FALSE));
	    CHKRV(write_body(subg, ofile));
	    CHKRV(write_trl(subg, ofile));
	}
    }
    return 0;
}

static int write_edge_name(Agedge_t * e, iochan_t * ofile, int terminate)
{
    int rv;
    char *p;
    Agraph_t *g;

    p = agnameof(e);
    g = agraphof(e);
    if (!EMPTY(p)) {
	if (!terminate) {
	    Level++;
	}
	CHKRV(ioput(g, ofile, "\t[key="));
	CHKRV(write_canonstr(g, ofile, p));
	if (terminate)
	    CHKRV(ioput(g, ofile, "]"));
	rv = TRUE;
    } else
	rv = FALSE;
    return rv;
}


static int write_nondefault_attrs(void *obj, iochan_t * ofile,
				  Dict_t * defdict)
{
    Agattr_t *data;
    Agsym_t *sym;
    Agraph_t *g;
    int cnt = 0;
    int rv;

    if (AGTYPE(obj) == AGINEDGE || AGTYPE(obj) == AGOUTEDGE) {
	CHKRV(rv = write_edge_name(obj, ofile, FALSE));
	if (rv)
	    cnt++;
    }
    data = agattrrec(obj);
    g = agraphof(obj);
    if (data)
	for (sym = dtfirst(defdict); sym; sym = dtnext(defdict, sym)) {
	    if (AGTYPE(obj) == AGINEDGE || AGTYPE(obj) == AGOUTEDGE) {
		if (Tailport && sym->id == Tailport->id)
		    continue;
		if (Headport && sym->id == Headport->id)
		    continue;
	    }
	    if (data->str[sym->id] != sym->defval) {
		if (cnt++ == 0) {
		    CHKRV(ioput(g, ofile, "\t["));
		    Level++;
		} else {
		    CHKRV(ioput(g, ofile, ",\n"));
		    CHKRV(indent(g, ofile));
		}
		CHKRV(write_canonstr(g, ofile, sym->name));
		CHKRV(ioput(g, ofile, "="));
		CHKRV(write_canonstr(g, ofile, data->str[sym->id]));
	    }
	}
    if (cnt > 0) {
	CHKRV(ioput(g, ofile, "]"));
	Level--;
    }
    AGATTRWF(obj) = TRUE;
    return 0;
}

static int write_nodename(Agnode_t * n, iochan_t * ofile)
{
    char *name;
    Agraph_t *g;

    name = agnameof(n);
    g = agraphof(n);
    if (name) {
	CHKRV(write_canonstr(g, ofile, name));
    } else {
	char buf[sizeof("__SUSPECT") + 20];
	snprintf(buf, sizeof(buf), "_%" PRIu64 "_SUSPECT", AGID(n));	/* could be deadly wrong */
	CHKRV(ioput(g, ofile, buf));
    }
    return 0;
}

static int attrs_written(void *obj)
{
    return AGATTRWF(obj);
}

static int write_node(Agnode_t * n, iochan_t * ofile, Dict_t * d)
{
    Agraph_t *g;

    g = agraphof(n);
    CHKRV(indent(g, ofile));
    CHKRV(write_nodename(n, ofile));
    if (!attrs_written(n))
	CHKRV(write_nondefault_attrs(n, ofile, d));
    return ioput(g, ofile, ";\n");
}

/* node must be written if it wasn't already emitted because of
 * a subgraph or one of its predecessors, and if it is a singleton
 * or has non-default attributes.
 */
static bool write_node_test(Agraph_t * g, Agnode_t * n,
			   uint64_t pred_id)
{
    if (!node_in_subg(g, n) && has_no_predecessor_below(g, n, pred_id)) {
	if (has_no_edges(g, n) || not_default_attrs(g, n))
	    return true;
    }
    return false;
}

static int write_port(Agedge_t * e, iochan_t * ofile, Agsym_t * port)
{
    char *val;
    Agraph_t *g;

    if (!port)
	return 0;
    g = agraphof(e);
    val = agxget(e, port);
    if (val[0] == '\0')
	return 0;

    CHKRV(ioput(g, ofile, ":"));
    if (aghtmlstr(val)) {
	CHKRV(write_canonstr(g, ofile, val));
    } else {
	char *s = strchr(val, ':');
	if (s) {
	    *s = '\0';
	    CHKRV(_write_canonstr(g, ofile, val, FALSE));
	    CHKRV(ioput(g, ofile, ":"));
	    CHKRV(_write_canonstr(g, ofile, s + 1, FALSE));
	    *s = ':';
	} else {
	    CHKRV(_write_canonstr(g, ofile, val, FALSE));
	}
    }
    return 0;
}

static int write_edge_test(Agraph_t * g, Agedge_t * e)
{
    Agraph_t *subg;

    /* can use agedge() because we subverted the dict compar_f */
    for (subg = agfstsubg(g); subg; subg = agnxtsubg(subg)) {
	if (irrelevant_subgraph(subg))
	    continue;
	if (agsubedge(subg, e, 0))
	    return FALSE;
    }
    return TRUE;
}

static int write_edge(Agedge_t * e, iochan_t * ofile, Dict_t * d)
{
    Agnode_t *t, *h;
    Agraph_t *g;

    t = AGTAIL(e);
    h = AGHEAD(e);
    g = agraphof(t);
    CHKRV(indent(g, ofile));
    CHKRV(write_nodename(t, ofile));
    CHKRV(write_port(e, ofile, Tailport));
    CHKRV(ioput(g, ofile, (agisdirected(agraphof(t)) ? " -> " : " -- ")));
    CHKRV(write_nodename(h, ofile));
    CHKRV(write_port(e, ofile, Headport));
    if (!attrs_written(e)) {
	CHKRV(write_nondefault_attrs(e, ofile, d));
    } else {
	CHKRV(write_edge_name(e, ofile, TRUE));
    }
    return ioput(g, ofile, ";\n");
}

static int write_body(Agraph_t * g, iochan_t * ofile)
{
    Agnode_t *n, *prev;
    Agedge_t *e;
    Agdatadict_t *dd;

    CHKRV(write_subgs(g, ofile));
    dd = agdatadict(g, false);
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (write_node_test(g, n, AGSEQ(n)))
	    CHKRV(write_node(n, ofile, dd ? dd->dict.n : 0));
	prev = n;
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    if (prev != aghead(e) && write_node_test(g, aghead(e), AGSEQ(n))) {
		CHKRV(write_node(aghead(e), ofile, dd ? dd->dict.n : 0));
		prev = aghead(e);
	    }
	    if (write_edge_test(g, e))
		CHKRV(write_edge(e, ofile, dd ? dd->dict.e : 0));
	}

	}
    return 0;
}

static void set_attrwf(Agraph_t * g, bool toplevel, bool value)
{
    Agraph_t *subg;
    Agnode_t *n;
    Agedge_t *e;

    AGATTRWF(g) = value;
    for (subg = agfstsubg(g); subg; subg = agnxtsubg(subg)) {
	set_attrwf(subg, false, value);
    }
    if (toplevel) {
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    AGATTRWF(n) = value;
	    for (e = agfstout(g, n); e; e = agnxtout(g, e))
		AGATTRWF(e) = value;
	}
    }
}

/* agwrite:
 * Return 0 on success, EOF on failure
 */
int agwrite(Agraph_t * g, void *ofile)
{
    char* s;
    Level = 0;			/* re-initialize tab level */
    s = agget(g, "linelength");
    if (s != NULL && isdigit((int)*s)) {
	unsigned long len = strtoul(s, NULL, 10);
	if ((len == 0 || len >= MIN_OUTPUTLINE) && len <= (unsigned long)INT_MAX)
	    Max_outputline = (int)len;
    }
    set_attrwf(g, true, false);
    CHKRV(write_hdr(g, ofile, TRUE));
    CHKRV(write_body(g, ofile));
    CHKRV(write_trl(g, ofile));
    Max_outputline = MAX_OUTPUTLINE;
    return AGDISC(g, io)->flush(ofile);
}
