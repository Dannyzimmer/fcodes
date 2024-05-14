/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
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

#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <common/macros.h>
#include <common/const.h>

#include <gvc/gvplugin_render.h>
#include <gvc/gvplugin_device.h>
#include <cgraph/alloc.h>
#include <cgraph/agxbuf.h>
#include <cgraph/prisize_t.h>
#include <cgraph/unreachable.h>
#include <common/utils.h>
#include <gvc/gvc.h>
#include <gvc/gvio.h>

typedef enum {
	FORMAT_DOT,
	FORMAT_CANON,
	FORMAT_PLAIN,
	FORMAT_PLAIN_EXT,
	FORMAT_XDOT,
	FORMAT_XDOT12,
	FORMAT_XDOT14,
} format_type;

#define XDOTVERSION "1.7"

#define NUMXBUFS (EMIT_HLABEL+1)
/* There are as many xbufs as there are values of emit_state_t.
 * However, only the first NUMXBUFS are distinct. Nodes, clusters, and
 * edges are drawn atomically, so they share the DRAW and LABEL buffers
 */
static agxbuf xbuf[NUMXBUFS];
static agxbuf* xbufs[] = {
    xbuf+EMIT_GDRAW, xbuf+EMIT_CDRAW, xbuf+EMIT_TDRAW, xbuf+EMIT_HDRAW, 
    xbuf+EMIT_GLABEL, xbuf+EMIT_CLABEL, xbuf+EMIT_TLABEL, xbuf+EMIT_HLABEL, 
    xbuf+EMIT_CDRAW, xbuf+EMIT_CDRAW, xbuf+EMIT_CLABEL, xbuf+EMIT_CLABEL, 
};
static double penwidth [] = {
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1,
};
static unsigned int textflags[EMIT_ELABEL+1];

typedef struct {
    attrsym_t *g_draw;
    attrsym_t *g_l_draw;
    attrsym_t *n_draw;
    attrsym_t *n_l_draw;
    attrsym_t *e_draw;
    attrsym_t *h_draw;
    attrsym_t *t_draw;
    attrsym_t *e_l_draw;
    attrsym_t *hl_draw;
    attrsym_t *tl_draw;
    char buf[NUMXBUFS][BUFSIZ];
    unsigned short version;
    char* version_s;
} xdot_state_t;
static xdot_state_t* xd;

static void xdot_str_xbuf (agxbuf* xb, char* pfx, const char* s)
{
    agxbprint (xb, "%s%" PRISIZE_T " -%s ", pfx, strlen(s), s);
}

static void xdot_str (GVJ_t *job, char* pfx, const char* s)
{   
    emit_state_t emit_state = job->obj->emit_state;
    xdot_str_xbuf (xbufs[emit_state], pfx, s);
}

/* xdot_fmt_num:
 * Convert double to string with space at end.
 * Trailing zeros are removed and decimal point, if possible.
 */
static void xdot_fmt_num(agxbuf *buf, double v) {
  agxbprint(buf, "%.02f", v);
  agxbuf_trim_zeros(buf);
  agxbputc(buf, ' ');
}

static void xdot_point(agxbuf *xb, pointf p)
{
  xdot_fmt_num(xb, p.x);
  xdot_fmt_num(xb, yDir(p.y));
}

static void xdot_num(agxbuf *xb, double v)
{
  xdot_fmt_num(xb, v);
}

static void xdot_points(GVJ_t *job, char c, pointf * A, int n)
{
    emit_state_t emit_state = job->obj->emit_state;
    int i;

    agxbprint(xbufs[emit_state], "%c %d ", c, n);
    for (i = 0; i < n; i++)
        xdot_point(xbufs[emit_state], A[i]);
}

static char*
color2str (unsigned char rgba[4])
{
    static char buf [10];

    if (rgba[3] == 0xFF)
	snprintf(buf, sizeof(buf), "#%02x%02x%02x", rgba[0], rgba[1],  rgba[2]);
    else
	snprintf(buf, sizeof(buf), "#%02x%02x%02x%02x", rgba[0], rgba[1],  rgba[2],
	         rgba[3]);
    return buf;
}

static void xdot_pencolor (GVJ_t *job)
{
    xdot_str (job, "c ", color2str (job->obj->pencolor.u.rgba));
}

static void xdot_fillcolor (GVJ_t *job)
{
    xdot_str (job, "C ", color2str (job->obj->fillcolor.u.rgba));
}

static void xdot_style (GVJ_t *job)
{
    agxbuf xb = {0};
    char* p, **s;

    /* First, check if penwidth state is correct */
    if (job->obj->penwidth != penwidth[job->obj->emit_state]) {
	penwidth[job->obj->emit_state] = job->obj->penwidth;
	agxbput (&xb, "setlinewidth(");
	agxbprint(&xb, "%.3f", job->obj->penwidth);
	agxbuf_trim_zeros(&xb);
	agxbputc(&xb, ')');
        xdot_str (job, "S ", agxbuse(&xb));
    }

    /* now process raw style, if any */
    s = job->obj->rawstyle;
    if (!s) {
	agxbfree(&xb);
	return;
    }

    while ((p = *s++)) {
	if (streq(p, "filled") || streq(p, "bold") || streq(p, "setlinewidth")) continue;
        agxbput(&xb, p);
        while (*p)
            p++;
        p++;
        if (*p) {  /* arguments */
            agxbputc(&xb, '(');
            for (const char *separator = ""; *p; separator = ",") {
                agxbprint(&xb, "%s%s", separator, p);
                while (*p) p++;
                p++;
            }
            agxbputc(&xb, ')');
        }
        xdot_str (job, "S ", agxbuse(&xb));
    }

    agxbfree(&xb);

}

/** set the value of a symbol within a node, escaping backslashes
 *
 * The back ends that output certain text-based formats, e.g. xdot, assume that
 * all characters that have special meaning within a string have already been
 * escaped, with the exception of double quote ("). Hence, any string being
 * constructed from user-provided input needs to be escaped for other
 * problematic characters (namely \) beforehand. This is a little utility
 * function to do such.
 *
 * @param n Node to operate on
 * @param sym Symbol to set
 * @param value Unescaped string
 */
static void put_escaping_backslashes(Agobj_t* n, Agsym_t *sym, const char *value)
{
    agxbuf buf = {0};

    /* print the given string to the buffer, escaping as we go */
    for (; *value != '\0'; ++value) {
        if (*value == '\\') {
            agxbputc(&buf, '\\');
        }
        agxbputc(&buf, *value);
    }

    /* update the node's symbol to the escaped text */
    agxset(n, sym, agxbuse(&buf));

    /* discard the buffer */
    agxbfree(&buf);
}

static void xdot_end_node(GVJ_t* job)
{
    Agnode_t* n = job->obj->u.n; 
    if (agxblen(xbufs[EMIT_NDRAW]))
	agxset(n, xd->n_draw, agxbuse(xbufs[EMIT_NDRAW]));
    if (agxblen(xbufs[EMIT_NLABEL]))
	put_escaping_backslashes(&n->base, xd->n_l_draw, agxbuse(xbufs[EMIT_NLABEL]));
    penwidth[EMIT_NDRAW] = 1;
    penwidth[EMIT_NLABEL] = 1;
    textflags[EMIT_NDRAW] = 0;
    textflags[EMIT_NLABEL] = 0;
}

static void xdot_end_edge(GVJ_t* job)
{
    Agedge_t* e = job->obj->u.e; 

    if (agxblen(xbufs[EMIT_EDRAW]))
	agxset(e, xd->e_draw, agxbuse(xbufs[EMIT_EDRAW]));
    if (agxblen(xbufs[EMIT_TDRAW]))
	agxset(e, xd->t_draw, agxbuse(xbufs[EMIT_TDRAW]));
    if (agxblen(xbufs[EMIT_HDRAW]))
	agxset(e, xd->h_draw, agxbuse(xbufs[EMIT_HDRAW]));
    if (agxblen(xbufs[EMIT_ELABEL]))
	put_escaping_backslashes(&e->base, xd->e_l_draw, agxbuse(xbufs[EMIT_ELABEL]));
    if (agxblen(xbufs[EMIT_TLABEL]))
	agxset(e, xd->tl_draw, agxbuse(xbufs[EMIT_TLABEL]));
    if (agxblen(xbufs[EMIT_HLABEL]))
	agxset(e, xd->hl_draw, agxbuse(xbufs[EMIT_HLABEL]));
    penwidth[EMIT_EDRAW] = 1;
    penwidth[EMIT_ELABEL] = 1;
    penwidth[EMIT_TDRAW] = 1;
    penwidth[EMIT_HDRAW] = 1;
    penwidth[EMIT_TLABEL] = 1;
    penwidth[EMIT_HLABEL] = 1;
    textflags[EMIT_EDRAW] = 0;
    textflags[EMIT_ELABEL] = 0;
    textflags[EMIT_TDRAW] = 0;
    textflags[EMIT_HDRAW] = 0;
    textflags[EMIT_TLABEL] = 0;
    textflags[EMIT_HLABEL] = 0;
}

static void xdot_end_cluster(GVJ_t * job)
{
    Agraph_t* cluster_g = job->obj->u.sg;

    agxset(cluster_g, xd->g_draw, agxbuse(xbufs[EMIT_CDRAW]));
    if (GD_label(cluster_g))
	agxset(cluster_g, xd->g_l_draw, agxbuse(xbufs[EMIT_CLABEL]));
    penwidth[EMIT_CDRAW] = 1;
    penwidth[EMIT_CLABEL] = 1;
    textflags[EMIT_CDRAW] = 0;
    textflags[EMIT_CLABEL] = 0;
}

static unsigned short versionStr2Version(const char *str) {
  unsigned short us = 0;
  for (size_t i = 0; str[i] != '\0'; ++i) {
    if (!isdigit((int)str[i])) {
      continue;
    }
    unsigned short digit = (unsigned short)(str[i] - '0');
    if (us > (USHRT_MAX - digit) / 10) {
      agerr(AGWARN, "xdot version \"%s\" too long", str);
      break;
    }
    us = (unsigned short)(us * 10 + digit);
  }
  return us;
}

/* 
 * John M. suggests:
 * You might want to add four more:
 *
 * _ohdraw_ (optional head-end arrow for edges)
 * _ohldraw_ (optional head-end label for edges)
 * _otdraw_ (optional tail-end arrow for edges)
 * _otldraw_ (optional tail-end label for edges)
 * 
 * that would be generated when an additional option is supplied to 
 * dot, etc. and 
 * these would be the arrow/label positions to use if a user want to flip the 
 * direction of an edge (as sometimes is there want).
 * 
 * N.B. John M. asks:
 *   By the way, I don't know if you ever plan to add other letters for 
 * the xdot spec, but could you reserve "a" and also "A" (for  attribute), 
 * "n" and also "N" (for numeric), "w" (for sWitch),  "s" (for string) 
 * and "t" (for tooltip) and "x" (for position). We use  those letters in 
 * our drawing spec (and also "<" and ">"), so if you  start generating 
 * output with them, it could break what we have. 
 */
static void
xdot_begin_graph (graph_t *g, int s_arrows, int e_arrows, format_type id)
{
    int i;
    unsigned short us;
    char* s;

    xd = gv_alloc(sizeof(xdot_state_t));

    if (id == FORMAT_XDOT14) {
	xd->version = 14;
	xd->version_s = "1.4";
    }
    else if (id == FORMAT_XDOT12) {
	xd->version = 12;
	xd->version_s = "1.2";
    }
    else if ((s = agget(g, "xdotversion")) && s[0] && ((us = versionStr2Version(s)) > 10)) {
	xd->version = us;
	xd->version_s = s;
    }
    else {
	xd->version = versionStr2Version(XDOTVERSION);
	xd->version_s = XDOTVERSION;
    }

    if (GD_n_cluster(g))
	xd->g_draw = safe_dcl(g, AGRAPH, "_draw_", "");
    else
	xd->g_draw = NULL;
    if (GD_has_labels(g) & GRAPH_LABEL)
	xd->g_l_draw = safe_dcl(g, AGRAPH, "_ldraw_", "");
    else
	xd->g_l_draw = NULL;

    xd->n_draw = safe_dcl(g, AGNODE, "_draw_", "");
    xd->n_l_draw = safe_dcl(g, AGNODE, "_ldraw_", "");

    xd->e_draw = safe_dcl(g, AGEDGE, "_draw_", "");
    if (e_arrows)
	xd->h_draw = safe_dcl(g, AGEDGE, "_hdraw_", "");
    else
	xd->h_draw = NULL;
    if (s_arrows)
	xd->t_draw = safe_dcl(g, AGEDGE, "_tdraw_", "");
    else
	xd->t_draw = NULL;
    if (GD_has_labels(g) & (EDGE_LABEL|EDGE_XLABEL))
	xd->e_l_draw = safe_dcl(g, AGEDGE, "_ldraw_", "");
    else
	xd->e_l_draw = NULL;
    if (GD_has_labels(g) & HEAD_LABEL)
	xd->hl_draw = safe_dcl(g, AGEDGE, "_hldraw_", "");
    else
	xd->hl_draw = NULL;
    if (GD_has_labels(g) & TAIL_LABEL)
	xd->tl_draw = safe_dcl(g, AGEDGE, "_tldraw_", "");
    else
	xd->tl_draw = NULL;

    for (i = 0; i < NUMXBUFS; i++)
	agxbinit(xbuf+i, BUFSIZ, xd->buf[i]);
}

static void dot_begin_graph(GVJ_t *job)
{
    int e_arrows;            /* graph has edges with end arrows */
    int s_arrows;            /* graph has edges with start arrows */
    graph_t *g = job->obj->u.g;

    switch (job->render.id) {
	case FORMAT_DOT:
	    attach_attrs(g);
	    break;
	case FORMAT_CANON:
	    if (HAS_CLUST_EDGE(g))
		undoClusterEdges(g);
	    break;
	case FORMAT_PLAIN:
	case FORMAT_PLAIN_EXT:
	    break;
	case FORMAT_XDOT:
	case FORMAT_XDOT12:
	case FORMAT_XDOT14:
	    attach_attrs_and_arrows(g, &s_arrows, &e_arrows);
	    xdot_begin_graph(g, s_arrows, e_arrows, job->render.id);
	    break;
	default:
	    UNREACHABLE();
    }
}

static void xdot_end_graph(graph_t* g)
{
    int i;

    if (agxblen(xbufs[EMIT_GDRAW])) {
	if (!xd->g_draw)
	    xd->g_draw = safe_dcl(g, AGRAPH, "_draw_", "");
	agxset(g, xd->g_draw, agxbuse(xbufs[EMIT_GDRAW]));
    }
    if (GD_label(g))
	put_escaping_backslashes(&g->base, xd->g_l_draw, agxbuse(xbufs[EMIT_GLABEL]));
    agsafeset (g, "xdotversion", xd->version_s, "");

    for (i = 0; i < NUMXBUFS; i++)
	agxbfree(xbuf+i);
    free (xd);
    penwidth[EMIT_GDRAW] = 1;
    penwidth[EMIT_GLABEL] = 1;
    textflags[EMIT_GDRAW] = 0;
    textflags[EMIT_GLABEL] = 0;
}

typedef int (*putstrfn) (void *chan, const char *str);
typedef int (*flushfn) (void *chan);
static void dot_end_graph(GVJ_t *job)
{
    graph_t *g = job->obj->u.g;
    Agiodisc_t* io_save;
    static Agiodisc_t io;

    if (io.afread == NULL) {
	io.afread = AgIoDisc.afread;
	io.putstr = (putstrfn)gvputs;
	io.flush = (flushfn)gvflush;
    }

    io_save = g->clos->disc.io;
    g->clos->disc.io = &io;
    switch (job->render.id) {
	case FORMAT_PLAIN:
	    write_plain(job, g, (FILE*)job, false);
	    break;
	case FORMAT_PLAIN_EXT:
	    write_plain(job, g, (FILE*)job, true);
	    break;
	case FORMAT_DOT:
	case FORMAT_CANON:
	    if (!(job->flags & OUTPUT_NOT_REQUIRED))
		agwrite(g, job);
	    break;
	case FORMAT_XDOT:
	case FORMAT_XDOT12:
	case FORMAT_XDOT14:
	    xdot_end_graph(g);
	    if (!(job->flags & OUTPUT_NOT_REQUIRED))
		agwrite(g, job);
	    break;
	default:
	    UNREACHABLE();
    }
    g->clos->disc.io = io_save;
}

static const unsigned int flag_masks[] = {0x1F, 0x3F, 0x7F};

static void xdot_textspan(GVJ_t * job, pointf p, textspan_t * span)
{
    emit_state_t emit_state = job->obj->emit_state;
    unsigned flags;
    int j;
    
    agxbput(xbufs[emit_state], "F ");
    xdot_fmt_num(xbufs[emit_state], span->font->size);
    xdot_str (job, "", span->font->name);
    xdot_pencolor(job);

    switch (span->just) {
    case 'l':
        j = -1; 
        break;
    case 'r':
        j = 1;
        break;
    default:
    case 'n':
        j = 0;
        break;
    }
    if (span->font)
	flags = span->font->flags;
    else
	flags = 0;
    const size_t flag_masks_size = sizeof(flag_masks) / sizeof(flag_masks[0]);
    if (xd->version >= 15 && (size_t)xd->version - 15 < flag_masks_size) {
	unsigned int mask = flag_masks[xd->version-15];
	unsigned int bits = flags & mask;
	if (textflags[emit_state] != bits) {
	    agxbprint(xbufs[emit_state], "t %u ", bits);
	    textflags[emit_state] = bits;
	}
    }

    p.y += span->yoffset_centerline;
    agxbput(xbufs[emit_state], "T ");
    xdot_point(xbufs[emit_state], p);
    agxbprint(xbufs[emit_state], "%d ", j);
    xdot_fmt_num(xbufs[emit_state], span->size.x);
    xdot_str (job, "", span->str);
}

static void xdot_color_stop (agxbuf* xb, float v, gvcolor_t* clr)
{
  agxbprint(xb, "%.03f", v);
  agxbuf_trim_zeros(xb);
  agxbputc(xb, ' ');
  xdot_str_xbuf(xb, "", color2str (clr->u.rgba));
}

static void xdot_gradient_fillcolor (GVJ_t* job, int filled, pointf* A, int n)
{
    char buf0[BUFSIZ];
    agxbuf xb;
    obj_state_t* obj = job->obj;
    double angle = obj->gradient_angle * M_PI / 180;
    pointf G[2],c1,c2;

    if (xd->version < 14) {
	xdot_fillcolor (job);
	return;
    }

    agxbinit(&xb, BUFSIZ, buf0);
    if (filled == GRADIENT) {
	get_gradient_points(A, G, n, angle, 2);
	agxbputc (&xb, '[');
	xdot_point (&xb, G[0]);
	xdot_point (&xb, G[1]);
    }
    else {
	get_gradient_points(A, G, n, 0, 3);
	  // r2 is outer radius
	double r2 = G[1].y;
	if (obj->gradient_angle == 0) {
	    c1.x = G[0].x;
	    c1.y = G[0].y;
	}
	else {
	    c1.x = G[0].x +  (r2/4) * cos(angle);
	    c1.y = G[0].y +  (r2/4) * sin(angle);
	}
	c2.x = G[0].x;
	c2.y = G[0].y;
	double r1 = r2 / 4;
	agxbputc(&xb, '(');
	xdot_point (&xb, c1);
	xdot_num (&xb, r1);
	xdot_point (&xb, c2);
	xdot_num (&xb, r2);
    }
    
    agxbput(&xb, "2 ");
    if (obj->gradient_frac > 0) {
	xdot_color_stop (&xb, obj->gradient_frac, &obj->fillcolor);
	xdot_color_stop (&xb, obj->gradient_frac, &obj->stopcolor);
    }
    else {
	xdot_color_stop (&xb, 0, &obj->fillcolor);
	xdot_color_stop (&xb, 1, &obj->stopcolor);
    }
    agxbpop(&xb);
    if (filled == GRADIENT)
	agxbputc(&xb, ']');
    else
	agxbputc(&xb, ')');
    xdot_str (job, "C ", agxbuse(&xb));
    agxbfree(&xb);
}

static void xdot_ellipse(GVJ_t * job, pointf * A, int filled)
{
    emit_state_t emit_state = job->obj->emit_state;

    xdot_style (job);
    xdot_pencolor (job);
    if (filled) {
	if ((filled == GRADIENT) || (filled == RGRADIENT)) {
	   xdot_gradient_fillcolor (job, filled, A, 2);
	}
        else 
	    xdot_fillcolor (job);
        agxbput(xbufs[emit_state], "E ");
    }
    else
        agxbput(xbufs[emit_state], "e ");
    xdot_point(xbufs[emit_state], A[0]);
    xdot_fmt_num(xbufs[emit_state], A[1].x - A[0].x);
    xdot_fmt_num(xbufs[emit_state], A[1].y - A[0].y);
}

static void xdot_bezier(GVJ_t *job, pointf *A, int n, int filled) {
    xdot_style (job);
    xdot_pencolor (job);
    if (filled) {
	if ((filled == GRADIENT) || (filled == RGRADIENT)) {
	   xdot_gradient_fillcolor (job, filled, A, n);
	}
        else
	    xdot_fillcolor (job);
        xdot_points(job, 'b', A, n);   /* NB - 'B' & 'b' are reversed in comparison to the other items */
    }
    else
        xdot_points(job, 'B', A, n);
}

static void xdot_polygon(GVJ_t * job, pointf * A, int n, int filled)
{
    xdot_style (job);
    xdot_pencolor (job);
    if (filled) {
	if ((filled == GRADIENT) || (filled == RGRADIENT)) {
	   xdot_gradient_fillcolor (job, filled, A, n);
	}
        else
	    xdot_fillcolor (job);
        xdot_points(job, 'P', A, n);
    }
    else
        xdot_points(job, 'p', A, n);
}

static void xdot_polyline(GVJ_t * job, pointf * A, int n)
{
    xdot_style (job);
    xdot_pencolor (job);
    xdot_points(job, 'L', A, n);
}

void core_loadimage_xdot(GVJ_t * job, usershape_t *us, boxf b, bool filled)
{
    (void)filled;

    emit_state_t emit_state = job->obj->emit_state;
    
    agxbput(xbufs[emit_state], "I ");
    xdot_point(xbufs[emit_state], b.LL);
    xdot_fmt_num(xbufs[emit_state], b.UR.x - b.LL.x);
    xdot_fmt_num(xbufs[emit_state], b.UR.y - b.LL.y);
    xdot_str (job, "", us->name);
}

gvrender_engine_t dot_engine = {
    0,				/* dot_begin_job */
    0,				/* dot_end_job */
    dot_begin_graph,
    dot_end_graph,
    0,				/* dot_begin_layer */
    0,				/* dot_end_layer */
    0,				/* dot_begin_page */
    0,				/* dot_end_page */
    0,				/* dot_begin_cluster */
    0,				/* dot_end_cluster */
    0,				/* dot_begin_nodes */
    0,				/* dot_end_nodes */
    0,				/* dot_begin_edges */
    0,				/* dot_end_edges */
    0,				/* dot_begin_node */
    0,				/* dot_end_node */
    0,				/* dot_begin_edge */
    0,				/* dot_end_edge */
    0,				/* dot_begin_anchor */
    0,				/* dot_end_anchor */
    0,				/* dot_begin_label */
    0,				/* dot_end_label */
    0,				/* dot_textspan */
    0,				/* dot_resolve_color */
    0,				/* dot_ellipse */
    0,				/* dot_polygon */
    0,				/* dot_bezier */
    0,				/* dot_polyline */
    0,				/* dot_comment */
    0,				/* dot_library_shape */
};

gvrender_engine_t xdot_engine = {
    0,				/* xdot_begin_job */
    0,				/* xdot_end_job */
    dot_begin_graph,
    dot_end_graph,
    0,				/* xdot_begin_layer */
    0,				/* xdot_end_layer */
    0,				/* xdot_begin_page */
    0,				/* xdot_end_page */
    0,				/* xdot_begin_cluster */
    xdot_end_cluster,
    0,				/* xdot_begin_nodes */
    0,				/* xdot_end_nodes */
    0,				/* xdot_begin_edges */
    0,				/* xdot_end_edges */
    0,				/* xdot_begin_node */
    xdot_end_node,
    0,				/* xdot_begin_edge */
    xdot_end_edge,
    0,                          /* xdot_begin_anchor */
    0,                          /* xdot_end_anchor */
    0,				/* xdot_begin_label */
    0,				/* xdot_end_label */
    xdot_textspan,
    0,				/* xdot_resolve_color */
    xdot_ellipse,
    xdot_polygon,
    xdot_bezier,
    xdot_polyline,
    0,				/* xdot_comment */
    0,				/* xdot_library_shape */
};

gvrender_features_t render_features_dot = {
    GVRENDER_DOES_TRANSFORM,	/* not really - uses raw graph coords */  /* flags */
    0.,                         /* default pad - graph units */
    NULL,			/* knowncolors */
    0,				/* sizeof knowncolors */
    COLOR_STRING,		/* color_type */
};

gvrender_features_t render_features_xdot = {
    GVRENDER_DOES_TRANSFORM 	/* not really - uses raw graph coords */  
	| GVRENDER_DOES_MAPS
	| GVRENDER_DOES_TARGETS
	| GVRENDER_DOES_TOOLTIPS, /* flags */
    0.,                         /* default pad - graph units */
    NULL,			/* knowncolors */
    0,				/* sizeof knowncolors */
    RGBA_BYTE,		/* color_type */
};

gvdevice_features_t device_features_canon = {
    LAYOUT_NOT_REQUIRED,	/* flags */
    {0.,0.},			/* default margin - points */
    {0.,0.},                    /* default height, width - device units */
    {72.,72.},			/* default dpi */
};

gvdevice_features_t device_features_dot = {
    0,				/* flags */
    {0.,0.},			/* default margin - points */
    {0.,0.},			/* default page width, height - points */
    {72.,72.},			/* default dpi */
};

gvplugin_installed_t gvrender_dot_types[] = {
    {FORMAT_DOT, "dot", 1, &dot_engine, &render_features_dot},
    {FORMAT_XDOT, "xdot", 1, &xdot_engine, &render_features_xdot},
    {0, NULL, 0, NULL, NULL}
};

gvplugin_installed_t gvdevice_dot_types[] = {
    {FORMAT_DOT, "dot:dot", 1, NULL, &device_features_dot},
    {FORMAT_DOT, "gv:dot", 1, NULL, &device_features_dot},
    {FORMAT_CANON, "canon:dot", 1, NULL, &device_features_canon},
    {FORMAT_PLAIN, "plain:dot", 1, NULL, &device_features_dot},
    {FORMAT_PLAIN_EXT, "plain-ext:dot", 1, NULL, &device_features_dot},
    {FORMAT_XDOT, "xdot:xdot", 1, NULL, &device_features_dot},
    {FORMAT_XDOT12, "xdot1.2:xdot", 1, NULL, &device_features_dot},
    {FORMAT_XDOT14, "xdot1.4:xdot", 1, NULL, &device_features_dot},
    {0, NULL, 0, NULL, NULL}
};
