/**
 * @file
 * @brief <a href=https://en.wikipedia.org/wiki/GXL>GXL</a>-DOT convert subroutines
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

#include    <assert.h>
#include    "convert.h"
#include    <cgraph/agxbuf.h>
#include    <cgraph/alloc.h>
#include    <cgraph/exit.h>
#include    <cgraph/stack.h>
#include    <stdbool.h>
#include    <stdio.h>
#ifdef HAVE_EXPAT
#include    <expat.h>
#include    <ctype.h>
#include    <limits.h>
#include    <stdlib.h>

#ifndef XML_STATUS_ERROR
#define XML_STATUS_ERROR 0
#endif

#define NAMEBUF		100

#define GXL_ATTR	"_gxl_"
#define GXL_ID      "_gxl_id"
#define GXL_ROLE	"_gxl_role"
#define GXL_HYPER	"_gxl_hypergraph"
#define GXL_FROM    "_gxl_fromorder"
#define GXL_TO      "_gxl_toorder"
#define GXL_TYPE    "_gxl_type"
#define GXL_COMP    "_gxl_composite_"
#define GXL_LOC     "_gxl_locator_"

typedef enum {
  TAG_NONE,
  TAG_GRAPH,
  TAG_NODE,
  TAG_EDGE,
  TAG_HTML_LIKE_STRING,
} attr_t;

typedef struct {
    agxbuf xml_attr_name;
    agxbuf xml_attr_value;
    agxbuf composite_buffer;
    bool listen;
    attr_t closedElementType;
    attr_t globalAttrType;
    bool compositeReadState;
    bool edgeinverted;
    Dt_t *nameMap;
} userdata_t;

static Agraph_t *root;		/* root graph */
static attr_t Current_class;	/* Current element type */
static Agraph_t *G;		/* Current graph */
static Agnode_t *N;		/* Set if Current_class == TAG_NODE */
static Agedge_t *E;		/* Set if Current_class == TAG_EDGE */

static gv_stack_t Gstack;

typedef struct {
    Dtlink_t link;
    char *name;
    char *unique_name;
} namev_t;

static namev_t *make_nitem(namev_t *objp, Dtdisc_t *disc){
    (void)disc;

    namev_t *np = malloc(sizeof(namev_t));
    if (np == NULL)
	return NULL;
    np->name = objp->name;
    np->unique_name = 0;
    return np;
}

static void free_nitem(namev_t *np, Dtdisc_t *disc) {
    (void)disc;

    free(np->unique_name);
    free(np);
}

static Dtdisc_t nameDisc = {
    .key = offsetof(namev_t, name),
    .size = -1,
    .link = offsetof(namev_t, link),
    .makef = (Dtmake_f)make_nitem,
    .freef = (Dtfree_f)free_nitem,
};

static userdata_t genUserdata(void) {
  userdata_t user = {0};
  user.listen = false;
  user.closedElementType = TAG_NONE;
  user.globalAttrType = TAG_NONE;
  user.compositeReadState = false;
  user.edgeinverted = false;
  user.nameMap = dtopen(&nameDisc, Dtoset);
  return user;
}

static void freeUserdata(userdata_t ud) {
  dtclose(ud.nameMap);
  agxbfree(&ud.xml_attr_name);
  agxbfree(&ud.xml_attr_value);
  agxbfree(&ud.composite_buffer);
}

static void addToMap(Dt_t * map, char *name, char *uniqueName)
{
    namev_t obj;
    namev_t *objp;

    obj.name = name;
    objp = dtinsert(map, &obj);
    assert(objp->unique_name == 0);
    objp->unique_name = gv_strdup(uniqueName);
}

static char *mapLookup(Dt_t *nm, const char *name) {
    namev_t *objp = dtmatch(nm, name);
    if (objp)
	return objp->unique_name;
    else
	return 0;
}

static int isAnonGraph(const char *name)
{
    if (*name++ != '%')
	return 0;
    while (isdigit((int)*name))
	name++;			/* skip over digits */
    return (*name == '\0');
}

static void push_subg(Agraph_t * g)
{
  // insert the new graph
  stack_push(&Gstack, g);

  // save the root if this is the first graph
  if (stack_size(&Gstack) == 1) {
    root = g;
  }

  // update the top graph
  G = g;
}

static Agraph_t *pop_subg(void)
{
  // is the stack empty?
  if (stack_is_empty(&Gstack)) {
    fprintf(stderr, "gxl2gv: Gstack underflow in graph parser\n");
    graphviz_exit(EXIT_FAILURE);
  }

  // pop the top graph
  Agraph_t *g = stack_pop(&Gstack);

  // update the top graph
  if (!stack_is_empty(&Gstack))
    G = stack_top(&Gstack);

  return g;
}

static Agnode_t *bind_node(const char *name)
{
    N = agnode(G, (char *) name, 1);
    return N;
}

static Agedge_t *bind_edge(const char *tail, const char *head)
{
    Agnode_t *tailNode, *headNode;
    char *key = 0;

    tailNode = agnode(G, (char *) tail, 1);
    headNode = agnode(G, (char *) head, 1);
    E = agedge(G, tailNode, headNode, key, 1);
    return E;
}

static int get_xml_attr(char *attrname, const char **atts)
{
    int count = 0;
    while (atts[count] != NULL) {
	if (strcmp(attrname, atts[count]) == 0) {
	    return count + 1;
	}
	count += 2;
    }
    return -1;
}

static void setName(Dt_t * names, Agobj_t * n, char *value)
{
    Agsym_t *ap;
    char *oldName;

    ap = agattr(root, AGTYPE(n), GXL_ID, "");
    agxset(n, ap, agnameof(n));
    oldName = agxget(n, ap);	/* set/get gives us new copy */
    addToMap(names, oldName, value);
    agrename(n, value);
}

static char *defval = "";

static void
setNodeAttr(Agnode_t * np, char *name, char *value, userdata_t * ud,
  bool is_html)
{
    Agsym_t *ap;

    if (strcmp(name, "name") == 0) {
	setName(ud->nameMap, (Agobj_t *) np, value);
    } else {
	ap = agattr(root, AGNODE, name, 0);
	if (!ap)
	    ap = agattr(root, AGNODE, name, defval);
	if (is_html) {
	    char *val = agstrdup_html(root, value);
	    agxset(np, ap, val);
	    agstrfree(root, val); // drop the extra reference count we bumped for val
	} else {
	    agxset(np, ap, value);
	}
    }
}

#define NODELBL "node:"
#define NLBLLEN (sizeof(NODELBL)-1)
#define EDGELBL "edge:"
#define ELBLLEN (sizeof(EDGELBL)-1)

/* setGlobalNodeAttr:
 * Set global node attribute.
 * The names must always begin with "node:".
 */
static void
setGlobalNodeAttr(Agraph_t * g, char *name, char *value)
{
    if (strncmp(name, NODELBL, NLBLLEN))
	fprintf(stderr,
		"Warning: global node attribute %s in graph %s does not begin with the prefix %s\n",
		name, agnameof(g), NODELBL);
    else
	name += NLBLLEN;
    if ((g != root) && !agattr(root, AGNODE, name, 0))
	agattr(root, AGNODE, name, defval);
    agattr(G, AGNODE, name, value);
}

static void
setEdgeAttr(Agedge_t * ep, char *name, char *value, userdata_t * ud,
  bool is_html)
{
    Agsym_t *ap;
    char *attrname;

    if (strcmp(name, "headport") == 0) {
	if (ud->edgeinverted)
	    attrname = "tailport";
	else
	    attrname = "headport";
	ap = agattr(root, AGEDGE, attrname, 0);
	if (!ap)
	    ap = agattr(root, AGEDGE, attrname, defval);
    } else if (strcmp(name, "tailport") == 0) {
	if (ud->edgeinverted)
	    attrname = "headport";
	else
	    attrname = "tailport";
	ap = agattr(root, AGEDGE, attrname, 0);
	if (!ap)
	    ap = agattr(root, AGEDGE, attrname, defval);
    } else {
	ap = agattr(root, AGEDGE, name, 0);
	if (!ap)
	    ap = agattr(root, AGEDGE, name, defval);
    }

    if (is_html) {
	char *val = agstrdup_html(root, value);
	agxset(ep, ap, val);
	agstrfree(root, val); // drop the extra reference count we bumped for val
    } else {
	agxset(ep, ap, value);
    }
}

/* setGlobalEdgeAttr:
 * Set global edge attribute.
 * The names always begin with "edge:".
 */
static void
setGlobalEdgeAttr(Agraph_t * g, char *name, char *value)
{
    if (strncmp(name, EDGELBL, ELBLLEN))
	fprintf(stderr,
		"Warning: global edge attribute %s in graph %s does not begin with the prefix %s\n",
		name, agnameof(g), EDGELBL);
    else
	name += ELBLLEN;
    if ((g != root) && !agattr(root, AGEDGE, name, 0))
	agattr(root, AGEDGE, name, defval);
    agattr(g, AGEDGE, name, value);
}

static void
setGraphAttr(Agraph_t * g, char *name, char *value, userdata_t * ud)
{
    Agsym_t *ap;

    if ((g == root) && !strcmp(name, "strict") && !strcmp(value, "true")) {
	g->desc.strict = true;
    } else if (strcmp(name, "name") == 0)
	setName(ud->nameMap, (Agobj_t *) g, value);
    else {
	ap = agattr(root, AGRAPH, name, 0);
	if (ap)
	    agxset(g, ap, value);
	else if (g == root)
	    agattr(root, AGRAPH, name, value);
	else {
	    ap = agattr(root, AGRAPH, name, defval);
	    agxset(g, ap, value);
	}
    }
}

static void setAttr(char *name, char *value, userdata_t * ud, bool is_html)
{
    switch (Current_class) {
    case TAG_GRAPH:
	setGraphAttr(G, name, value, ud);
	break;
    case TAG_NODE:
	setNodeAttr(N, name, value, ud, is_html);
	break;
    case TAG_EDGE:
	setEdgeAttr(E, name, value, ud, is_html);
	break;
    default:
	break;
    }
}

/*------------- expat handlers ----------------------------------*/

static void
startElementHandler(void *userData, const char *name, const char **atts)
{
    int pos;
    userdata_t *ud = userData;
    Agraph_t *g = NULL;

    if (strcmp(name, "gxl") == 0) {
	/* do nothing */
    } else if (strcmp(name, "graph") == 0) {
	const char *edgeMode = "";
	char buf[NAMEBUF];	/* holds % + number */

	Current_class = TAG_GRAPH;
	if (ud->closedElementType == TAG_GRAPH) {
	    fprintf(stderr,
		    "Warning: Node contains more than one graph.\n");
	}
	pos = get_xml_attr("id", atts);
	if (pos <= 0) {
	    fprintf(stderr, "Error: Graph has no ID attribute.\n");
	    graphviz_exit(EXIT_FAILURE);
	}
	const char *id = atts[pos];
	pos = get_xml_attr("edgemode", atts);
	if (pos > 0) {
	    edgeMode = atts[pos];
	}

	if (stack_is_empty(&Gstack)) {
	    if (strcmp(edgeMode, "directed") == 0) {
		g = agopen((char *) id, Agdirected, &AgDefaultDisc);
	    } else if (strcmp(edgeMode, "undirected") == 0) {
		g = agopen((char *) id, Agundirected, &AgDefaultDisc);
	    } else {
		fprintf(stderr,
			"Warning: graph has no edgemode attribute");
		fprintf(stderr, " - assume directed\n");
		g = agopen((char *) id, Agdirected, &AgDefaultDisc);
	    }
	    push_subg(g);
	} else {
	    Agraph_t *subg;
	    if (isAnonGraph(id)) {
		static int anon_id = 1;
		snprintf(buf, sizeof(buf), "%%%d", anon_id++);
		id = buf;
	    }
	    subg = agsubg(G, (char *) id, 1);
	    push_subg(subg);
	}

	pos = get_xml_attr("role", atts);
	if (pos > 0) {
	    setGraphAttr(G, GXL_ROLE, (char *) atts[pos], ud);
	}

	pos = get_xml_attr("hypergraph", atts);
	if (pos > 0) {
	    setGraphAttr(G, GXL_HYPER, (char *) atts[pos], ud);
	}

    } else if (strcmp(name, "node") == 0) {
	Current_class = TAG_NODE;
	pos = get_xml_attr("id", atts);
	if (pos > 0) {
	    const char *attrname;
	    attrname = atts[pos];

	    if (attrname != NULL && strcmp(attrname, "") != 0) {
		bind_node(attrname);
	    }
	}

    } else if (strcmp(name, "edge") == 0) {
	const char *head = "", *tail = "";
	char *tname;
	Agnode_t *t;

	Current_class = TAG_EDGE;
	pos = get_xml_attr("from", atts);
	if (pos > 0)
	    tail = atts[pos];
	pos = get_xml_attr("to", atts);
	if (pos > 0)
	    head = atts[pos];

	tname = mapLookup(ud->nameMap, tail);
	if (tname)
	    tail = tname;

	tname = mapLookup(ud->nameMap, head);
	if (tname)
	    head = tname;

	bind_edge(tail, head);

	t = AGTAIL(E);
	tname = agnameof(t);

	if (strcmp(tname, tail) == 0) {
	    ud->edgeinverted = false;
	} else if (strcmp(tname, head) == 0) {
	    ud->edgeinverted = true;
	}

	pos = get_xml_attr("fromorder", atts);
	if (pos > 0) {
	    setEdgeAttr(E, GXL_FROM, (char *) atts[pos], ud, false);
	}

	pos = get_xml_attr("toorder", atts);
	if (pos > 0) {
	    setEdgeAttr(E, GXL_TO, (char *) atts[pos], ud, false);
	}

	pos = get_xml_attr("id", atts);
	if (pos > 0) {
	    setEdgeAttr(E, GXL_ID, (char *) atts[pos], ud, false);
	}
    } else if (strcmp(name, "attr") == 0) {
	const char *attrname = atts[get_xml_attr("name", atts)];

	agxbput(&ud->xml_attr_name, attrname);
	pos = get_xml_attr("kind", atts);

	if (pos > 0) {
	    if (strcmp("node", atts[pos]) == 0)
		ud->globalAttrType = TAG_NODE;
	    else if (strcmp("edge", atts[pos]) == 0)
		ud->globalAttrType = TAG_EDGE;
	    else if (strcmp("graph", atts[pos]) == 0)
		ud->globalAttrType = TAG_GRAPH;
	    else if (strcmp("HTML-like string", atts[pos]) == 0)
		ud->globalAttrType = TAG_HTML_LIKE_STRING;
	} else {
	    ud->globalAttrType = TAG_NONE;
	}

    } else if (strcmp(name, "string") == 0
	       || strcmp(name, "bool") == 0
	       || strcmp(name, "int") == 0 || strcmp(name, "float") == 0) {

	ud->listen = true;
	if (ud->compositeReadState) {
	    agxbprint(&ud->composite_buffer, "<%s>", name);
	}
    } else if (strcmp(name, "rel") == 0 || strcmp(name, "relend") == 0) {
	fprintf(stderr, "%s element is ignored by DOT\n", name);
    } else if (strcmp(name, "type") == 0) {
	pos = get_xml_attr("xlink:href", atts);
	if (pos > 0) {
	    setAttr(GXL_TYPE, (char *) atts[pos], ud, false);
	}
    } else if (strcmp(name, "locator") == 0) {
	pos = get_xml_attr("xlink:href", atts);
	if (pos > 0) {
	    const char *href = atts[pos];
	    agxbprint(&ud->xml_attr_value, "%s%s", GXL_LOC, href);
	}
    } else if (strcmp(name, "seq") == 0
	       || strcmp(name, "set") == 0
	       || strcmp(name, "bag") == 0
	       || strcmp(name, "tup") == 0 || strcmp(name, "enum") == 0) {

	ud->compositeReadState = true;
	agxbprint(&ud->composite_buffer, "<%s>", name);
    } else {
	/* must be some extension */
	fprintf(stderr,
		"Unknown node %s; DOT does not support extensions.\n",
		name);
    }
}

static void endElementHandler(void *userData, const char *name)
{
    userdata_t *ud = userData;

    if (strcmp(name, "graph") == 0) {
	pop_subg();
	ud->closedElementType = TAG_GRAPH;
    } else if (strcmp(name, "node") == 0) {
	Current_class = TAG_GRAPH;
	N = 0;
	ud->closedElementType = TAG_NODE;
    } else if (strcmp(name, "edge") == 0) {
	Current_class = TAG_GRAPH;
	E = 0;
	ud->closedElementType = TAG_EDGE;
	ud->edgeinverted = false;
    } else if (strcmp(name, "attr") == 0) {
	agxbuf new_name = {0};
	char *value;

	ud->closedElementType = TAG_NONE;
	if (ud->compositeReadState) {
	    agxbprint(&new_name, "%s%s", GXL_COMP, agxbuse(&ud->xml_attr_name));
	    value = agxbuse(&ud->composite_buffer);
	    agxbclear(&ud->xml_attr_value);
	    ud->compositeReadState = false;
	} else {
	    agxbput(&new_name, agxbuse(&ud->xml_attr_name));
	    value = agxbuse(&ud->xml_attr_value);
	}

	switch (ud->globalAttrType) {
	case TAG_NONE:
	    setAttr(agxbuse(&new_name), value, ud, false);
	    break;
	case TAG_NODE:
	    setGlobalNodeAttr(G, agxbuse(&new_name), value);
	    break;
	case TAG_EDGE:
	    setGlobalEdgeAttr(G, agxbuse(&new_name), value);
	    break;
	case TAG_GRAPH:
	    setGraphAttr(G, agxbuse(&new_name), value, ud);
	    break;
	case TAG_HTML_LIKE_STRING:
	    setAttr(agxbuse(&new_name), value, ud, true);
	    break;
	}
	agxbfree(&new_name);
	ud->globalAttrType = TAG_NONE;
    } else if (strcmp(name, "string") == 0
	       || strcmp(name, "bool") == 0
	       || strcmp(name, "int") == 0 || strcmp(name, "float") == 0) {
	ud->listen = false;
	if (ud->compositeReadState) {
	    agxbprint(&ud->composite_buffer, "</%s>", name);
	}
    } else if (strcmp(name, "seq") == 0
	       || strcmp(name, "set") == 0
	       || strcmp(name, "bag") == 0
	       || strcmp(name, "tup") == 0 || strcmp(name, "enum") == 0) {
	agxbprint(&ud->composite_buffer, "</%s>", name);
    }
}

static void characterDataHandler(void *userData, const char *s, int length)
{
    userdata_t *ud = userData;

    assert(length >= 0 && "Expat returned negative length data");
    size_t len = (size_t)length;

    if (!ud->listen)
	return;

    if (ud->compositeReadState) {
	agxbput_n(&ud->composite_buffer, s, len);
	return;
    }

    agxbput_n(&ud->xml_attr_value, s, len);
}

Agraph_t *gxl_to_gv(FILE * gxlFile)
{
    char buf[BUFSIZ];
    int done;
    userdata_t udata = genUserdata();
    XML_Parser parser = XML_ParserCreate(NULL);

    XML_SetUserData(parser, &udata);
    XML_SetElementHandler(parser, startElementHandler, endElementHandler);
    XML_SetCharacterDataHandler(parser, characterDataHandler);

    Current_class = TAG_GRAPH;
    root = 0;
    do {
	size_t len = fread(buf, 1, sizeof(buf), gxlFile);
	if (len == 0)
	    break;
	done = len < sizeof(buf);
	assert(len <= (size_t)INT_MAX && "too large data for Expat API");
	if (XML_Parse(parser, buf, (int)len, done) == XML_STATUS_ERROR) {
	    fprintf(stderr,
		    "%s at line %lu\n",
		    XML_ErrorString(XML_GetErrorCode(parser)),
		    XML_GetCurrentLineNumber(parser));
	    graphviz_exit(1);
	}
    } while (!done);
    XML_ParserFree(parser);
    freeUserdata(udata);
    stack_reset(&Gstack);

    return root;
}

#endif
