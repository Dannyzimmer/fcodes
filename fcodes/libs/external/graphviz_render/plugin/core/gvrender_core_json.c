/*************************************************************************
 * Copyright (c) 2015 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include "config.h"

#ifdef _WIN32
#include <io.h>
#endif

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <common/macros.h>
#include <common/const.h>
#include <xdot/xdot.h>

#include <gvc/gvplugin_render.h>
#include <gvc/gvplugin_device.h>
#include <cgraph/alloc.h>
#include <cgraph/startswith.h>
#include <cgraph/unreachable.h>
#include <common/utils.h>
#include <gvc/gvc.h>
#include <gvc/gvio.h>
#include <gvc/gvcint.h>

typedef enum {
	FORMAT_JSON,
	FORMAT_JSON0,
	FORMAT_DOT_JSON,
	FORMAT_XDOT_JSON,
} format_type;

typedef struct {
    int Level;
    bool isLatin;
    bool doXDot;
} state_t;

typedef struct {
    Agrec_t h;
    int id;
} gvid_t;

#define ID "id"
#define ND_gid(n) (((gvid_t*)aggetrec(n, ID, 0))->id) 
#define ED_gid(n) (((gvid_t*)aggetrec(n, ID, 0))->id) 
#define GD_gid(n) (((gvid_t*)aggetrec(n, ID, 0))->id) 

static bool IS_CLUSTER(Agraph_t *s) {
  return startswith(agnameof(s), "cluster");
}

static void json_begin_graph(GVJ_t *job)
{
    if (job->render.id == FORMAT_JSON) {
	GVC_t* gvc = gvCloneGVC (job->gvc); 
	graph_t *g = job->obj->u.g;
	gvRender (gvc, g, "xdot", NULL); 
	gvFreeCloneGVC (gvc);
    }
    else if (job->render.id == FORMAT_JSON0) {
	attach_attrs(job->gvc->g);
    }
}

#define LOCALNAMEPREFIX		'%'

/** Convert dot string to a valid json string embedded in double quotes and
 *   output
 *
 * \param ins Input string
 * \param sp State, used to determine encoding of the input string
 * \param job Output job
 */
static void stoj(char *ins, state_t *sp, GVJ_t *job) {
    char* s;
    char* input;
    char c;

    if (sp->isLatin)
	input = latin1ToUTF8 (ins);
    else
	input = ins;

    gvputc(job, '"');
    for (s = input; (c = *s); s++) {
	switch (c) {
	case '"' :
	    gvputs(job, "\\\"");
	    break;
	case '\\' :
	    gvputs(job, "\\\\");
	    break;
	case '/' :
	    gvputs(job, "\\/");
	    break;
	case '\b' :
	    gvputs(job, "\\b");
	    break;
	case '\f' :
	    gvputs(job, "\\f");
	    break;
	case '\n' :
	    gvputs(job, "\\n");
	    break;
	case '\r' :
	    gvputs(job, "\\r");
	    break;
	case '\t' :
	    gvputs(job, "\\t");
	    break;
	default :
	    gvputc(job, c);
	    break;
	}
    }
    gvputc(job, '"');

    if (sp->isLatin)
	free (input);
}

static void indent(GVJ_t * job, int level)
{
    int i;
    for (i = level; i > 0; i--)
	gvputs(job, "  ");
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

static void write_polyline (GVJ_t * job, xdot_polyline* polyline)
{
    const size_t cnt = polyline->cnt;
    xdot_point* pts = polyline->pts;

    gvprintf(job, "\"points\": [");
    for (size_t i = 0; i < cnt; i++) {
	if (i > 0) gvprintf(job, ",");
	gvprintf(job, "[%.03f,%.03f]", pts[i].x, pts[i].y);
    }
    gvprintf(job, "]\n");
}

static void write_stops (GVJ_t * job, int n_stops, xdot_color_stop* stp, state_t* sp)
{
    int i;

    gvprintf(job, "\"stops\": [");
    for (i = 0; i < n_stops; i++) {
	if (i > 0) gvprintf(job, ",");
	gvprintf(job, "{\"frac\": %.03f, \"color\": ", stp[i].frac);
	stoj(stp[i].color, sp, job);
	gvputc(job, '}');
    }
    gvprintf(job, "]\n");
} 

static void write_radial_grad (GVJ_t * job, xdot_radial_grad* rg, state_t* sp)
{
    indent(job, sp->Level);
    gvprintf(job, "\"p0\": [%.03f,%.03f,%.03f],\n", rg->x0, rg->y0, rg->r0); 
    indent(job, sp->Level);
    gvprintf(job, "\"p1\": [%.03f,%.03f,%.03f],\n", rg->x1, rg->y1, rg->r1); 
    indent(job, sp->Level);
    write_stops (job, rg->n_stops, rg->stops, sp);
}

static void write_linear_grad (GVJ_t * job, xdot_linear_grad* lg, state_t* sp)
{
    indent(job, sp->Level);
    gvprintf(job, "\"p0\": [%.03f,%.03f],\n", lg->x0, lg->y0); 
    indent(job, sp->Level);
    gvprintf(job, "\"p1\": [%.03f,%.03f],\n", lg->x1, lg->y1); 
    indent(job, sp->Level);
    write_stops (job, lg->n_stops, lg->stops, sp);
}

static void write_xdot (xdot_op * op, GVJ_t * job, state_t* sp)
{
    indent(job, sp->Level++);
    gvputs(job, "{\n");
    indent(job, sp->Level);

    switch (op->kind) {
    case xd_filled_ellipse :
    case xd_unfilled_ellipse :
	gvprintf(job, "\"op\": \"%c\",\n", op->kind == xd_filled_ellipse ? 'E' : 'e');
 	indent(job, sp->Level);
	gvprintf(job, "\"rect\": [%.03f,%.03f,%.03f,%.03f]\n", 
	    op->u.ellipse.x, op->u.ellipse.y, op->u.ellipse.w, op->u.ellipse.h);
	break;
    case xd_filled_polygon :
    case xd_unfilled_polygon :
	gvprintf(job, "\"op\": \"%c\",\n", op->kind == xd_filled_polygon ? 'P' : 'p');
 	indent(job, sp->Level);
	write_polyline (job, &op->u.polygon);
	break;
    case xd_filled_bezier :
    case xd_unfilled_bezier :
	gvprintf(job, "\"op\": \"%c\",\n", op->kind == xd_filled_bezier ? 'B' : 'b');
 	indent(job, sp->Level);
	write_polyline (job, &op->u.bezier);
	break;
    case xd_polyline :
	gvprintf(job, "\"op\": \"L\",\n"); 
 	indent(job, sp->Level);
	write_polyline (job, &op->u.polyline);
	break;
    case xd_text :
	gvprintf(job, "\"op\": \"T\",\n"); 
 	indent(job, sp->Level);
	gvprintf(job, "\"pt\": [%.03f,%.03f],\n", op->u.text.x, op->u.text.y); 
 	indent(job, sp->Level);
	gvprintf(job, "\"align\": \"%c\",\n",
	    op->u.text.align == xd_left? 'l' :
	    (op->u.text.align == xd_center ? 'c' : 'r'));
 	indent(job, sp->Level);
	gvprintf(job, "\"width\": %.03f,\n", op->u.text.width); 
 	indent(job, sp->Level);
	gvputs(job, "\"text\": ");
	stoj(op->u.text.text, sp, job);
	gvputc(job, '\n');
	break;
    case xd_fill_color :
    case xd_pen_color :
	gvprintf(job, "\"op\": \"%c\",\n", op->kind == xd_fill_color ? 'C' : 'c');
 	indent(job, sp->Level);
	gvprintf(job, "\"grad\": \"none\",\n"); 
 	indent(job, sp->Level);
	gvputs(job, "\"color\": ");
	stoj(op->u.color, sp, job);
	gvputc(job, '\n');
	break;
    case xd_grad_pen_color :
    case xd_grad_fill_color :
	gvprintf(job, "\"op\": \"%c\",\n", op->kind == xd_grad_fill_color ? 'C' : 'c');
 	indent(job, sp->Level);
	if (op->u.grad_color.type == xd_none) {
	    gvprintf(job, "\"grad\": \"none\",\n"); 
 	    indent(job, sp->Level);
	    gvputs(job, "\"color\": ");
	    stoj(op->u.grad_color.u.clr, sp, job);
	    gvputc(job, '\n');
	}
	else {
	    if (op->u.grad_color.type == xd_linear) {
		gvprintf(job, "\"grad\": \"linear\",\n"); 
		indent(job, sp->Level);
		write_linear_grad (job, &op->u.grad_color.u.ling, sp);
	    }
	    else {
		gvprintf(job, "\"grad\": \"radial\",\n"); 
		indent(job, sp->Level);
		write_radial_grad (job, &op->u.grad_color.u.ring, sp);
	    }
	}
	break;
    case xd_font :
	gvprintf(job, "\"op\": \"F\",\n");
 	indent(job, sp->Level);
	gvprintf(job, "\"size\": %.03f,\n", op->u.font.size); 
 	indent(job, sp->Level);
	gvputs(job, "\"face\": ");
	stoj(op->u.font.name, sp, job);
	gvputc(job, '\n');
	break;
    case xd_style :
	gvprintf(job, "\"op\": \"S\",\n");
 	indent(job, sp->Level);
	gvputs(job, "\"style\": ");
	stoj(op->u.style, sp, job);
	gvputc(job, '\n');
	break;
    case xd_image :
	break;
    case xd_fontchar :
	gvprintf(job, "\"op\": \"t\",\n");
 	indent(job, sp->Level);
	gvprintf(job, "\"fontchar\": %d\n", op->u.fontchar);
	break;
    default:
	UNREACHABLE();
    }
    sp->Level--;
    indent(job, sp->Level);
    gvputs(job, "}");
}

static void write_xdots (char * val, GVJ_t * job, state_t* sp)
{
    xdot* cmds;

    if (!val || *val == '\0') return;

    cmds = parseXDot(val);
    if (!cmds) {
	agerr(AGWARN, "Could not parse xdot \"%s\"\n", val);
	return;
    }

    gvputs(job, "\n");
    indent(job, sp->Level++);
    gvputs(job, "[\n");
    for (size_t i = 0; i < cmds->cnt; i++) {
	if (i > 0)
	    gvputs(job, ",\n");
	write_xdot (cmds->ops+i, job, sp);
    }
    sp->Level--;
    gvputs(job, "\n");
    indent(job, sp->Level);
    gvputs(job, "]");
    freeXDot(cmds);
}

static bool isXDot(const char* name) {
  return streq(name, "_draw_") || streq(name, "_ldraw_") ||
         streq(name, "_hdraw_") || streq(name, "_tdraw_") ||
         streq(name, "_hldraw_") || streq(name, "_tldraw_");
}

static void write_attrs(Agobj_t * obj, GVJ_t * job, state_t* sp)
{
    Agraph_t* g = agroot(obj);
    int type = AGTYPE(obj);
    char* attrval;
    Agsym_t* sym = agnxtattr(g, type, NULL);
    if (!sym) return;

    for (; sym; sym = agnxtattr(g, type, sym)) {
	if (!(attrval = agxget(obj, sym))) continue;
	if (*attrval == '\0' && !streq(sym->name, "label")) continue;
	gvputs(job, ",\n");
	indent(job, sp->Level);
	stoj(sym->name, sp, job);
	gvputs(job, ": ");
	if (sp->doXDot && isXDot(sym->name))
	    write_xdots(agxget(obj, sym), job, sp);
	else
	    stoj(agxget(obj, sym), sp, job);
    }
}

static void write_hdr(Agraph_t *g, GVJ_t *job, bool top, state_t *sp) {
    char *name;

    name = agnameof(g);
    indent(job, sp->Level);
    gvputs(job, "\"name\": ");
    stoj(name, sp, job);

    if (top) {
	gvputs(job, ",\n");
	indent(job, sp->Level);
	gvprintf(job, "\"directed\": %s,\n", agisdirected(g)?"true":"false");
	indent(job, sp->Level);
	gvprintf(job, "\"strict\": %s", agisstrict(g)?"true":"false");
    }
}

static void write_graph(Agraph_t *g, GVJ_t *job, bool top, state_t *sp);

static void write_subg(Agraph_t * g, GVJ_t * job, state_t* sp)
{
    Agraph_t* sg;

    write_graph (g, job, false, sp);
    for (sg = agfstsubg(g); sg; sg = agnxtsubg(sg)) {
	gvputs(job, ",\n");
	write_subg(sg, job, sp);
    }
}

/// write out subgraphs
///
/// \return True if the given graph has any subgraphs
static bool write_subgs(Agraph_t *g, GVJ_t *job, bool top, state_t *sp) {
    Agraph_t* sg;

    sg = agfstsubg(g);
    if (!sg) return false;
   
    gvputs(job, ",\n");
    indent(job, sp->Level++);
    if (top)
	gvputs(job, "\"objects\": [\n");
    else {
	gvputs(job, "\"subgraphs\": [\n");
	indent(job, sp->Level);
    }
    const char *separator = "";
    for (; sg; sg = agnxtsubg(sg)) {
	gvputs(job, separator);
        if (top)
	    write_subg (sg, job, sp);
	else
	    gvprintf(job, "%d", GD_gid(sg));
	separator = ",\n";
    }
    if (!top) {
	sp->Level--;
	gvputs(job, "\n");
	indent(job, sp->Level);
	gvputs(job, "]");
    }

    return true;
}

static int agseqasc(Agedge_t **lhs, Agedge_t **rhs)
{
    Agedge_t *e1 = *lhs;
    Agedge_t *e2 = *rhs;

    if (AGSEQ(e1) < AGSEQ(e2)) {
        return -1;
    }
    else if (AGSEQ(e1) > AGSEQ(e2)) {
        return 1;
    }
    else {
        return 0;
    }
}

static void write_edge(Agedge_t *e, GVJ_t *job, bool top, state_t *sp) {
    if (top) {
	indent(job, sp->Level++);
	gvputs(job, "{\n");
	indent(job, sp->Level);
	gvprintf(job, "\"_gvid\": %d,\n", ED_gid(e));
	indent(job, sp->Level);
	gvprintf(job, "\"tail\": %d,\n", ND_gid(agtail(e)));
	indent(job, sp->Level);
	gvprintf(job, "\"head\": %d", ND_gid(aghead(e)));
    	write_attrs((Agobj_t*)e, job, sp);
	gvputs(job, "\n");
	sp->Level--;
	indent(job, sp->Level);
	gvputs(job, "}");
    }
    else {
	gvprintf(job, "%d", ED_gid(e));
    }
}

static int write_edges(Agraph_t *g, GVJ_t *job, bool top, state_t *sp) {
    size_t count = 0;

    for (Agnode_t *np = agfstnode(g); np; np = agnxtnode(g, np)) {
        for (Agedge_t *ep = agfstout(g, np); ep; ep = agnxtout(g, ep)) {
            ++count;
        }
    }

    if (count == 0) {
        return 0;
    }

    Agedge_t **edges = gv_calloc(count, sizeof(Agedge_t *));

    size_t i = 0;
    for (Agnode_t *np = agfstnode(g); np; np = agnxtnode(g, np)) {
        for (Agedge_t *ep = agfstout(g, np); ep; ep = agnxtout(g, ep)) {
            edges[i] = ep;
            ++i;
        }
    }

    qsort(edges, count, sizeof(Agedge_t *), (qsort_cmpf)agseqasc);

    gvputs(job, ",\n");
    indent(job, sp->Level++);
    gvputs(job, "\"edges\": [\n");
    if (!top)
        indent(job, sp->Level);
    for (size_t j = 0; j < count; ++j) {
        if (j > 0) {
            if (top)
                gvputs(job, ",\n");
            else
                gvputs(job, ",");
        }
        write_edge(edges[j], job, top, sp);
    }

    free(edges);

    sp->Level--;
    gvputs(job, "\n");
    indent(job, sp->Level);
    gvputs(job, "]");
    return 1;
}

static void write_node(Agnode_t *n, GVJ_t *job, bool top, state_t *sp) {
    if (top) {
	indent(job, sp->Level++);
	gvputs(job, "{\n");
	indent(job, sp->Level);
	gvprintf(job, "\"_gvid\": %d,\n", ND_gid(n));
	indent(job, sp->Level);
	gvputs(job, "\"name\": ");
	stoj(agnameof(n), sp, job);
    	write_attrs((Agobj_t*)n, job, sp);
	gvputs(job, "\n");
	sp->Level--;
	indent(job, sp->Level);
	gvputs(job, "}");
    }
    else {
	gvprintf(job, "%d", ND_gid(n));
    }
}

static int write_nodes(Agraph_t *g, GVJ_t *job, bool top, bool has_subgs, state_t *sp) {

    // is every subcomponent of this graph a cluster?
    bool only_clusters = true;
    for (Agnode_t *n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (!IS_CLUST_NODE(n)) {
	    only_clusters = false;
	    break;
	}
    }

    if (only_clusters) {
	if (has_subgs && top) {
	    sp->Level--;
	    gvputs(job, "\n");
	    indent(job, sp->Level);
	    gvputs(job, "]");
	}
	return 0;
    }
    gvputs(job, ",\n");
    if (top) {
	if (!has_subgs) {
            indent(job, sp->Level++);
            gvputs(job, "\"objects\": [\n");
        }
    }
    else {
        indent(job, sp->Level++);
	gvputs(job, "\"nodes\": [\n");
	indent(job, sp->Level);
    }
    const char *separator = "";
    for (Agnode_t *n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (IS_CLUST_NODE(n)) continue;
	gvputs(job, separator);
	write_node (n, job, top, sp);
	separator = top ? ",\n" : ",";
    }
    sp->Level--;
    gvputs(job, "\n");
    indent(job, sp->Level);
    gvputs(job, "]");
    return 1;
}

typedef struct {
    Dtlink_t link;
    char* id;
    int v;
} intm;

static void freef(intm *obj, Dtdisc_t *disc) {
    (void)disc;
    free(obj->id);
    free(obj);
}

static Dtdisc_t intDisc = {
    offsetof(intm, id),
    -1,
    offsetof(intm, link),
    (Dtmake_f) NULL,
    (Dtfree_f) freef,
    (Dtcompar_f) NULL,
    0,
};

static int lookup (Dt_t* map, char* name)
{
    intm* ip = dtmatch(map, name);
    if (ip) return ip->v;
    else return -1;
}
 
static void insert (Dt_t* map, char* name, int v)
{
    intm* ip = dtmatch(map, name);

    if (ip) {
	if (ip->v != v)
	    agerr(AGWARN, "Duplicate cluster name \"%s\"\n", name);
	return;
    }
    ip = gv_alloc(sizeof(intm));
    ip->id = gv_strdup(name);
    ip->v = v;
    dtinsert (map, ip);
}

static int label_subgs(Agraph_t* g, int lbl, Dt_t* map)
{
    Agraph_t* sg;

    if (g != agroot(g)) {
	GD_gid(g) = lbl++;
	if (IS_CLUSTER(g))
	    insert (map, agnameof(g), GD_gid(g)); 
    }
    for (sg = agfstsubg(g); sg; sg = agnxtsubg(sg)) {
	lbl = label_subgs(sg, lbl, map);
    }
    return lbl;
}


static void write_graph(Agraph_t *g, GVJ_t *job, bool top, state_t *sp) {
    Agnode_t* np; 
    Agedge_t* ep; 
    int ncnt = 0;
    int ecnt = 0;
    int sgcnt = 0;
    Dt_t* map;

    if (top) {
	map = dtopen (&intDisc, Dtoset);
	aginit(g, AGNODE, ID, sizeof(gvid_t), false);
	aginit(g, AGEDGE, ID, sizeof(gvid_t), false);
	aginit(g, AGRAPH, ID, -((int)sizeof(gvid_t)), false);
	sgcnt = label_subgs(g, sgcnt, map);
	for (np = agfstnode(g); np; np = agnxtnode(g,np)) {
	    if (IS_CLUST_NODE(np)) {
		ND_gid(np) = lookup(map, agnameof(np));
	    }
	    else {
		ND_gid(np) = sgcnt + ncnt++;
	    }
	    for (ep = agfstout(g, np); ep; ep = agnxtout(g,ep)) {
		ED_gid(ep) = ecnt++;
	    }
	}
	dtclose(map);
    }

    indent(job, sp->Level++);
    gvputs(job, "{\n");
    write_hdr(g, job, top, sp);
    write_attrs((Agobj_t*)g, job, sp);
    if (top) {
	gvputs(job, ",\n");
	indent(job, sp->Level);
	gvprintf(job, "\"_subgraph_cnt\": %d", sgcnt);
    } else {
	gvputs(job, ",\n");
	indent(job, sp->Level);
	gvprintf(job, "\"_gvid\": %d", GD_gid(g));
    }
    bool has_subgs = write_subgs(g, job, top, sp);
    write_nodes (g, job, top, has_subgs, sp);
    write_edges (g, job, top, sp);
    gvputs(job, "\n");
    sp->Level--;
    indent(job, sp->Level);
    if (top)
	gvputs(job, "}\n");
    else
	gvputs(job, "}");
}

typedef int (*putstrfn) (void *chan, const char *str);
typedef int (*flushfn) (void *chan);

static void json_end_graph(GVJ_t *job)
{
    graph_t *g = job->obj->u.g;
    state_t sp;
    static Agiodisc_t io;

    if (io.afread == NULL) {
	io.afread = AgIoDisc.afread;
	io.putstr = (putstrfn)gvputs;
	io.flush = (flushfn)gvflush;
    }

    g->clos->disc.io = &io;

    set_attrwf(g, true, false);
    sp.Level = 0;
    sp.isLatin = GD_charset(g) == CHAR_LATIN1;
    sp.doXDot = job->render.id == FORMAT_JSON || job->render.id == FORMAT_XDOT_JSON;
    write_graph(g, job, true, &sp);
}

gvrender_engine_t json_engine = {
    0,				/* json_begin_job */
    0,				/* json_end_job */
    json_begin_graph,
    json_end_graph,
    0,				/* json_begin_layer */
    0,				/* json_end_layer */
    0,				/* json_begin_page */
    0,				/* json_end_page */
    0,				/* json_begin_cluster */
    0,				/* json_end_cluster */
    0,				/* json_begin_nodes */
    0,				/* json_end_nodes */
    0,				/* json_begin_edges */
    0,				/* json_end_edges */
    0,				/* json_begin_node */
    0,				/* json_end_node */
    0,				/* json_begin_edge */
    0,				/* json_end_edge */
    0,				/* json_begin_anchor */
    0,				/* json_end_anchor */
    0,				/* json_begin_label */
    0,				/* json_end_label */
    0,				/* json_textspan */
    0,				/* json_resolve_color */
    0,				/* json_ellipse */
    0,				/* json_polygon */
    0,				/* json_bezier */
    0,				/* json_polyline */
    0,				/* json_comment */
    0,				/* json_library_shape */
};

gvrender_features_t render_features_json1 = {
    GVRENDER_DOES_TRANSFORM,	/* not really - uses raw graph coords */  /* flags */
    0.,                         /* default pad - graph units */
    NULL,			/* knowncolors */
    0,				/* sizeof knowncolors */
    COLOR_STRING,		/* color_type */
};

gvrender_features_t render_features_json = {
    GVRENDER_DOES_TRANSFORM	/* not really - uses raw graph coords */
	| GVRENDER_DOES_MAPS
	| GVRENDER_DOES_TARGETS
	| GVRENDER_DOES_TOOLTIPS, /* flags */
    0.,                         /* default pad - graph units */
    NULL,			/* knowncolors */
    0,				/* sizeof knowncolors */
    COLOR_STRING,		/* color_type */
};

gvdevice_features_t device_features_json_nop = {
    LAYOUT_NOT_REQUIRED,	/* flags */
    {0.,0.},			/* default margin - points */
    {0.,0.},			/* default page width, height - points */
    {72.,72.},			/* default dpi */
};

gvdevice_features_t device_features_json = {
    0,				/* flags */
    {0.,0.},			/* default margin - points */
    {0.,0.},			/* default page width, height - points */
    {72.,72.},			/* default dpi */
};

gvplugin_installed_t gvrender_json_types[] = {
    {FORMAT_JSON, "json", 1, &json_engine, &render_features_json},
    {FORMAT_JSON0, "json0", 1, &json_engine, &render_features_json},
    {FORMAT_DOT_JSON, "dot_json", 1, &json_engine, &render_features_json},
    {FORMAT_XDOT_JSON, "xdot_json", 1, &json_engine, &render_features_json},
    {0, NULL, 0, NULL, NULL}
};

gvplugin_installed_t gvdevice_json_types[] = {
    {FORMAT_JSON, "json:json", 1, NULL, &device_features_json},
    {FORMAT_JSON0, "json0:json", 1, NULL, &device_features_json},
    {FORMAT_DOT_JSON, "dot_json:json", 1, NULL, &device_features_json_nop},
    {FORMAT_XDOT_JSON, "xdot_json:json", 1, NULL, &device_features_json_nop},
    {0, NULL, 0, NULL, NULL}
};
