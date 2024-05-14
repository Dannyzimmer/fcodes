/**
 * @file
 * @brief merge and pack disjoint graphs
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
 * Written by Emden R. Gansner
 */



#include "config.h"

#include <getopt.h>

#include <assert.h>
#include <gvc/gvc.h>
#include <cgraph/alloc.h>
#include <cgraph/exit.h>
#include <common/render.h>
#include <neatogen/neatoprocs.h>
#include <ingraphs/ingraphs.h>
#include <iostream>
#include <limits>
#include <map>
#include <pack/pack.h>
#include <set>
#include <stddef.h>
#include <string>
#include <utility>
#include <vector>
#include "openFile.h"

extern "C" {
#ifdef GVDLL
  __declspec(dllimport)
#endif
extern gvplugin_library_t gvplugin_neato_layout_LTX_library;
}

lt_symlist_t lt_preloaded_symbols[] = {
#ifdef GVDLL
	{ "gvplugin_neato_layout_LTX_library", 0 },
#else
	{ "gvplugin_neato_layout_LTX_library", &gvplugin_neato_layout_LTX_library },
#endif
	{ 0, 0 }
};

/* gvpack:
 * Input consists of graphs in dot format.
 * The graphs should have pos, width and height set for nodes, 
 * bb set for clusters, and, optionally, spline info for edges.
 * The graphs are packed geometrically and combined
 * into a single output graph, ready to be sent to neato -s -n2.
 *  -m <i> specifies the margin, in points, about each graph.
 */

typedef struct {
    Dtlink_t link;
    char *name;
    char *value;
    size_t cnt;
} attr_t;

typedef struct {
    Dtlink_t link;
    char *name;
    size_t cnt;
} pair_t;

static int verbose = 0;
static char **myFiles = 0;
static FILE *outfp;		/* output; stdout by default */
static Agdesc_t kind;		/* type of graph */
static std::vector<attr_t> G_args; // Storage for -G arguments
static bool doPack;              /* Do packing if true */
static char* gname = const_cast<char*>("root");

#define NEWNODE(n) ((node_t*)ND_alg(n))

static const char useString[] =
    "Usage: gvpack [-gnuv?] [-m<margin>] {-array[_rc][n]] [-o<outf>] <files>\n\
  -n          - use node granularity\n\
  -g          - use graph granularity\n\
  -array*     - pack as array of graphs\n\
  -G<n>=<v>   - attach name/value attribute to output graph\n\
  -m<n>       - set margin to <n> points\n\
  -s<gname>   - use <gname> for name of root graph\n\
  -o<outfile> - write output to <outfile>\n\
  -u          - no packing; just combine graphs\n\
  -v          - verbose\n\
  -?          - print usage\n\
If no files are specified, stdin is used\n";

static void usage(int v)
{
    std::cout << useString;
    graphviz_exit(v);
}

/* setNameValue:
 * If arg is a name-value pair, add it to the list
 * and return 0; otherwise, return 1.
 */
static int setNameValue(char *arg)
{
    char *p;
    char *rhs = const_cast<char*>("true");

    if ((p = strchr(arg, '='))) {
	*p++ = '\0';
	rhs = p;
    }
    G_args.push_back(attr_t{{0, {0}}, arg, rhs, 0});

    return 0;
}

/* setUInt:
 * If arg is an integer, value is stored in v
 * and function returns 0; otherwise, returns 1.
 */
static int setUInt(unsigned int *v, char *arg)
{
    char *p;
    unsigned int i;

    i = (unsigned int) strtol(arg, &p, 10);
    if (p == arg) {
	std::cerr << "Error: bad value in flag -" << (arg - 1) << " - ignored\n";
	return 1;
    }
    *v = i;
    return 0;
}

static Agsym_t *agraphattr(Agraph_t *g, char *name, const char *value) {
    return agattr(g, AGRAPH, name, value);
}

static Agsym_t *agnodeattr(Agraph_t *g, char *name, const char *value) {
    return agattr(g, AGNODE, name, value);
}

static Agsym_t *agedgeattr(Agraph_t *g, char *name, const char *value) {
    return agattr(g, AGEDGE, name, value);
}

/* init:
 */
static void init(int argc, char *argv[], pack_info* pinfo)
{
    int c;

    agnodeattr(nullptr, const_cast<char*>("label"), NODENAME_ESC);
    pinfo->mode = l_clust;
    pinfo->margin = CL_OFFSET;
    pinfo->doSplines = true; // Use edges in packing
    pinfo->fixed = nullptr;
    pinfo->sz = 0;

    opterr = 0;
    while ((c = getopt(argc, argv, ":na:gvum:s:o:G:?")) != -1) {
	switch (c) {
	case 'a': {
	    auto buf = std::string("a") + optarg + "\n";
	    parsePackModeInfo(buf.c_str(), pinfo->mode, pinfo);
	    break;
	}
	case 'n':
	    parsePackModeInfo ("node", pinfo->mode, pinfo);
	    break;
	case 's':
	    gname = optarg;
	    break;
	case 'g':
	    parsePackModeInfo ("graph", pinfo->mode, pinfo);
	    break;
	case 'm':
	    setUInt(&pinfo->margin, optarg);
	    break;
	case 'o':
	    if (outfp != nullptr)
		fclose(outfp);
	    outfp = openFile("gvpack", optarg, "w");
	    break;
	case 'u':
	    pinfo->mode = l_undef;
	    break;
	case 'G':
	    if (*optarg)
		setNameValue(optarg);
	    else
		std::cerr << "gvpack: option -G missing argument - ignored\n";
	    break;
	case 'v':
	    verbose = 1;
	    Verbose = 1;
	    break;
	case ':':
	    std::cerr << "gvpack: option -" << (char)optopt
	              << " missing argument - ignored\n";
	    break;
	case '?':
	    if (optopt == '\0' || optopt == '?')
		usage(0);
	    else {
		std::cerr << "gvpack: option -" << (char)optopt << " unrecognized\n";
		usage(1);
	    }
	    break;
	}
    }
    argv += optind;
    argc -= optind;

    if (argc > 0) {
	myFiles = argv;
    }
    if (!outfp)
	outfp = stdout;		/* stdout the default */
    if (verbose)
	std::cerr << "  margin " << pinfo->margin << '\n';
}

/* init_node_edge:
 * initialize node and edge attributes
 */
static void init_node_edge(Agraph_t * g)
{
    node_t *n;
    edge_t *e;
    int nG = agnnodes(g);
    attrsym_t *N_pos = agfindnodeattr(g, const_cast<char*>("pos"));
    attrsym_t *N_pin = agfindnodeattr(g, const_cast<char*>("pin"));

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	neato_init_node(n);
	user_pos(N_pos, N_pin, n, nG);	/* set user position if given */
    }
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e))
	    common_init_edge(e);
    }
}

/* init_graph:
 * Initialize attributes. We always do the minimum required by
 * libcommon. If fill is true, we use init_nop (neato -n) to
 * read in attributes relevant to the layout.
 */
static void init_graph(Agraph_t *g, bool fill, GVC_t *gvc) {
    int d;
    node_t *n;
    edge_t *e;

    aginit (g, AGRAPH, "Agraphinfo_t", sizeof(Agraphinfo_t), true);
    aginit (g, AGNODE, "Agnodeinfo_t", sizeof(Agnodeinfo_t), true);
    aginit (g, AGEDGE, "Agedgeinfo_t", sizeof(Agedgeinfo_t), true);
    GD_gvc(g) = gvc;
    graph_init(g, false);
    d = late_int(g, agfindgraphattr(g, const_cast<char*>("dim")), 2, 2);
    if (d != 2) {
	std::cerr << "Error: graph " << agnameof(g) << " has dim = " << d
	          << " (!= 2)\n";
	graphviz_exit(1);
    }
    Ndim = GD_ndim(g) = 2;
    init_node_edge(g);
    if (fill) {
	int ret = init_nop(g,0);
	if (ret) {
	    if (ret < 0)
		std::cerr << "Error loading layout info from graph " << agnameof(g) << '\n';
	    else if (ret > 0)
		std::cerr << "gvpack does not support backgrounds as found in graph "
		          << agnameof(g) << '\n';
	    graphviz_exit(1);
	}
	if (Concentrate) { /* check for edges without pos info */
	    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
		for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
		    if (ED_spl(e) == nullptr) ED_edge_type(e) = IGNORED;
		}
	    }
	}
    }
}

/* cloneAttrs:
 * Copy all attributes from old object to new. Assume
 * attributes have been initialized.
 */
static void cloneDfltAttrs(Agraph_t *old, Agraph_t *new_graph, int kind) {
    Agsym_t *a;

    for (a = agnxtattr(old, kind, 0); a; a =  agnxtattr(old, kind, a)) {
	if (aghtmlstr(a->defval)) {
	    char *s = agstrdup_html(new_graph, a->defval);
	    agattr(new_graph, kind, a->name, s);
	    agstrfree(new_graph, s); // drop the extra reference count we bumped for s
	} else {
	    agattr(new_graph, kind, a->name, a->defval);
	}
    }
}
static void cloneAttrs(void *old, void *new_graph) {
    int kind = AGTYPE(old);
    Agsym_t *a;
    char* s;
    Agraph_t *g = agroot(old);
    Agraph_t *ng = agroot(new_graph);

    for (a = agnxtattr(g, kind, 0); a; a =  agnxtattr(g, kind, a)) {
	s = agxget (old, a);
	if (aghtmlstr(s)) {
	    char *scopy = agstrdup_html(ng, s);
	    agset(new_graph, a->name, scopy);
	    agstrfree(ng, scopy); // drop the extra reference count we bumped for scopy
	} else {
	    agset(new_graph, a->name, s);
	}
    }
}

/* cloneEdge:
 * Note that here, and in cloneNode and cloneCluster,
 * we do a shallow copy. We thus assume that attributes
 * are not disturbed. In particular, we assume they are
 * not deallocated.
 */
static void cloneEdge(Agedge_t *old, Agedge_t *new_edge) {
  cloneAttrs(old, new_edge);
  ED_spl(new_edge) = ED_spl(old);
  ED_edge_type(new_edge) = ED_edge_type(old);
  ED_label(new_edge) = ED_label(old);
  ED_head_label(new_edge) = ED_head_label(old);
  ED_tail_label(new_edge) = ED_tail_label(old);
  ED_xlabel(new_edge) = ED_xlabel(old);
}

/* cloneNode:
 */
static void cloneNode(Agnode_t *old, Agnode_t *new_node) {
  cloneAttrs(old, new_node);
  ND_coord(new_node).x = POINTS(ND_pos(old)[0]);
  ND_coord(new_node).y = POINTS(ND_pos(old)[1]);
  ND_height(new_node) = ND_height(old);
  ND_ht(new_node) = ND_ht(old);
  ND_width(new_node) = ND_width(old);
  ND_lw(new_node) = ND_lw(old);
  ND_rw(new_node) = ND_rw(old);
  ND_shape(new_node) = ND_shape(old);
  ND_shape_info(new_node) = ND_shape_info(old);
  ND_xlabel(new_node) = ND_xlabel(old);
}

/* cloneCluster:
 */
static void cloneCluster(Agraph_t *old, Agraph_t *new_cluster) {
  // string attributes were cloned as subgraphs
  GD_label(new_cluster) = GD_label(old);
  GD_bb(new_cluster) = GD_bb(old);
}

namespace {
/// a value of a graph attribute, possibly set multiple times
struct AttributeValue {
  std::string value; ///< text of the value
  size_t instances;  ///< number of times this attribute was seen
};
} // namespace

/// attribute name → value collection of those we have seen
using attr_map_t = std::map<std::string, AttributeValue>;

/* fillDict:
 * Fill newdict with all the name-value attributes of
 * objp. If the attribute has already been defined and
 * has a different default, set default to "".
 */
static void fillDict(attr_map_t &newdict, Agraph_t *g, int kind) {

  for (Agsym_t *a = agnxtattr(g, kind, 0); a; a = agnxtattr(g, kind, a)) {
    char *name = a->name;
    char *value = a->defval;
    auto it = newdict.find(name);
    if (it == newdict.end()) {
      newdict.insert({name, AttributeValue{value, 1}});
    } else if (it->second.value == value)
      ++it->second.instances;
  }
}

/* fillGraph:
 * Use all the name-value entries in the dictionary d to define
 * to define universal node/edge/graph attributes for g.
 * For a non-empty default value, the attribute must be defined and the
 * same in all graphs.
 */
static void fillGraph(Agraph_t *g, const attr_map_t &d,
                      Agsym_t *(*setf)(Agraph_t *, char *, const char *),
                      size_t cnt) {
  for (const auto &kv : d) {
    const std::string &name = kv.first;
    const std::string &value = kv.second.value;
    const size_t &attr_cnt = kv.second.instances;
    if (cnt == attr_cnt)
      setf(g, const_cast<char *>(name.c_str()), value.c_str());
    else
      setf(g, const_cast<char *>(name.c_str()), "");
  }
}

/* initAttrs:
 * Initialize the attributes of root as the union of the
 * attributes in the graphs gs.
 */
static void initAttrs(Agraph_t *root, std::vector<Agraph_t*> &gs) {
    attr_map_t n_attrs;
    attr_map_t e_attrs;
    attr_map_t g_attrs;

    for (Agraph_t *g : gs) {
	fillDict(g_attrs, g, AGRAPH);
	fillDict(n_attrs, g, AGNODE);
	fillDict(e_attrs, g, AGEDGE);
    }

    fillGraph(root, g_attrs, agraphattr, gs.size());
    fillGraph(root, n_attrs, agnodeattr, gs.size());
    fillGraph(root, e_attrs, agedgeattr, gs.size());
}

/* cloneGraphAttr:
 */
static void cloneGraphAttr(Agraph_t * g, Agraph_t * ng)
{
    cloneAttrs(g, ng);
    cloneDfltAttrs(g, ng, AGNODE);
    cloneDfltAttrs(g, ng, AGEDGE);
}

/// names that have already been used during generation
using used_t = std::multiset<std::string>;

/* xName:
 * Create a name for an object in the new graph using the
 * dictionary names and the old name. If the old name has not
 * been used, use it and add it to names. If it has been used,
 * create a new name using the old name and a number.
 * Note that returned string will immediately made into an agstring.
 */
static std::string xName(used_t &names, char *oldname) {
  size_t previous_instances = names.count(oldname);
  names.insert(oldname);
  if (previous_instances > 0) {
    return std::string(oldname) + "_gv" + std::to_string(previous_instances);
  }
  return oldname;
}

#define MARK(e) (ED_alg(e) = e)
#define MARKED(e) (ED_alg(e))
#define ISCLUSTER(g) (!strncmp(agnameof(g),"cluster",7))
#define SETCLUST(g,h) (GD_alg(g) = h)
#define GETCLUST(g) ((Agraph_t*)GD_alg(g))

/* cloneSubg:
 * Create a copy of g in ng, copying attributes, inserting nodes
 * and adding edges.
 */
static void
cloneSubg(Agraph_t *g, Agraph_t *ng, Agsym_t *G_bb, used_t &gnames) {
    node_t *n;
    node_t *nn;
    edge_t *e;
    edge_t *ne;
    node_t *nt;
    node_t *nh;
    Agraph_t *subg;
    Agraph_t *nsubg;

    cloneGraphAttr(g, ng);
    if (doPack)
	agxset(ng, G_bb, "");	/* Unset all subgraph bb */

    /* clone subgraphs */
    for (subg = agfstsubg (g); subg; subg = agnxtsubg (subg)) {
	nsubg = agsubg(ng, const_cast<char*>(xName(gnames, agnameof(subg)).c_str()),
	               1);
	agbindrec (nsubg, "Agraphinfo_t", sizeof(Agraphinfo_t), true);
	cloneSubg(subg, nsubg, G_bb, gnames);
	/* if subgraphs are clusters, point to the new 
	 * one so we can find it later.
	 */
	if (ISCLUSTER(subg))
	    SETCLUST(subg, nsubg);
    }

    /* add remaining nodes */
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	nn = NEWNODE(n);
        agsubnode(ng, nn, 1);
    }

    /* add remaining edges. libgraph doesn't provide a way to find
     * multiedges, so we add edges bottom up, marking edges when added.
     */
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    if (MARKED(e))
		continue;
	    nt = NEWNODE(agtail(e));
	    nh = NEWNODE(aghead(e));
	    ne = agedge(ng, nt, nh, nullptr, 1);
	    agbindrec (ne, "Agedgeinfo_t", sizeof(Agedgeinfo_t), true);
	    cloneEdge(e, ne);
	    MARK(e);
	}
    }
}

/* cloneClusterTree:
 * Given old and new subgraphs which are corresponding
 * clusters, recursively create the subtree of clusters
 * under ng using the subtree of clusters under g.
 */
static void cloneClusterTree(Agraph_t * g, Agraph_t * ng)
{
    int i;

    cloneCluster(g, ng);

    if (GD_n_cluster(g)) {
	GD_n_cluster(ng) = GD_n_cluster(g);
	GD_clust(ng) = reinterpret_cast<Agraph_t**>(gv_calloc(1 + GD_n_cluster(g), sizeof(Agraph_t*)));
	for (i = 1; i <= GD_n_cluster(g); i++) {
	    Agraph_t *c = GETCLUST(GD_clust(g)[i]);
	    GD_clust(ng)[i] = c;
	    cloneClusterTree(GD_clust(g)[i], c);
	}
    }
}

/* cloneGraph:
 * Create and return a new graph which is the logical union
 * of the graphs gs. 
 */
static Agraph_t *cloneGraph(std::vector<Agraph_t*> &gs, GVC_t *gvc) {
    Agraph_t *root;
    Agraph_t *subg;
    Agnode_t *n;
    Agnode_t *np;
    Agsym_t *G_bb;
    Agsym_t *rv;
    bool doWarn = true;

    if (verbose)
	std::cerr << "Creating clone graph\n";
    root = agopen(gname, kind, &AgDefaultDisc);
    initAttrs(root, gs);
    G_bb = agfindgraphattr(root, const_cast<char*>("bb"));
    if (doPack) assert(G_bb);

    /* add command-line attributes */
    for (attr_t &a : G_args) {
	rv = agfindgraphattr(root, a.name);
	if (rv)
	    agxset(root, rv, a.value);
	else
	    agattr(root, AGRAPH, a.name, a.value);
    }

    /* do common initialization. This will handle root's label. */
    init_graph(root, false, gvc);
    State = GVSPLINES;

    used_t gnames; // dict of used subgraph names
    used_t nnames; // dict of used node names
    for (size_t i = 0; i < gs.size(); i++) {
	Agraph_t *g = gs[i];
	if (verbose)
	    std::cerr << "Cloning graph " << agnameof(g) << '\n';
	GD_n_cluster(root) += GD_n_cluster(g);
	GD_has_labels(root) |= GD_has_labels(g);

	/* Clone nodes, checking for node name conflicts */
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    if (doWarn && agfindnode(root, agnameof(n))) {
		std::cerr << "Warning: node " << agnameof(n) << " in graph[" << i << "] "
		          << agnameof(g) << " already defined\n"
		          << "Some nodes will be renamed.\n";
		doWarn = false;
	    }
	    np = agnode(root, const_cast<char*>(xName(nnames, agnameof(n)).c_str()),
	                1);
	    agbindrec (np, "Agnodeinfo_t", sizeof(Agnodeinfo_t), true);
	    ND_alg(n) = np;
	    cloneNode(n, np);
	}

	/* wrap the clone of g in a subgraph of root */
	subg = agsubg(root, const_cast<char*>(xName(gnames, agnameof(g)).c_str()),
	              1);
	agbindrec (subg, "Agraphinfo_t", sizeof(Agraphinfo_t), true);
	cloneSubg(g, subg, G_bb, gnames);
    }

    /* set up cluster tree */
    if (GD_n_cluster(root)) {
	int j, idx;
	GD_clust(root) = reinterpret_cast<graph_t**>(gv_calloc(1 + GD_n_cluster(root), sizeof(graph_t*)));

	idx = 1;
	for (Agraph_t *g : gs) {
	    for (j = 1; j <= GD_n_cluster(g); j++) {
		Agraph_t *c = GETCLUST(GD_clust(g)[j]);
		GD_clust(root)[idx++] = c;
		cloneClusterTree(GD_clust(g)[j], c);
	    }
	}
    }

    return root;
}

/* readGraphs:
 * Read input, parse the graphs, use init_nop (neato -n) to
 * read in all attributes need for layout.
 * Return the list of graphs. If cp != nullptr, set it to the number
 * of graphs read.
 * We keep track of the types of graphs read. They all must be
 * either directed or undirected. If all graphs are strict, the
 * combined graph will be strict; other, the combined graph will
 * be non-strict.
 */
static std::vector<Agraph_t*> readGraphs(GVC_t *gvc) {
    Agraph_t *g;
    std::vector<Agraph_t*> gs;
    ingraph_state ig;
    int kindUnset = 1;

    /* set various state values */
    PSinputscale = POINTS_PER_INCH;
    Nop = 2;

    newIngraph(&ig, myFiles);
    while ((g = nextGraph(&ig)) != 0) {
	if (verbose)
	    std::cerr << "Reading graph " << agnameof(g) << '\n';
	if (agnnodes(g) == 0) {
	    std::cerr << "Graph " << agnameof(g) << " is empty - ignoring\n";
	    continue;
	}
	if (kindUnset) {
	    kindUnset = 0;
	    kind = g->desc;
	}
	else if (kind.directed != g->desc.directed) {
	    std::cerr << "Error: all graphs must be directed or undirected\n";
	    graphviz_exit(1);
	} else if (!agisstrict(g))
	    kind = g->desc;
	init_graph(g, doPack, gvc);
	gs.push_back(g);
    }

    return gs;
}

/* compBB:
 * Compute the bounding box containing the graphs.
 * We can just use the bounding boxes of the graphs.
 */
static boxf compBB(std::vector<Agraph_t*> &gs) {
    boxf bb, bb2;

    bb = GD_bb(gs[0]);

    for (size_t i = 1; i < gs.size(); i++) {
	bb2 = GD_bb(gs[i]);
	bb.LL.x = MIN(bb.LL.x, bb2.LL.x);
	bb.LL.y = MIN(bb.LL.y, bb2.LL.y);
	bb.UR.x = MAX(bb.UR.x, bb2.UR.x);
	bb.UR.y = MAX(bb.UR.y, bb2.UR.y);
    }

    return bb;
}

#ifdef DEBUG
void dump(Agraph_t * g)
{
    node_t *v;
    edge_t *e;

    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	std::cerr << agnameof(v) << '\n';
	for (e = agfstout(g, v); e; e = agnxtout(g, e)) {
	    std::cerr << "  " << agnameof(agtail(e)) << " -- " << agnameof(aghead(e))
	              << '\n';
	}
    }
}

void dumps(Agraph_t * g)
{
    graph_t *subg;

    for (subg = agfstsubg(g); subg; subg = agnxtsubg(subg)) {
	dump(subg);
	std::cerr << "====\n";
    }
}
#endif

int main(int argc, char *argv[])
{
    Agraph_t *g;
    pack_info pinfo;
    GVC_t * gvc;

    init(argc, argv, &pinfo);

    doPack = (pinfo.mode != l_undef);

#if defined(_WIN32)
    lt_preloaded_symbols[0].address = &gvplugin_neato_layout_LTX_library;
#endif
    gvc = gvContextPlugins(lt_preloaded_symbols, DEMAND_LOADING);
    std::vector<Agraph_t*> gs = readGraphs(gvc);
    if (gs.empty())
	graphviz_exit(0);

    /* pack graphs */
    if (doPack) {
	assert(gs.size() <= INT_MAX);
	if (packGraphs((int)gs.size(), gs.data(), 0, &pinfo)) {
	    std::cerr << "gvpack: packing of graphs failed.\n";
	    graphviz_exit(1);
	}
    }

    /* create union graph and copy attributes */
    g = cloneGraph(gs, gvc);

    /* compute new top-level bb and set */
    if (doPack) {
	GD_bb(g) = compBB(gs);
	dotneato_postprocess(g);
	attach_attrs(g);
    }
    agwrite(g, outfp);
    graphviz_exit(0);
}
