/**
 * @file
 * @brief DOT-<a href=https://en.wikipedia.org/wiki/GXL>GXL</a> convert subroutines
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

#include <cgraph/agxbuf.h>
#include <cgraph/alloc.h>
#include <cgraph/startswith.h>
#include <common/types.h>
#include <common/utils.h>
#include "convert.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define EMPTY(s)	((s == 0) || (*s == '\0'))
#define SLEN(s)     (sizeof(s)-1)

#define GXL_ATTR    "_gxl_"
#define GXL_ROLE	"_gxl_role"
#define GXL_HYPER	"_gxl_hypergraph"
#define GXL_ID      "_gxl_id"
#define GXL_FROM    "_gxl_fromorder"
#define GXL_TO      "_gxl_toorder"
#define GXL_TYPE    "_gxl_type"
#define GXL_COMP    "_gxl_composite_"
#define GXL_LOC     "_gxl_locator_"

#define GXL_COMP_LEN (SLEN(GXL_COMP))

typedef struct {
    Agrec_t h;
    int written;
} Local_Agnodeinfo_t;

static int Level;		/* level of tabs */
static Agsym_t *Tailport, *Headport;

typedef struct {
    Dtlink_t link;
    char *name;
    char *unique_name;
} namev_t;

static namev_t *make_nitem(namev_t *objp, Dtdisc_t *disc) {
    (void)disc;

    namev_t *np = gv_alloc(sizeof(*np));
    np->name = objp->name;
    np->unique_name = 0;
    return np;
}

static void free_nitem(namev_t *np, Dtdisc_t *disc) {
    (void)disc;

    free(np);
}

static Dtdisc_t nameDisc = {
    offsetof(namev_t, name),
    -1,
    offsetof(namev_t, link),
    (Dtmake_f) make_nitem,
    (Dtfree_f) free_nitem,
    NULL,
    NULL,
};

typedef struct {
    Dtlink_t link;
    char *name;
} idv_t;

static void free_iditem(idv_t *idp, Dtdisc_t *disc) {
    (void)disc;

    free(idp->name);
    free(idp);
}

static Dtdisc_t idDisc = {
    offsetof(idv_t, name),
    -1,
    offsetof(idv_t, link),
    NULL,
    (Dtfree_f) free_iditem,
    NULL,
    NULL,
};

typedef struct {
    Dt_t *nodeMap;
    Dt_t *graphMap;
    Dt_t *synNodeMap;
    Dt_t *idList;
    Agraph_t *root;
    char attrsNotWritten;
    char directed;
} gxlstate_t;

static void writeBody(gxlstate_t *, Agraph_t * g, FILE * gxlFile);
static void iterateBody(gxlstate_t * stp, Agraph_t * g);

static void tabover(FILE * gxlFile)
{
    int temp = Level;
    while (temp--)
	putc('\t', gxlFile);
}

/* legalGXLName:
 * By XML spec, 
 *   ID := (alpha|'_'|':')(NameChar)*
 *   NameChar := alpha|digit|'.'|':'|'-'|'_'
 */
static bool legalGXLName(const char *id) {
    char c = *id++;
    if (!isalpha(c) && c != '_' && c != ':')
	return false;
    while ((c = *id++)) {
	if (!isalnum(c) && c != '_' && c != ':' && c != '-' && c != '.')
	    return false;
    }
    return true;
}

// `fputs` wrapper to handle the difference in calling convention to what
// `xml_escape`’s `cb` expects
static inline int put(void *stream, const char *s) {
  return fputs(s, stream);
}

// write a string to the given file, XML-escaping the input
static inline int xml_puts(FILE *stream, const char *s) {
  const xml_flags_t flags = {.dash = 1, .nbsp = 1};
  return xml_escape(s, flags, put, stream);
}

// wrapper around `xml_escape` to set flags for URL escaping
static int xml_url_puts(FILE *f, const char *s) {
  const xml_flags_t flags = {0};
  return xml_escape(s, flags, put, f);
}

static bool isGxlGrammar(const char *name) {
  return startswith(name, GXL_ATTR);
}

static bool isLocatorType(const char *name) {
  return startswith(name, GXL_LOC);
}

static bool idexists(Dt_t * ids, char *id) {
  return dtmatch(ids, id) != NULL;
}

/* addid:
 * assume id is not in ids.
 */
static char *addid(Dt_t *ids, const char *id) {
    idv_t *idp = gv_alloc(sizeof(*idp));

    idp->name = gv_strdup(id);
    dtinsert(ids, idp);
    return idp->name;
}

static char *createGraphId(Dt_t * ids)
{
    static int graphIdCounter = 0;
    agxbuf buf = {0};
    char *name;

    do {
	agxbprint(&buf, "G_%d", graphIdCounter++);
	name = agxbuse(&buf);
    } while (idexists(ids, name));
    char *rv = addid(ids, name);
    agxbfree(&buf);
    return rv;
}

static char *createNodeId(Dt_t * ids)
{
    static int nodeIdCounter = 0;
    agxbuf buf = {0};
    char *name;

    do {
	agxbprint(&buf, "N_%d", nodeIdCounter++);
	name = agxbuse(&buf);
    } while (idexists(ids, name));
    char *rv = addid(ids, name);
    agxbfree(&buf);
    return rv;
}

static char *mapLookup(Dt_t * nm, char *name)
{
    namev_t *objp = dtmatch(nm, name);
    if (objp)
	return objp->unique_name;
    return NULL;
}

/* nodeID:
 * Return id associated with the given node.
 */
static char *nodeID(gxlstate_t * stp, Agnode_t * n)
{
    char *name = agnameof(n);
    char *uniqueName = mapLookup(stp->nodeMap, name);
    assert(uniqueName);
    return uniqueName;
}

#define EDGEOP "--"		/* cannot use '>'; illegal in ID in GXL */

static char *createEdgeId(gxlstate_t * stp, Agedge_t * e)
{
    int edgeIdCounter = 1;
    char *hname = nodeID(stp, AGHEAD(e));
    char *tname = nodeID(stp, AGTAIL(e));

    agxbuf bp = {0};

    agxbprint(&bp, "%s%s%s", tname, EDGEOP, hname);
    char *id_name = agxbuse(&bp);
    while (idexists(stp->idList, id_name)) {
	agxbprint(&bp, "%s%s%s:%d", tname, EDGEOP, hname, edgeIdCounter++);
	id_name = agxbuse(&bp);
    }

    char *rv = addid(stp->idList, id_name);
    agxbfree(&bp);
    return rv;
}

static void addToMap(Dt_t * map, char *name, char *uniqueName)
{
    namev_t obj = {.name = name};
    namev_t *objp = dtinsert(map, &obj);
    assert(objp->unique_name == NULL);
    objp->unique_name = uniqueName;
}

static void graphAttrs(FILE * gxlFile, Agraph_t * g)
{
    char *val = agget(g, GXL_ROLE);
    if (!EMPTY(val)) {
	fprintf(gxlFile, " role=\"");
	xml_puts(gxlFile, val);
	fprintf(gxlFile, "\"");
    }
    val = agget(g, GXL_HYPER);
    if (!EMPTY(val)) {
	fprintf(gxlFile, " hypergraph=\"");
	xml_puts(gxlFile, val);
	fprintf(gxlFile, "\"");
    }
}

static void edgeAttrs(FILE * gxlFile, Agedge_t * e)
{
    char *val = agget(e, GXL_ID);
    if (!EMPTY(val)) {
	fprintf(gxlFile, " id=\"");
	xml_puts(gxlFile, val);
	fprintf(gxlFile, "\"");
    }
    val = agget(e, GXL_FROM);
    if (!EMPTY(val)) {
	fprintf(gxlFile, " fromorder=\"");
	xml_puts(gxlFile, val);
	fprintf(gxlFile, "\"");
    }
    val = agget(e, GXL_TO);
    if (!EMPTY(val)) {
	fprintf(gxlFile, " toorder=\"");
	xml_puts(gxlFile, val);
	fprintf(gxlFile, "\"");
    }
}


static void printHref(FILE * gxlFile, void *n)
{
    char *val = agget(n, GXL_TYPE);
    if (!EMPTY(val)) {
	tabover(gxlFile);
	fprintf(gxlFile, "\t<type xlink:href=\"");
	xml_url_puts(gxlFile, val);
	fprintf(gxlFile, "\">\n");
	tabover(gxlFile);
	fprintf(gxlFile, "\t</type>\n");
    }
}


static void
writeDict(FILE *gxlFile, const char *name, Dict_t *dict, bool isGraph) {
    Dict_t *view = dtview(dict, NULL);
    for (Agsym_t *sym = dtfirst(dict); sym; sym = dtnext(dict, sym)) {
	if (!isGxlGrammar(sym->name)) {
	    if (EMPTY(sym->defval)) {	/* try to skip empty str (default) */
		if (view == NULL)
		    continue;	/* no parent */
		Agsym_t *psym = dtsearch(view, sym);
		/* assert(psym); */
		if (EMPTY(psym->defval))
		    continue;	/* also empty in parent */
	    }

	    if (isLocatorType(sym->defval)) {
		char *locatorVal = sym->defval + strlen(GXL_LOC);

		tabover(gxlFile);
		fprintf(gxlFile, "\t<attr name=\"");
		xml_puts(gxlFile, sym->name);
		fprintf(gxlFile, "\">\n");
		tabover(gxlFile);
		fprintf(gxlFile, "\t\t<locator xlink:href=\"");
		xml_url_puts(gxlFile, locatorVal);
		fprintf(gxlFile, "\"/>\n");
		tabover(gxlFile);
		fprintf(gxlFile, "\t</attr>\n");
	    } else {
		tabover(gxlFile);
		if (isGraph) {
		    fprintf(gxlFile, "\t<attr name=\"");
		    xml_puts(gxlFile, sym->name);
		    fprintf(gxlFile, "\" ");
		    fprintf(gxlFile, "kind=\"");
		    xml_puts(gxlFile, name);
		    fprintf(gxlFile, "\">\n");
		}
		else {
		    fprintf(gxlFile, "\t<attr name=\"");
		    xml_puts(gxlFile, name);
		    fprintf(gxlFile, ":");
		    xml_puts(gxlFile, sym->name);
		    fprintf(gxlFile, "\" kind=\"");
		    xml_puts(gxlFile, name);
		    fprintf(gxlFile, "\">\n");
		}
		tabover(gxlFile);
		fprintf(gxlFile, "\t\t<string>");
		xml_puts(gxlFile, sym->defval);
		fprintf(gxlFile, "</string>\n");
		tabover(gxlFile);
		fprintf(gxlFile, "\t</attr>\n");
	    }
	} else {
	    /* gxl attr; check for special cases like composites */
	    if (startswith(sym->name, GXL_COMP)) {
		if (EMPTY(sym->defval)) {
		    if (view == NULL)
			continue;
		    Agsym_t *psym = dtsearch(view, sym);
		    if (EMPTY(psym->defval))
			continue;
		}

		tabover(gxlFile);
		fprintf(gxlFile, "\t<attr name=\"");
		xml_puts(gxlFile, sym->name + GXL_COMP_LEN);
		fprintf(gxlFile, "\" ");
		fprintf(gxlFile, "kind=\"");
		xml_puts(gxlFile, name);
		fprintf(gxlFile, "\">\n");
		tabover(gxlFile);
		fprintf(gxlFile, "\t\t");
		xml_puts(gxlFile, sym->defval);
		fprintf(gxlFile, "\n");
		tabover(gxlFile);
		fprintf(gxlFile, "\t</attr>\n");
	    }
	}
    }
    dtview(dict, view);		/* restore previous view */
}

static void writeDicts(Agraph_t * g, FILE * gxlFile)
{
    Agdatadict_t *def;
    if ((def = agdatadict(g, false))) {
	writeDict(gxlFile, "graph", def->dict.g, true);
	writeDict(gxlFile, "node", def->dict.n, false);
	writeDict(gxlFile, "edge", def->dict.e, false);
    }
}

static void writeHdr(gxlstate_t *stp, Agraph_t *g, FILE *gxlFile, bool top) {
    char *kind;

    Level++;
    stp->attrsNotWritten = AGATTRWF(g);

    char *name = agnameof(g);
    if (g->desc.directed)
	kind = "directed";
    else
	kind = "undirected";
    if (!top && agparent(g)) {
	/* this must be anonymous graph */

	agxbuf buf = {0};
	agxbprint(&buf, "N_%s", name);
	char *bp = agxbuse(&buf);
	if (idexists(stp->idList, bp) || !legalGXLName(bp)) {
	    bp = createNodeId(stp->idList);
	} else {
	    bp = addid(stp->idList, bp);
	}
	addToMap(stp->synNodeMap, name, bp);

	tabover(gxlFile);
	fprintf(gxlFile, "<node id=\"%s\">\n", bp);
	agxbfree(&buf);
	Level++;
    } else {
	Tailport = agattr(g, AGEDGE, "tailport", NULL);
	Headport = agattr(g, AGEDGE, "headport", NULL);
    }

    char *uniqueName = mapLookup(stp->graphMap, name);
    tabover(gxlFile);
    fprintf(gxlFile, "<graph id=\"%s\" edgeids=\"true\" edgemode=\"%s\"",
	    uniqueName, kind);
    graphAttrs(gxlFile, g);
    fprintf(gxlFile, ">\n");

    if (uniqueName && (strcmp(name, uniqueName) != 0)) {
	tabover(gxlFile);
	fprintf(gxlFile, "\t<attr name=\"name\">\n");
	tabover(gxlFile);
	fprintf(gxlFile, "\t\t<string>");
	xml_puts(gxlFile, name);
	fprintf(gxlFile, "</string>\n");
	tabover(gxlFile);
	fprintf(gxlFile, "\t</attr>\n");
    }

    if (agisstrict(g)) {
	tabover(gxlFile);
	fprintf(gxlFile, "\t<attr name=\"strict\">\n");
	tabover(gxlFile);
	fprintf(gxlFile, "\t\t<string>true</string>\n");
	tabover(gxlFile);
	fprintf(gxlFile, "\t</attr>\n");
    }

    writeDicts(g, gxlFile);
    printHref(gxlFile, g);
    AGATTRWF(g) = !(AGATTRWF(g));
}

static void writeTrl(Agraph_t *g, FILE *gxlFile, bool top) {
    tabover(gxlFile);
    fprintf(gxlFile, "</graph>\n");
    Level--;
    if (!top && agparent(g)) {
	tabover(gxlFile);
	fprintf(gxlFile, "</node>\n");
	Level--;
    }
}


static void writeSubgs(gxlstate_t * stp, Agraph_t * g, FILE * gxlFile)
{
    for (Agraph_t *subg = agfstsubg(g); subg; subg = agnxtsubg(subg)) {
	writeHdr(stp, subg, gxlFile, false);
	writeBody(stp, subg, gxlFile);
	writeTrl(subg, gxlFile, false);
    }
}

static bool writeEdgeName(Agedge_t *e, FILE *gxlFile) {
    char *p = agnameof(e);
    if (!(EMPTY(p))) {
	tabover(gxlFile);
	fprintf(gxlFile, "\t<attr name=\"key\">\n");
	tabover(gxlFile);
	fprintf(gxlFile, "\t\t<string>");
	xml_puts(gxlFile, p);
	fprintf(gxlFile, "</string>\n");
	tabover(gxlFile);
	fprintf(gxlFile, "\t</attr>\n");
	return true;
    }
    return false;
}


static void
writeNondefaultAttr(void *obj, FILE * gxlFile, Dict_t * defdict)
{
    int cnt = 0;

    if (AGTYPE(obj) == AGINEDGE || AGTYPE(obj) == AGOUTEDGE) {
	if (writeEdgeName(obj, gxlFile))
	    cnt++;
    }
    Agattr_t *data = agattrrec(obj);
    if (data) {
	for (Agsym_t *sym = dtfirst(defdict); sym; sym = dtnext(defdict, sym)) {
	    if (!isGxlGrammar(sym->name)) {
		if (AGTYPE(obj) == AGINEDGE || AGTYPE(obj) == AGOUTEDGE) {
		    if (Tailport && sym->id == Tailport->id)
			continue;
		    if (Headport && sym->id == Headport->id)
			continue;
		}
		if (data->str[sym->id] != sym->defval) {

		    if (strcmp(data->str[sym->id], "") == 0)
			continue;

		    if (isLocatorType(data->str[sym->id])) {
			char *locatorVal = data->str[sym->id] + strlen(GXL_LOC);

			tabover(gxlFile);
			fprintf(gxlFile, "\t<attr name=\"");
			xml_puts(gxlFile, sym->name);
			fprintf(gxlFile, "\">\n");
			tabover(gxlFile);
			fprintf(gxlFile, "\t\t<locator xlink:href=\"");
			xml_url_puts(gxlFile, locatorVal);
			fprintf(gxlFile, "\"/>\n");
			tabover(gxlFile);
			fprintf(gxlFile, "\t</attr>\n");
		    } else {
			tabover(gxlFile);
			fprintf(gxlFile, "\t<attr name=\"");
			xml_puts(gxlFile, sym->name);
			fprintf(gxlFile, "\"");
			if (aghtmlstr(data->str[sym->id])) {
			  // This is a <…> string. Note this in the kind.
			  fprintf(gxlFile, " kind=\"HTML-like string\"");
			}
			fprintf(gxlFile, ">\n");
			tabover(gxlFile);
			fprintf(gxlFile, "\t\t<string>");
			xml_puts(gxlFile, data->str[sym->id]);
			fprintf(gxlFile, "</string>\n");
			tabover(gxlFile);
			fprintf(gxlFile, "\t</attr>\n");
		    }
		}
	    } else {
		/* gxl attr; check for special cases like composites */
		if (startswith(sym->name, GXL_COMP)) {
		    if (data->str[sym->id] != sym->defval) {

			tabover(gxlFile);
			fprintf(gxlFile, "\t<attr name=\"");
			xml_puts(gxlFile, sym->name + GXL_COMP_LEN);
			fprintf(gxlFile, "\">\n");
			tabover(gxlFile);
			fprintf(gxlFile, "\t\t");
			xml_puts(gxlFile, data->str[sym->id]);
			fprintf(gxlFile, "\n");
			tabover(gxlFile);
			fprintf(gxlFile, "\t</attr>\n");
		    }
		}
	    }
	}
    }
    AGATTRWF(obj) = !(AGATTRWF(obj));
}

static bool attrs_written(gxlstate_t * stp, void *obj) {
  return AGATTRWF(obj) != stp->attrsNotWritten;
}

static void
writeNode(gxlstate_t * stp, Agnode_t * n, FILE * gxlFile, Dict_t * d)
{
    char *name = agnameof(n);
    char *uniqueName = nodeID(stp, n);
    Level++;
    tabover(gxlFile);
    fprintf(gxlFile, "<node id=\"%s\">\n", uniqueName);

    printHref(gxlFile, n);

    if (strcmp(name, uniqueName)) {
	tabover(gxlFile);
	fprintf(gxlFile, "\t<attr name=\"name\">\n");
	tabover(gxlFile);
	fprintf(gxlFile, "\t\t<string>");
	xml_puts(gxlFile, name);
	fprintf(gxlFile, "</string>\n");
	tabover(gxlFile);
	fprintf(gxlFile, "\t</attr>\n");
    }

    if (!attrs_written(stp, n))
	writeNondefaultAttr(n, gxlFile, d);
    tabover(gxlFile);
    fprintf(gxlFile, "</node>\n");
    Level--;
}

static void writePort(Agedge_t * e, FILE * gxlFile, char *name)
{
    char *val = agget(e, name);
    if (val && val[0]) {
	tabover(gxlFile);
	fprintf(gxlFile, "\t<attr name=\"");
	xml_puts(gxlFile, name);
	fprintf(gxlFile, "\">\n");
	tabover(gxlFile);
	fprintf(gxlFile, "\t\t<string>");
	xml_puts(gxlFile, val);
	fprintf(gxlFile, "</string>\n");
	tabover(gxlFile);
	fprintf(gxlFile, "\t</attr>\n");
    }
}

static bool writeEdgeTest(Agraph_t *g, Agedge_t *e) {
    /* can use agedge() because we subverted the dict compar_f */
    for (Agraph_t *subg = agfstsubg(g); subg; subg = agnxtsubg(subg)) {
	if (agsubedge(subg, e, 0))
	    return false;
    }
    return true;
}

static void
writeEdge(gxlstate_t * stp, Agedge_t * e, FILE * gxlFile, Dict_t * d)
{
    Agnode_t *t = AGTAIL(e);
    Agnode_t *h = AGHEAD(e);

    Level++;
    tabover(gxlFile);
    fprintf(gxlFile, "<edge from=\"%s\" ", nodeID(stp, t));
    fprintf(gxlFile, "to=\"%s\"", nodeID(stp, h));
    edgeAttrs(gxlFile, e);

    if (stp->directed) {
	fprintf(gxlFile, " isdirected=\"true\"");
    } else {
	fprintf(gxlFile, " isdirected=\"false\"");
    }

    char *edge_id = agget(e, GXL_ID);
    if (!EMPTY(edge_id)) {
	fprintf(gxlFile, ">\n");
    } else {
	char *bp = createEdgeId(stp, e);
	fprintf(gxlFile, " id=\"%s\">\n", bp);
    }

    printHref(gxlFile, e);

    writePort(e, gxlFile, "tailport");
    writePort(e, gxlFile, "headport");
    if (!(attrs_written(stp, e)))
	writeNondefaultAttr(e, gxlFile, d);
    else
	writeEdgeName(e, gxlFile);
    tabover(gxlFile);
    fprintf(gxlFile, "</edge>\n");
    Level--;
}


#define writeval(n) (((Local_Agnodeinfo_t*)((n)->base.data))->written)

static void writeBody(gxlstate_t * stp, Agraph_t * g, FILE * gxlFile)
{
    writeSubgs(stp, g, gxlFile);
    Agdatadict_t *dd = agdatadict(g, false);
    for (Agnode_t *n = agfstnode(g); n; n = agnxtnode(g, n)) {
	Agnode_t *realn = agidnode(stp->root, AGID(n), 0);
	if (!writeval(realn)) {
	    writeval(realn) = 1;
	    writeNode(stp, n, gxlFile, dd->dict.n);
	}

	for (Agedge_t *e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    if (writeEdgeTest(g, e))
		writeEdge(stp, e, gxlFile, dd->dict.e);
	}
    }
}

static void iterateHdr(gxlstate_t * stp, Agraph_t * g)
{
    char *name = agnameof(g);
    char *gxlId = agget(g, GXL_ID);
    if (EMPTY(gxlId))
	gxlId = name;

    if (idexists(stp->idList, gxlId) || !legalGXLName(gxlId))
	gxlId = createGraphId(stp->idList);
    else
	gxlId = addid(stp->idList, gxlId);
    addToMap(stp->graphMap, name, gxlId);
}

static void iterate_subgs(gxlstate_t * stp, Agraph_t * g)
{
    for (Agraph_t *subg = agfstsubg(g); subg; subg = agnxtsubg(subg)) {
	iterateHdr(stp, subg);
	iterateBody(stp, subg);
    }
}


static void iterateBody(gxlstate_t * stp, Agraph_t * g)
{
    iterate_subgs(stp, g);
    for (Agnode_t *n = agfstnode(g); n; n = agnxtnode(g, n)) {
	char *nodename = agnameof(n);

	if (!mapLookup(stp->nodeMap, nodename)) {
	    char *gxlId = agget(n, GXL_ID);
	    if (EMPTY(gxlId))
		gxlId = nodename;
	    if (idexists(stp->idList, gxlId) || !legalGXLName(gxlId))
		gxlId = createNodeId(stp->idList);
	    else
		gxlId = addid(stp->idList, gxlId);
	    addToMap(stp->nodeMap, nodename, gxlId);
	}

	for (Agedge_t *e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    if (writeEdgeTest(g, e)) {
		char *edge_id = agget(e, GXL_ID);
		if (!EMPTY(edge_id))
		    addid(stp->idList, edge_id);
	    }
	}
    }
}

static gxlstate_t initState(Agraph_t *g) {
  gxlstate_t stp = {0};
  stp.nodeMap = dtopen(&nameDisc, Dtoset);
  stp.graphMap = dtopen(&nameDisc, Dtoset);
  stp.synNodeMap = dtopen(&nameDisc, Dtoset);
  stp.idList = dtopen(&idDisc, Dtoset);
  stp.attrsNotWritten = 0;
  stp.root = g;
  stp.directed = agisdirected(g) != 0;
  return stp;
}

static void freeState(gxlstate_t stp) {
  dtclose(stp.nodeMap);
  dtclose(stp.graphMap);
  dtclose(stp.synNodeMap);
  dtclose(stp.idList);
}

void gv_to_gxl(Agraph_t * g, FILE * gxlFile)
{
    gxlstate_t stp = initState(g);
    aginit(g, AGNODE, "node", sizeof(Local_Agnodeinfo_t), true);

    iterateHdr(&stp, g);
    iterateBody(&stp, g);

    Level = 0;

    fprintf(gxlFile, "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n");
    fprintf(gxlFile, "<gxl>\n");

    writeHdr(&stp, g, gxlFile, true);
    writeBody(&stp, g, gxlFile);
    writeTrl(g, gxlFile, true);

    fprintf(gxlFile, "</gxl>\n");

    freeState(stp);
}
