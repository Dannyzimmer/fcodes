/**
 * @file
 * @brief graphics code generator
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

#include "config.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <common/render.h>
#include <cgraph/agxbuf.h>
#include <cgraph/alloc.h>
#include <cgraph/list.h>
#include <cgraph/unreachable.h>
#include <common/htmltable.h>
#include <gvc/gvc.h>
#include <cdt/cdt.h>
#include <pathplan/pathgeom.h>
#include <xdot/xdot.h>

#ifdef _WIN32
#define strtok_r strtok_s
#endif

#define P2RECT(p, pr, sx, sy) (pr[0].x = p.x - sx, pr[0].y = p.y - sy, pr[1].x = p.x + sx, pr[1].y = p.y + sy)
#define FUZZ 3
#define EPSILON .0001

typedef struct {
    xdot_op op;
    boxf bb;
    textspan_t* span;
} exdot_op;

void* init_xdot (Agraph_t* g)
{
    char* p;
    xdot* xd = NULL;

    if (!((p = agget(g, "_background")) && p[0])) {
	if (!((p = agget(g, "_draw_")) && p[0])) {
	    return NULL;
	}
    }
#ifdef DEBUG
    if (Verbose) {
	start_timer();
    }
#endif
    xd = parseXDotF (p, NULL, sizeof (exdot_op));

    if (!xd) {
	agerr(AGWARN, "Could not parse \"_background\" attribute in graph %s\n", agnameof(g));
	agerr(AGPREV, "  \"%s\"\n", p);
    }
#ifdef DEBUG
    if (Verbose) {
	xdot_stats stats;
	double et = elapsed_sec();
	statXDot (xd, &stats);
	fprintf (stderr, "%d ops %.2f sec\n", stats.cnt, et);
	fprintf (stderr, "%d polygons %d points\n", stats.n_polygon, stats.n_polygon_pts);
	fprintf (stderr, "%d polylines %d points\n", stats.n_polyline, stats.n_polyline_pts);
	fprintf (stderr, "%d beziers %d points\n", stats.n_bezier, stats.n_bezier_pts);
	fprintf (stderr, "%d ellipses\n", stats.n_ellipse);
	fprintf (stderr, "%d texts\n", stats.n_text);
    }
#endif
    return xd;
}

static char *defaultlinestyle[3] = { "solid\0", "setlinewidth\0001\0", 0 };

/* push empty graphic state for current object */
obj_state_t* push_obj_state(GVJ_t *job)
{
    obj_state_t *obj = gv_alloc(sizeof(obj_state_t));

    obj_state_t *parent = obj->parent = job->obj;
    job->obj = obj;
    if (parent) {
        obj->pencolor = parent->pencolor;        /* default styles to parent's style */
        obj->fillcolor = parent->fillcolor;
        obj->pen = parent->pen;
        obj->fill = parent->fill;
        obj->penwidth = parent->penwidth;
	obj->gradient_angle = parent->gradient_angle;
	obj->stopcolor = parent->stopcolor;
    }
    else {
	/* obj->pencolor = NULL */
	/* obj->fillcolor = NULL */
	obj->pen = PEN_SOLID;
	obj->fill = FILL_NONE;
	obj->penwidth = PENWIDTH_NORMAL;
    }
    return obj;
}

/* pop graphic state of current object */
void pop_obj_state(GVJ_t *job)
{
    obj_state_t *obj = job->obj;

    assert(obj);

    free(obj->id);
    free(obj->url);
    free(obj->labelurl);
    free(obj->tailurl);
    free(obj->headurl);
    free(obj->tooltip);
    free(obj->labeltooltip);
    free(obj->tailtooltip);
    free(obj->headtooltip);
    free(obj->target);
    free(obj->labeltarget);
    free(obj->tailtarget);
    free(obj->headtarget);
    free(obj->url_map_p);
    free(obj->url_bsplinemap_p);
    free(obj->url_bsplinemap_n);

    job->obj = obj->parent;
    free(obj);
}

/* initMapData:
 * Store image map data into job, substituting for node, edge, etc.
 * names.
 * Return 1 if an assignment was made for url or tooltip or target.
 */
int
initMapData (GVJ_t* job, char* lbl, char* url, char* tooltip, char* target, char *id,
  void* gobj)
{
    obj_state_t *obj = job->obj;
    int flags = job->flags;
    int assigned = 0;

    if ((flags & GVRENDER_DOES_LABELS) && lbl)
        obj->label = lbl;
    if (flags & GVRENDER_DOES_MAPS) {
        obj->id = strdup_and_subst_obj(id, gobj);
	if (url && url[0]) {
            obj->url = strdup_and_subst_obj(url, gobj);
	    assigned = 1;
        }
    }
    if (flags & GVRENDER_DOES_TOOLTIPS) {
        if (tooltip && tooltip[0]) {
            obj->tooltip = strdup_and_subst_obj(tooltip, gobj);
            obj->explicit_tooltip = true;
	    assigned = 1;
        }
        else if (obj->label) {
            obj->tooltip = gv_strdup(obj->label);
	    assigned = 1;
        }
    }
    if ((flags & GVRENDER_DOES_TARGETS) && target && target[0]) {
        obj->target = strdup_and_subst_obj(target, gobj);
	assigned = 1;
    }
    return assigned;
}

static void
layerPagePrefix (GVJ_t* job, agxbuf* xb)
{
    if (job->layerNum > 1 && (job->flags & GVDEVICE_DOES_LAYERS)) {
	agxbprint (xb, "%s_", job->gvc->layerIDs[job->layerNum]);
    }
    if (job->pagesArrayElem.x > 0 || job->pagesArrayElem.y > 0) {
	agxbprint (xb, "page%d,%d_", job->pagesArrayElem.x, job->pagesArrayElem.y);
    }
}

/* genObjId:
 * Use id of root graph if any, plus kind and internal id of object
 */
char*
getObjId (GVJ_t* job, void* obj, agxbuf* xb)
{
    char* id;
    graph_t* root = job->gvc->g;
    char* gid = GD_drawing(root)->id;
    long idnum = 0;
    char* pfx = NULL;

    layerPagePrefix (job, xb);

    id = agget(obj, "id");
    if (id && *id != '\0') {
	agxbput (xb, id);
	return agxbuse(xb);
    }

    if (obj != root && gid) {
	agxbprint (xb, "%s_", gid);
    }

    switch (agobjkind(obj)) {
    case AGRAPH:
	idnum = AGSEQ(obj);
	if (root == obj)
	    pfx = "graph";
	else
	    pfx = "clust";
	break;
    case AGNODE:
        idnum = AGSEQ((Agnode_t*)obj);
	pfx = "node";
	break;
    case AGEDGE:
        idnum = AGSEQ((Agedge_t*)obj);
	pfx = "edge";
	break;
    }

    agxbprint (xb, "%s%ld", pfx, idnum);

    return agxbuse(xb);
}

/* interpretCRNL:
 * Map "\n" to ^J, "\r" to ^M and "\l" to ^J.
 * Map "\\" to backslash.
 * Map "\x" to x.
 * Mapping is done in place.
 * Return input string.
 */

static char*
interpretCRNL (char* ins)
{
    char* rets = ins;
    char* outs = ins;
    char c;
    bool backslash_seen = false;

    while ((c = *ins++)) {
	if (backslash_seen) {
	    switch (c) {
	    case 'n' :
	    case 'l' :
		*outs++ = '\n';
		break;
	    case 'r' :
		*outs++ = '\r';
		break;
	    default :
		*outs++ = c;
		break;
	    }
	    backslash_seen = false;
	}
	else {
	    if (c == '\\')
		backslash_seen = true;
	    else
		*outs++ = c;
	}
    }
    *outs = '\0';
    return rets;
}
 
/* preprocessTooltip:
 * Tooltips are a weak form of escString, so we expect object substitution
 * and newlines to be handled. The former occurs in initMapData. Here we
 * map "\r", "\l" and "\n" to newlines. (We don't try to handle alignment
 * as in real labels.) To make things uniform when the 
 * tooltip is emitted latter as visible text, we also convert HTML escape
 * sequences into UTF8. This is already occurring when tooltips are input
 * via HTML-like tables.
 */ 
static char*
preprocessTooltip(char* s, void* gobj)
{
    Agraph_t* g = agroot(gobj);
    int charset = GD_charset(g);
    char* news;
    switch (charset) {
	case CHAR_LATIN1:
	    news = latin1ToUTF8(s);
	    break;
	default: /* UTF8 */
	    news = htmlEntityUTF8(s, g);
	    break;
    }
    
    return interpretCRNL (news);
}
 
static void
initObjMapData (GVJ_t* job, textlabel_t *lab, void* gobj)
{
    char* lbl;
    char* url = agget(gobj, "href");
    char* tooltip = agget(gobj, "tooltip");
    char* target = agget(gobj, "target");
    char* id;
    agxbuf xb = {0};

    if (lab) lbl = lab->text;
    else lbl = NULL;
    if (!url || !*url)  /* try URL as an alias for href */
	url = agget(gobj, "URL");
    id = getObjId (job, gobj, &xb);
    if (tooltip) 
	tooltip = preprocessTooltip (tooltip, gobj);
    initMapData (job, lbl, url, tooltip, target, id, gobj);

    free (tooltip);
    agxbfree(&xb);
}

static void map_point(GVJ_t *job, pointf pf)
{
    obj_state_t *obj = job->obj;
    int flags = job->flags;
    pointf *p;

    if (flags & (GVRENDER_DOES_MAPS | GVRENDER_DOES_TOOLTIPS)) {
	if (flags & GVRENDER_DOES_MAP_RECTANGLE) {
	    obj->url_map_shape = MAP_RECTANGLE;
	    obj->url_map_n = 2;
	}
	else {
	    obj->url_map_shape = MAP_POLYGON;
	    obj->url_map_n = 4;
	}
	free(obj->url_map_p);
	obj->url_map_p = p = gv_calloc(obj->url_map_n, sizeof(pointf));
	P2RECT(pf, p, FUZZ, FUZZ);
	if (! (flags & GVRENDER_DOES_TRANSFORM))
	    gvrender_ptf_A(job, p, p, 2);
	if (! (flags & GVRENDER_DOES_MAP_RECTANGLE))
	    rect2poly(p);
    }
}

static char **checkClusterStyle(graph_t* sg, int *flagp)
{
    char *style;
    char **pstyle = NULL;
    int istyle = 0;

    if ((style = agget(sg, "style")) != 0 && style[0]) {
	char **pp;
	char **qp;
	char *p;
	pp = pstyle = parse_style(style);
	while ((p = *pp)) {
	    if (strcmp(p, "filled") == 0) {
		istyle |= FILLED;
		pp++;
 	    }else if (strcmp(p, "radial") == 0) {
 		istyle |= (FILLED | RADIAL);
		qp = pp; /* remove rounded from list passed to renderer */
		do {
		    qp++;
		    *(qp-1) = *qp;
		} while (*qp);
 	    }else if (strcmp(p, "striped") == 0) {
 		istyle |= STRIPED;
		qp = pp; /* remove rounded from list passed to renderer */
		do {
		    qp++;
		    *(qp-1) = *qp;
		} while (*qp);
	    }else if (strcmp(p, "rounded") == 0) {
		istyle |= ROUNDED;
		qp = pp; /* remove rounded from list passed to renderer */
		do {
		    qp++;
		    *(qp-1) = *qp;
		} while (*qp);
	    } else pp++;
	}
    }

    *flagp = istyle;
    return pstyle;
}

typedef struct {
    char* color;   /* segment color */
    float t;       /* segment size >= 0 */
    bool hasFraction;  /* true if color explicitly specifies its fraction */
} colorseg_t;
/* Sum of segment sizes should add to 1 */
typedef struct {
    int numc;     /* number of used segments in segs; may include segs with t == 0 */
    char* base;   /* storage of color names */
    colorseg_t* segs;  /* array of segments; real segments always followed by a sentinel */
} colorsegs_t;
 
static void
freeSegs (colorsegs_t* segs)
{
    free (segs->base);
    free (segs->segs);
}

/* getSegLen:
 * Find semicolon in s, replace with '\0'.
 * Convert remainder to float v.
 * Return 0 if no float given
 * Return -1 on failure
 */
static double getSegLen (char* s)
{
    char* p = strchr (s, ';');
    char* endp;
    double v;

    if (!p) {
	return 0;
    }
    *p++ = '\0';
    v = strtod (p, &endp);
    if (endp != p) {  /* scanned something */
	if (v >= 0)
	    return v;
    }
    return -1;
}

#define EPS 1E-5
#define AEQ0(x) (((x) < EPS) && ((x) > -EPS))

/* parseSegs:
 * Parse string of form color;float:color;float:...:color;float:color
 * where the semicolon-floats are optional, nonnegative, sum to <= 1.
 * Store the values in an array of colorseg_t's and return the array in psegs.
 * If nseg == 0, count the number of colors.
 * If the sum of the floats does not equal 1, the remainder is equally distributed
 * to all colors without an explicit float. If no such colors exist, the remainder
 * is added to the last color.
 *  0 => okay
 *  1 => error without message 
 *  2 => error with message 
 *  3 => warning message
 * There is a last sentinel segment with color == NULL; it will always follow
 * the last segment with t > 0.
 *
 * Note that psegs is only assigned to if the return value is 0 or 3.
 * Otherwise, psegs is left unchanged and the allocated memory is
 * freed before returning.
 */
static int parseSegs(char *clrs, int nseg, colorsegs_t *psegs) {
    colorsegs_t segs = {0};
    colorseg_t* s;
    char* colors = gv_strdup(clrs);
    char* color;
    int cnum = 0;
    double v, left = 1;
    static int doWarn = 1;
    int i, rval = 0;
    char* p;

    if (nseg == 0) {
	nseg = 1;
	/* need to know how many colors separated by ':' */
	for (p = colors; *p; p++) {
	    if (*p == ':') nseg++;
	}
    }

    segs.base = colors;
    segs.segs = s = gv_calloc(nseg + 1, sizeof(colorseg_t));
    for (color = strtok(colors, ":"); color; color = strtok(0, ":")) {
	if ((v = getSegLen (color)) >= 0) {
	    double del = v - left;
	    if (del > 0) {
		if (doWarn && !AEQ0(del)) {
		    agerr (AGWARN, "Total size > 1 in \"%s\" color spec ", clrs);
		    doWarn = 0;
		    rval = 3;
		}
		v = left;
	    }
	    left -= v;
	    if (v > 0) s[cnum].hasFraction = true;
	    if (*color) s[cnum].color = color;
	    s[cnum++].t = v;
	}
	else {
	    if (doWarn) {
		agerr (AGERR, "Illegal value in \"%s\" color attribute; float expected after ';'\n",
                    clrs);
		doWarn = 0;
		rval = 2;
	    }
	    else rval = 1;
	    freeSegs(&segs);
	    return rval;
	}
	if (AEQ0(left)) {
	    left = 0;
	    break;
	}
    }

    /* distribute remaining into slot with t == 0; if none, add to last */
    if (left > 0) {
	/* count zero segments */
	nseg = 0;
	for (i = 0; i < cnum; i++) {
	    if (!(s[i].t > 0)) nseg++;
	}
	if (nseg > 0) {
	    double delta = left/nseg;
	    for (i = 0; i < cnum; i++) {
		if (!(s[i].t > 0)) s[i].t = delta;
	    }
	}
	else {
	    s[cnum-1].t += left;
	}
    }
    
    /* Make sure last positive segment is followed by a sentinel. */
    for (i = cnum-1; i >= 0; i--) {
	if (s[i].t > 0) break;
    }
    s[i+1].color = NULL;
    segs.numc = i+1;

    *psegs = segs;
    return rval;
}

#define THIN_LINE 0.5

/* wedgedEllipse:
 * Fill an ellipse whose bounding box is given by 2 points in pf
 * with multiple wedges determined by the color spec in clrs.
 * clrs is a list of colon separated colors, with possible quantities. 
 * Thin boundaries are drawn.
 *  0 => okay
 *  1 => error without message 
 *  2 => error with message 
 *  3 => warning message
 */
int 
wedgedEllipse (GVJ_t* job, pointf * pf, char* clrs)
{
    colorsegs_t segs;
    int rv;
    double save_penwidth = job->obj->penwidth;
    pointf ctr, semi;
    Ppolyline_t* pp;
    double angle0, angle1;

    rv = parseSegs (clrs, 0, &segs);
    if (rv == 1 || rv == 2) return rv;
    ctr.x = (pf[0].x + pf[1].x) / 2.;
    ctr.y = (pf[0].y + pf[1].y) / 2.;
    semi.x = pf[1].x - ctr.x;
    semi.y = pf[1].y - ctr.y;
    if (save_penwidth > THIN_LINE)
	gvrender_set_penwidth(job, THIN_LINE);
	
    angle0 = 0;
    for (colorseg_t *s = segs.segs; s->color; s++) {
	if (!(s->t > 0)) continue;
	gvrender_set_fillcolor (job, (s->color?s->color:DEFAULT_COLOR));

	if (s[1].color == NULL) 
	    angle1 = 2*M_PI;
	else
	    angle1 = angle0 + 2 * M_PI * s->t;
	pp = ellipticWedge (ctr, semi.x, semi.y, angle0, angle1);
	gvrender_beziercurve(job, pp->ps, pp->pn, 1);
	angle0 = angle1;
	freePath (pp);
    }

    if (save_penwidth > THIN_LINE)
	gvrender_set_penwidth(job, save_penwidth);
    freeSegs(&segs);
    return rv;
}

/* stripedBox:
 * Fill a rectangular box with vertical stripes of colors.
 * AF gives 4 corner points, with AF[0] the LL corner and the points ordered CCW.
 * clrs is a list of colon separated colors, with possible quantities. 
 * Thin boundaries are drawn.
 *  0 => okay
 *  1 => error without message 
 *  2 => error with message 
 *  3 => warning message
 */
int
stripedBox (GVJ_t * job, pointf* AF, char* clrs, int rotate)
{
    colorsegs_t segs;
    int rv;
    double xdelta;
    pointf pts[4];
    double lastx;
    double save_penwidth = job->obj->penwidth;

    rv = parseSegs (clrs, 0, &segs);
    if (rv == 1 || rv == 2) return rv;
    if (rotate) {
	pts[0] = AF[2];
	pts[1] = AF[3];
	pts[2] = AF[0];
	pts[3] = AF[1];
    } else {
	pts[0] = AF[0];
	pts[1] = AF[1];
	pts[2] = AF[2];
	pts[3] = AF[3];
    }
    lastx = pts[1].x;
    xdelta = (pts[1].x - pts[0].x);
    pts[1].x = pts[2].x = pts[0].x;
    
    if (save_penwidth > THIN_LINE)
	gvrender_set_penwidth(job, THIN_LINE);
    for (colorseg_t *s = segs.segs; s->color; s++) {
	if (!(s->t > 0)) continue;
	gvrender_set_fillcolor (job, (s->color?s->color:DEFAULT_COLOR));
	/* gvrender_polygon(job, pts, 4, FILL | NO_POLY); */
	if (s[1].color == NULL) 
	    pts[1].x = pts[2].x = lastx;
	else
	    pts[1].x = pts[2].x = pts[0].x + xdelta*(s->t);
	gvrender_polygon(job, pts, 4, FILL);
	pts[0].x = pts[3].x = pts[1].x;
    }
    if (save_penwidth > THIN_LINE)
	gvrender_set_penwidth(job, save_penwidth);
    freeSegs(&segs);
    return rv;
}

void emit_map_rect(GVJ_t *job, boxf b)
{
    obj_state_t *obj = job->obj;
    int flags = job->flags;
    pointf *p;

    if (flags & (GVRENDER_DOES_MAPS | GVRENDER_DOES_TOOLTIPS)) {
	if (flags & GVRENDER_DOES_MAP_RECTANGLE) {
	    obj->url_map_shape = MAP_RECTANGLE;
	    obj->url_map_n = 2;
	}
	else {
	    obj->url_map_shape = MAP_POLYGON;
	    obj->url_map_n = 4;
	}
	free(obj->url_map_p);
	obj->url_map_p = p = gv_calloc(obj->url_map_n, sizeof(pointf));
	p[0] = b.LL;
	p[1] = b.UR;
	if (! (flags & GVRENDER_DOES_TRANSFORM))
	    gvrender_ptf_A(job, p, p, 2);
	if (! (flags & GVRENDER_DOES_MAP_RECTANGLE))
	    rect2poly(p);
    }
}

static void map_label(GVJ_t *job, textlabel_t *lab)
{
    obj_state_t *obj = job->obj;
    int flags = job->flags;
    pointf *p;

    if (flags & (GVRENDER_DOES_MAPS | GVRENDER_DOES_TOOLTIPS)) {
	if (flags & GVRENDER_DOES_MAP_RECTANGLE) {
	    obj->url_map_shape = MAP_RECTANGLE;
	    obj->url_map_n = 2;
	}
	else {
	    obj->url_map_shape = MAP_POLYGON;
	    obj->url_map_n = 4;
	}
	free(obj->url_map_p);
	obj->url_map_p = p = gv_calloc(obj->url_map_n, sizeof(pointf));
	P2RECT(lab->pos, p, lab->dimen.x / 2., lab->dimen.y / 2.);
	if (! (flags & GVRENDER_DOES_TRANSFORM))
	    gvrender_ptf_A(job, p, p, 2);
	if (! (flags & GVRENDER_DOES_MAP_RECTANGLE))
	    rect2poly(p);
    }
}

/* isRect:
 * isRect function returns true when polygon has
 * regular rectangular shape. Rectangle is regular when
 * it is not skewed and distorted and orientation is almost zero
 */
static bool isRect(polygon_t * p)
{
    return p->sides == 4 && ROUND(p->orientation) % 90 == 0
            && p->distortion == 0.0 && p->skew == 0.0;
}

/*
 * isFilled function returns true if filled style has been set for node 'n'
 * otherwise returns false. it accepts pointer to node_t as an argument
 */
static bool isFilled(node_t * n)
{
    char *style, *p, **pp;
    bool r = false;
    style = late_nnstring(n, N_style, "");
    if (style[0]) {
        pp = parse_style(style);
        while ((p = *pp)) {
            if (strcmp(p, "filled") == 0)
                r = true;
            pp++;
        }
    }
    return r;
}

/* pEllipse:
 * pEllipse function returns 'np' points from the circumference
 * of ellipse described by radii 'a' and 'b'.
 * Assumes 'np' is greater than zero.
 * 'np' should be at least 4 to sample polygon from ellipse
 */
static pointf *pEllipse(double a, double b, size_t np) {
    double theta = 0.0;
    double deltheta = 2 * M_PI / (double)np;

    pointf *ps = gv_calloc(np, sizeof(pointf));
    for (size_t i = 0; i < np; i++) {
        ps[i].x = a * cos(theta);
        ps[i].y = b * sin(theta);
        theta += deltheta;
    }
    return ps;
}

#define HW 2.0   /* maximum distance away from line, in points */

/* check_control_points:
 * check_control_points function checks the size of quadrilateral
 * formed by four control points
 * returns true if four points are in line (or close to line)
 * else return false
 */
static bool check_control_points(pointf *cp)
{
    double dis1 = ptToLine2 (cp[0], cp[3], cp[1]);
    double dis2 = ptToLine2 (cp[0], cp[3], cp[2]);
    return dis1 < HW * HW && dis2 < HW * HW;
}

/* update bounding box to contain a bezier segment */
void update_bb_bz(boxf *bb, pointf *cp)
{

    /* if any control point of the segment is outside the bounding box */
    if (cp[0].x > bb->UR.x || cp[0].x < bb->LL.x ||
        cp[0].y > bb->UR.y || cp[0].y < bb->LL.y ||
        cp[1].x > bb->UR.x || cp[1].x < bb->LL.x ||
        cp[1].y > bb->UR.y || cp[1].y < bb->LL.y ||
        cp[2].x > bb->UR.x || cp[2].x < bb->LL.x ||
        cp[2].y > bb->UR.y || cp[2].y < bb->LL.y ||
        cp[3].x > bb->UR.x || cp[3].x < bb->LL.x ||
        cp[3].y > bb->UR.y || cp[3].y < bb->LL.y) {

        /* if the segment is sufficiently refined */
        if (check_control_points(cp)) {        
            int i;
            /* expand the bounding box */
            for (i = 0; i < 4; i++) {
                if (cp[i].x > bb->UR.x)
                    bb->UR.x = cp[i].x;
                else if (cp[i].x < bb->LL.x)
                    bb->LL.x = cp[i].x;
                if (cp[i].y > bb->UR.y)
                    bb->UR.y = cp[i].y;
                else if (cp[i].y < bb->LL.y)
                    bb->LL.y = cp[i].y;
            }
        }
        else { /* else refine the segment */
            pointf left[4], right[4];
            Bezier (cp, 3, 0.5, left, right);
            update_bb_bz(bb, left);
            update_bb_bz(bb, right);
        }
    }
}

#if defined(DEBUG) && DEBUG == 2
static void psmapOutput (pointf* ps, int n)
{
   int i;
   fprintf (stdout, "newpath %f %f moveto\n", ps[0].x, ps[0].y);
   for (i=1; i < n; i++)
        fprintf (stdout, "%f %f lineto\n", ps[i].x, ps[i].y);
   fprintf (stdout, "closepath stroke\n");
}
#endif

typedef struct segitem_s {
    pointf p;
    struct segitem_s* next;
} segitem_t;

#define MARK_FIRST_SEG(L) ((L)->next = (segitem_t*)1)
#define FIRST_SEG(L) ((L)->next == (segitem_t*)1)
#define INIT_SEG(P,L) {(L)->next = 0; (L)->p = P;} 

static segitem_t* appendSeg (pointf p, segitem_t* lp)
{
    segitem_t* s = gv_alloc(sizeof(segitem_t));
    INIT_SEG (p, s);
    lp->next = s;
    return s;
}

/* map_bspline_poly:
 * Output the polygon determined by the n points in p1, followed
 * by the n points in p2 in reverse order. Assumes n <= 50.
 */
static void map_bspline_poly(pointf **pbs_p, int **pbs_n, int *pbs_poly_n, int n, pointf* p1, pointf* p2)
{
    int i = 0, nump = 0, last = 2*n-1;

    for ( ; i < *pbs_poly_n; i++)
        nump += (*pbs_n)[i];

    (*pbs_poly_n)++;
    *pbs_n = grealloc(*pbs_n, (*pbs_poly_n) * sizeof(int));
    (*pbs_n)[i] = 2*n;
    *pbs_p = grealloc(*pbs_p, (nump + 2*n) * sizeof(pointf));

    for (i = 0; i < n; i++) {
        (*pbs_p)[nump+i] = p1[i];
        (*pbs_p)[nump+last-i] = p2[i];
    }
#if defined(DEBUG) && DEBUG == 2
    psmapOutput (*pbs_p + nump, last+1);
#endif
}

/* approx_bezier:
 * Approximate Bezier by line segments. If the four points are
 * almost colinear, as determined by check_control_points, we store
 * the segment cp[0]-cp[3]. Otherwise we split the Bezier into 2 and recurse. 
 * Since 2 contiguous segments share an endpoint, we actually store
 * the segments as a list of points.
 * New points are appended to the list given by lp. The tail of the
 * list is returned.
 */
static segitem_t* approx_bezier (pointf *cp, segitem_t* lp)
{
    pointf left[4], right[4];

    if (check_control_points(cp)) {
        if (FIRST_SEG (lp)) INIT_SEG (cp[0], lp);
        lp = appendSeg (cp[3], lp);
    }
    else {
        Bezier (cp, 3, 0.5, left, right);
        lp = approx_bezier (left, lp);
        lp = approx_bezier (right, lp);
    }
    return lp;
}

/* bisect:
 * Return the angle of the bisector between the two rays
 * pp-cp and cp-np. The bisector returned is always to the
 * left of pp-cp-np.
 */
static double bisect (pointf pp, pointf cp, pointf np)
{
  double ang, theta, phi;
  theta = atan2(np.y - cp.y,np.x - cp.x);
  phi = atan2(pp.y - cp.y,pp.x - cp.x);
  ang = theta - phi;
  if (ang > 0) ang -= 2*M_PI;

  return phi + ang / 2.0;
}

/* mkSegPts:
 * Determine polygon points related to 2 segments prv-cur and cur-nxt.
 * The points lie on the bisector of the 2 segments, passing through cur,
 * and distance w2 from cur. The points are stored in p1 and p2.
 * If p1 is NULL, we use the normal to cur-nxt.
 * If p2 is NULL, we use the normal to prv-cur.
 * Assume at least one of prv or nxt is non-NULL.
 */
static void mkSegPts (segitem_t* prv, segitem_t* cur, segitem_t* nxt,
        pointf* p1, pointf* p2, double w2)
{
    pointf cp, pp, np;
    double theta, delx, dely;
    pointf p;

    cp = cur->p;
    /* if prv or nxt are NULL, use the one given to create a collinear
     * prv or nxt. This could be more efficiently done with special case code, 
     * but this way is more uniform.
     */
    if (prv) {
        pp = prv->p;
        if (nxt)
            np = nxt->p;
        else {
            np.x = 2*cp.x - pp.x;
            np.y = 2*cp.y - pp.y;
        }
    }
    else {
        np = nxt->p;
        pp.x = 2*cp.x - np.x;
        pp.y = 2*cp.y - np.y;
    }
    theta = bisect(pp,cp,np);
    delx = w2*cos(theta);
    dely = w2*sin(theta);
    p.x = cp.x + delx;
    p.y = cp.y + dely;
    *p1 = p;
    p.x = cp.x - delx;
    p.y = cp.y - dely;
    *p2 = p;
}

/* map_output_bspline:
 * Construct and output a closed polygon approximating the input
 * B-spline bp. We do this by first approximating bp by a sequence
 * of line segments. We then use the sequence of segments to determine
 * the polygon.
 * In cmapx, polygons are limited to 100 points, so we output polygons
 * in chunks of 100.
 */
static void map_output_bspline (pointf **pbs, int **pbs_n, int *pbs_poly_n, bezier* bp, double w2)
{
    segitem_t* segl = gv_alloc(sizeof(segitem_t));
    segitem_t* segp = segl;
    segitem_t* segprev;
    segitem_t* segnext;
    int nc, j, k, cnt;
    pointf pts[4], pt1[50], pt2[50];

    MARK_FIRST_SEG(segl);
    nc = (bp->size - 1)/3; /* nc is number of bezier curves */
    for (j = 0; j < nc; j++) {
        for (k = 0; k < 4; k++) {
            pts[k] = bp->list[3*j + k];
        }
        segp = approx_bezier (pts, segp);
    }

    segp = segl;
    segprev = 0;
    cnt = 0;
    while (segp) {
        segnext = segp->next;
        mkSegPts (segprev, segp, segnext, pt1+cnt, pt2+cnt, w2);
        cnt++;
        if (segnext == NULL || cnt == 50) {
            map_bspline_poly (pbs, pbs_n, pbs_poly_n, cnt, pt1, pt2);
            pt1[0] = pt1[cnt-1];
            pt2[0] = pt2[cnt-1];
            cnt = 1;
        }
        segprev = segp;
        segp = segnext;
    }

    /* free segl */
    while (segl) {
        segp = segl->next;
        free (segl);
        segl = segp;
    }
}

static bool is_natural_number(const char *sstr)
{
    const char *str = sstr;

    while (*str)
	if (!isdigit((int)*str++))
	    return false;
    return true;
}

static int layer_index(GVC_t *gvc, char *str, int all)
{
    int i;

    if (streq(str, "all"))
	return all;
    if (is_natural_number(str))
	return atoi(str);
    if (gvc->layerIDs)
	for (i = 1; i <= gvc->numLayers; i++)
	    if (streq(str, gvc->layerIDs[i]))
		return i;
    return -1;
}

static bool selectedLayer(GVC_t *gvc, int layerNum, int numLayers, char *spec)
{
    int n0, n1;
    char *w0, *w1;
    char *buf_part_p = NULL, *buf_p = NULL, *cur, *part_in_p;
    bool rval = false;

    // copy `spec` so we can `strtok_r` it
    char *spec_copy = gv_strdup(spec);
    part_in_p = spec_copy;

    while (!rval && (cur = strtok_r(part_in_p, gvc->layerListDelims, &buf_part_p))) {
	w1 = w0 = strtok_r (cur, gvc->layerDelims, &buf_p);
	if (w0)
	    w1 = strtok_r (NULL, gvc->layerDelims, &buf_p);
	if (w1 != NULL) {
	    assert(w0 != NULL);
	    n0 = layer_index(gvc, w0, 0);
	    n1 = layer_index(gvc, w1, numLayers);
	    if (n0 >= 0 || n1 >= 0) {
		if (n0 > n1) {
		    int t = n0;
		    n0 = n1;
		    n1 = t;
		}
		rval = BETWEEN(n0, layerNum, n1);
	    }
	} else if (w0 != NULL) {
	    n0 = layer_index(gvc, w0, layerNum);
	    rval = (n0 == layerNum);
	} else {
	    rval = false;
	}
	part_in_p = NULL;
    }
    free(spec_copy);
    return rval;
}

static bool selectedlayer(GVJ_t *job, char *spec)
{
    return selectedLayer (job->gvc, job->layerNum, job->numLayers, spec);
}

/* parse_layerselect:
 * Parse the graph's layerselect attribute, which determines
 * which layers are emitted. The specification is the same used
 * by the layer attribute.
 *
 * If we find n layers, we return an array arr of n+2 ints. arr[0]=n.
 * arr[n+1]=numLayers+1, acting as a sentinel. The other entries give
 * the desired layer indices.
 *
 * If no layers are detected, NULL is returned.
 *
 * This implementation does a linear walk through each layer index and
 * uses selectedLayer to match it against p. There is probably a more
 * efficient way to do this, but this is simple and until we find people
 * using huge numbers of layers, it should be adequate.
 */
static int *parse_layerselect(GVC_t *gvc, char *p) {
    int* laylist = gv_calloc(gvc->numLayers + 2, sizeof(int));
    int i, cnt = 0;
    for (i = 1; i <=gvc->numLayers; i++) {
	if (selectedLayer (gvc, i, gvc->numLayers, p)) {
	    laylist[++cnt] = i;
	} 
    }
    if (cnt) {
	laylist[0] = cnt;
	laylist[cnt+1] = gvc->numLayers+1;
    }
    else {
	agerr(AGWARN, "The layerselect attribute \"%s\" does not match any layer specifed by the layers attribute - ignored.\n", p);
	free (laylist);
	laylist = NULL;
    }
    return laylist;
}

DEFINE_LIST(layer_names, char*)

/* parse_layers:
 * Split input string into tokens, with separators specified by
 * the layersep attribute. Store the values in the gvc->layerIDs array,
 * starting at index 1, and return the count.
 * Note that there is no mechanism
 * to free the memory before exit.
 */
static int parse_layers(GVC_t *gvc, graph_t * g, char *p)
{
    char *tok;

    gvc->layerDelims = agget(g, "layersep");
    if (!gvc->layerDelims)
        gvc->layerDelims = DEFAULT_LAYERSEP;
    gvc->layerListDelims = agget(g, "layerlistsep");
    if (!gvc->layerListDelims)
        gvc->layerListDelims = DEFAULT_LAYERLISTSEP;
    if ((tok = strpbrk (gvc->layerDelims, gvc->layerListDelims))) { /* conflict in delimiter strings */
	agerr(AGWARN, "The character \'%c\' appears in both the layersep and layerlistsep attributes - layerlistsep ignored.\n", *tok);
        gvc->layerListDelims = "";
    }

    gvc->layers = gv_strdup(p);
    layer_names_t layerIDs = {0};

    // inferred entry for the first (unnamed) layer
    layer_names_append(&layerIDs, NULL);

    for (tok = strtok(gvc->layers, gvc->layerDelims); tok;
         tok = strtok(NULL, gvc->layerDelims)) {
        layer_names_append(&layerIDs, tok);
    }

    assert(layer_names_size(&layerIDs) - 1 <= INT_MAX);
    int ntok = (int)(layer_names_size(&layerIDs) - 1);

    // if we found layers, save them for later reference
    if (layer_names_size(&layerIDs) > 1) {
        layer_names_append(&layerIDs, NULL); // add a terminating entry
        gvc->layerIDs = layer_names_detach(&layerIDs);
    }
    layer_names_free(&layerIDs);

    return ntok;
}

/* chkOrder:
 * Determine order of output.
 * Output usually in breadth first graph walk order
 */
static int chkOrder(graph_t * g)
{
    char *p = agget(g, "outputorder");
    if (p) {
        if (!strcmp(p, "nodesfirst"))
            return EMIT_SORTED;
        if (!strcmp(p, "edgesfirst"))
            return EMIT_EDGE_SORTED;
    }
    return 0;
}

static void init_layering(GVC_t * gvc, graph_t * g)
{
    char *str;

    /* free layer strings and pointers from previous graph */
    free(gvc->layers);
    gvc->layers = NULL;
    free(gvc->layerIDs);
    gvc->layerIDs = NULL;
    free(gvc->layerlist);
    gvc->layerlist = NULL;
    if ((str = agget(g, "layers")) != 0) {
	gvc->numLayers = parse_layers(gvc, g, str);
 	if ((str = agget(g, "layerselect")) != 0 && *str) {
	    gvc->layerlist = parse_layerselect(gvc, str);
	}
    } else {
	gvc->numLayers = 1;
    }
}

/* numPhysicalLayers:
 * Return number of physical layers to be emitted.
 */
static int numPhysicalLayers (GVJ_t *job)
{
    if (job->gvc->layerlist) {
	return job->gvc->layerlist[0];
    }
    else
	return job->numLayers;

}

static void firstlayer(GVJ_t *job, int** listp)
{
    job->numLayers = job->gvc->numLayers;
    if (job->gvc->layerlist) {
	int *list = job->gvc->layerlist;
	int cnt = *list++;
	if (cnt > 1 && !(job->flags & GVDEVICE_DOES_LAYERS)) {
	    agerr(AGWARN, "layers not supported in %s output\n",
		job->output_langname);
	    list[1] = job->numLayers + 1; /* only one layer printed */
	}
	job->layerNum = *list++;
	*listp = list;
    }
    else {
	if (job->numLayers > 1 && !(job->flags & GVDEVICE_DOES_LAYERS)) {
	    agerr(AGWARN, "layers not supported in %s output\n",
		job->output_langname);
	    job->numLayers = 1;
	}
	job->layerNum = 1;
	*listp = NULL;
    }
}

static bool validlayer(GVJ_t *job)
{
    return job->layerNum <= job->numLayers;
}

static void nextlayer(GVJ_t *job, int** listp)
{
    int *list = *listp;
    if (list) {
	job->layerNum = *list++;
	*listp = list;
    }
    else
	job->layerNum++;
}

static point pagecode(GVJ_t *job, char c)
{
    point rv;
    rv.x = rv.y = 0;
    switch (c) {
    case 'T':
	job->pagesArrayFirst.y = job->pagesArraySize.y - 1;
	rv.y = -1;
	break;
    case 'B':
	rv.y = 1;
	break;
    case 'L':
	rv.x = 1;
	break;
    case 'R':
	job->pagesArrayFirst.x = job->pagesArraySize.x - 1;
	rv.x = -1;
	break;
    }
    return rv;
}

static void init_job_pagination(GVJ_t * job, graph_t *g)
{
    GVC_t *gvc = job->gvc;
    pointf pageSize;	/* page size for the graph - points*/
    pointf imageSize;	/* image size on one page of the graph - points */
    pointf margin;	/* margin for a page of the graph - points */
    pointf centering = {0.0, 0.0}; /* centering offset - points */

    /* unpaginated image size - in points - in graph orientation */
    imageSize = job->view;

    /* rotate imageSize to page orientation */
    if (job->rotation)
	imageSize = exch_xyf(imageSize);

    /* margin - in points - in page orientation */
    margin = job->margin;

    /* determine pagination */
    if (gvc->graph_sets_pageSize && (job->flags & GVDEVICE_DOES_PAGES)) {
	/* page was set by user */

        /* determine size of page for image */
	pageSize.x = gvc->pageSize.x - 2 * margin.x;
	pageSize.y = gvc->pageSize.y - 2 * margin.y;

	if (pageSize.x < EPSILON)
	    job->pagesArraySize.x = 1;
	else {
	    job->pagesArraySize.x = (int)(imageSize.x / pageSize.x);
	    if (imageSize.x - job->pagesArraySize.x * pageSize.x > EPSILON)
		job->pagesArraySize.x++;
	}
	if (pageSize.y < EPSILON)
	    job->pagesArraySize.y = 1;
	else {
	    job->pagesArraySize.y = (int)(imageSize.y / pageSize.y);
	    if (imageSize.y - job->pagesArraySize.y * pageSize.y > EPSILON)
		job->pagesArraySize.y++;
	}
	job->numPages = job->pagesArraySize.x * job->pagesArraySize.y;

	/* find the drawable size in points */
	imageSize.x = MIN(imageSize.x, pageSize.x);
	imageSize.y = MIN(imageSize.y, pageSize.y);
    } else {
	/* page not set by user, use default from renderer */
	if (job->render.features) {
	    pageSize.x = job->device.features->default_pagesize.x - 2*margin.x;
	    pageSize.x = fmax(pageSize.x, 0);
	    pageSize.y = job->device.features->default_pagesize.y - 2*margin.y;
	    pageSize.y = fmax(pageSize.y, 0);
	}
	else
	    pageSize.x = pageSize.y = 0.;
	job->pagesArraySize.x = job->pagesArraySize.y = job->numPages = 1;
        
        pageSize.x = fmax(pageSize.x, imageSize.x);
        pageSize.y = fmax(pageSize.y, imageSize.y);
    }

    /* initial window size */
//fprintf(stderr,"page=%g,%g dpi=%g,%g zoom=%g\n", pageSize.x, pageSize.y, job->dpi.x, job->dpi.y, job->zoom);
    job->width = ROUND((pageSize.x + 2*margin.x) * job->dpi.x / POINTS_PER_INCH);
    job->height = ROUND((pageSize.y + 2*margin.y) * job->dpi.y / POINTS_PER_INCH);

    /* set up pagedir */
    job->pagesArrayMajor.x = job->pagesArrayMajor.y 
		= job->pagesArrayMinor.x = job->pagesArrayMinor.y = 0;
    job->pagesArrayFirst.x = job->pagesArrayFirst.y = 0;
    job->pagesArrayMajor = pagecode(job, gvc->pagedir[0]);
    job->pagesArrayMinor = pagecode(job, gvc->pagedir[1]);
    if (abs(job->pagesArrayMajor.x + job->pagesArrayMinor.x) != 1
     || abs(job->pagesArrayMajor.y + job->pagesArrayMinor.y) != 1) {
	job->pagesArrayMajor = pagecode(job, 'B');
	job->pagesArrayMinor = pagecode(job, 'L');
	agerr(AGWARN, "pagedir=%s ignored\n", gvc->pagedir);
    }

    /* determine page box including centering */
    if (GD_drawing(g)->centered) {
	if (pageSize.x > imageSize.x)
	    centering.x = (pageSize.x - imageSize.x) / 2;
	if (pageSize.y > imageSize.y)
	    centering.y = (pageSize.y - imageSize.y) / 2;
    }

    /* rotate back into graph orientation */
    if (job->rotation) {
	imageSize = exch_xyf(imageSize);
	pageSize = exch_xyf(pageSize);
	margin = exch_xyf(margin);
	centering = exch_xyf(centering);
    }

    /* canvas area, centered if necessary */
    job->canvasBox.LL.x = margin.x + centering.x;
    job->canvasBox.LL.y = margin.y + centering.y;
    job->canvasBox.UR.x = margin.x + centering.x + imageSize.x;
    job->canvasBox.UR.y = margin.y + centering.y + imageSize.y;

    /* size of one page in graph units */
    job->pageSize.x = imageSize.x / job->zoom;
    job->pageSize.y = imageSize.y / job->zoom;

    /* pageBoundingBox in device units and page orientation */
    job->pageBoundingBox.LL.x = ROUND(job->canvasBox.LL.x * job->dpi.x / POINTS_PER_INCH);
    job->pageBoundingBox.LL.y = ROUND(job->canvasBox.LL.y * job->dpi.y / POINTS_PER_INCH);
    job->pageBoundingBox.UR.x = ROUND(job->canvasBox.UR.x * job->dpi.x / POINTS_PER_INCH);
    job->pageBoundingBox.UR.y = ROUND(job->canvasBox.UR.y * job->dpi.y / POINTS_PER_INCH);
    if (job->rotation) {
        job->pageBoundingBox.LL = exch_xy(job->pageBoundingBox.LL);
        job->pageBoundingBox.UR = exch_xy(job->pageBoundingBox.UR);
        job->canvasBox.LL = exch_xyf(job->canvasBox.LL);
        job->canvasBox.UR = exch_xyf(job->canvasBox.UR);
    }
}

static void firstpage(GVJ_t *job)
{
    job->pagesArrayElem = job->pagesArrayFirst;
}

static bool validpage(GVJ_t *job)
{
    return job->pagesArrayElem.x >= 0
	 && job->pagesArrayElem.x < job->pagesArraySize.x
	 && job->pagesArrayElem.y >= 0
	 && job->pagesArrayElem.y < job->pagesArraySize.y;
}

static void nextpage(GVJ_t *job)
{
    job->pagesArrayElem = add_point(job->pagesArrayElem, job->pagesArrayMinor);
    if (!validpage(job)) {
	if (job->pagesArrayMajor.y)
	    job->pagesArrayElem.x = job->pagesArrayFirst.x;
	else
	    job->pagesArrayElem.y = job->pagesArrayFirst.y;
	job->pagesArrayElem = add_point(job->pagesArrayElem, job->pagesArrayMajor);
    }
}

static bool write_edge_test(Agraph_t * g, Agedge_t * e)
{
    Agraph_t *sg;
    int c;

    for (c = 1; c <= GD_n_cluster(g); c++) {
	sg = GD_clust(g)[c];
	if (agcontains(sg, e))
	    return false;
    }
    return true;
}

static bool write_node_test(Agraph_t * g, Agnode_t * n)
{
    Agraph_t *sg;
    int c;

    for (c = 1; c <= GD_n_cluster(g); c++) {
	sg = GD_clust(g)[c];
	if (agcontains(sg, n))
	    return false;
    }
    return true;
}

static pointf *copyPts(xdot_point *inpts, size_t numpts) {
    pointf *pts = gv_calloc(numpts, sizeof(pointf));
    for (size_t i = 0; i < numpts; i++) {
	pts[i].x = inpts[i].x;
	pts[i].y = inpts[i].y;
    }
    return pts;
}

static void emit_xdot (GVJ_t * job, xdot* xd)
{
    int image_warn = 1;
    exdot_op* op;
    int angle;
    char** styles = NULL;
    int filled = FILL;

    op = (exdot_op*)(xd->ops);
    for (size_t i = 0; i < xd->cnt; i++) {
	switch (op->op.kind) {
	case xd_filled_ellipse :
	case xd_unfilled_ellipse :
    	    if (boxf_overlap(op->bb, job->clip)) {
		pointf pts[] = {{.x = op->op.u.ellipse.x - op->op.u.ellipse.w,
		                 .y = op->op.u.ellipse.y - op->op.u.ellipse.h},
		                {.x = op->op.u.ellipse.x + op->op.u.ellipse.w,
		                 .y = op->op.u.ellipse.y + op->op.u.ellipse.h}};
		gvrender_ellipse(job, pts, op->op.kind == xd_filled_ellipse ? filled : 0);
	    }
	    break;
	case xd_filled_polygon :
	case xd_unfilled_polygon :
    	    if (boxf_overlap(op->bb, job->clip)) {
		pointf *pts = copyPts(op->op.u.polygon.pts, op->op.u.polygon.cnt);
		assert(op->op.u.polygon.cnt <= INT_MAX &&
		       "polygon count exceeds gvrender_polygon support");
		gvrender_polygon(job, pts, (int)op->op.u.polygon.cnt,
		                 op->op.kind == xd_filled_polygon ? filled : 0);
		free(pts);
	    }
	    break;
	case xd_filled_bezier :
	case xd_unfilled_bezier :
    	    if (boxf_overlap(op->bb, job->clip)) {
		pointf *pts = copyPts(op->op.u.bezier.pts, op->op.u.bezier.cnt);
		assert(op->op.u.bezier.cnt <= INT_MAX &&
		       "polygon count exceeds gvrender_beizercurve support");
		gvrender_beziercurve(job, pts, (int)op->op.u.bezier.cnt,
		                     op->op.kind == xd_filled_bezier ? filled : 0);
		free(pts);
	    }
	    break;
	case xd_polyline :
    	    if (boxf_overlap(op->bb, job->clip)) {
		pointf *pts = copyPts(op->op.u.polyline.pts, op->op.u.polyline.cnt);
		assert(op->op.u.polyline.cnt <= INT_MAX &&
		       "polygon count exceeds gvrender_polyline support");
		gvrender_polyline(job, pts, (int)op->op.u.polyline.cnt);
		free(pts);
	    }
	    break;
	case xd_text :
    	    if (boxf_overlap(op->bb, job->clip)) {
		pointf pt = {.x = op->op.u.text.x, .y = op->op.u.text.y};
		gvrender_textspan(job, pt, op->span);
	    }
	    break;
	case xd_fill_color :
            gvrender_set_fillcolor(job, op->op.u.color);
	    filled = FILL;
	    break;
	case xd_pen_color :
            gvrender_set_pencolor(job, op->op.u.color);
	    filled = FILL;
	    break;
	case xd_grad_fill_color : 
	    {
		char* clr0;
		char* clr1;
		float frac;
		if (op->op.u.grad_color.type == xd_radial) {
		    xdot_radial_grad* p = &op->op.u.grad_color.u.ring;
		    clr0 = p->stops[0].color;
		    clr1 = p->stops[1].color;
		    frac = p->stops[1].frac;
		    if (p->x1 == p->x0 && p->y1 == p->y0)
			angle = 0;
		    else
			angle = (int)(180.0*acos((p->x0 - p->x1)/p->r0)/M_PI);
        	    gvrender_set_fillcolor(job, clr0);
		    gvrender_set_gradient_vals(job, clr1, angle, frac);
		    filled = RGRADIENT;
		}
		else {
		    xdot_linear_grad* p = &op->op.u.grad_color.u.ling;
		    clr0 = p->stops[0].color;
		    clr1 = p->stops[1].color;
		    frac = p->stops[1].frac;
		    angle = (int)(180.0*atan2(p->y1-p->y0,p->x1-p->x0)/M_PI);
        	    gvrender_set_fillcolor(job, clr0);
		    gvrender_set_gradient_vals(job, clr1, angle, frac);
		    filled = GRADIENT;
		}
	    }
	    break;
	case xd_grad_pen_color :
	    agerr (AGWARN, "gradient pen colors not yet supported.\n");
	    break;
	case xd_font :
	    /* fontsize and fontname already encoded via xdotBB */
	    break;
	case xd_style :
	    styles = parse_style (op->op.u.style);
            gvrender_set_style (job, styles);
	    break;
	case xd_fontchar :
	    /* font characteristics already encoded via xdotBB */
	    break;
	case xd_image :
	    if (image_warn) {
	        agerr(AGWARN, "Images unsupported in \"background\" attribute\n");
	        image_warn = 0;
	    }
	    break;
	default:
	    UNREACHABLE();
	}
	op++;
    }
    if (styles)
	gvrender_set_style(job, job->gvc->defaultlinestyle);
}

static void emit_background(GVJ_t * job, graph_t *g)
{
    xdot* xd;
    char *str;
    int dfltColor;
    
    /* if no bgcolor specified - first assume default of "white" */
    if (! ((str = agget(g, "bgcolor")) && str[0])) {
	str = "white";
	dfltColor = 1;
    }
    else
	dfltColor = 0;
    

    /* if device has no truecolor support, change "transparent" to "white" */
    if (! (job->flags & GVDEVICE_DOES_TRUECOLOR) && (streq(str, "transparent"))) {
	str = "white";
	dfltColor = 1;
    }

    /* except for "transparent" on truecolor, or default "white" on (assumed) white paper, paint background */
    if (!(   ((job->flags & GVDEVICE_DOES_TRUECOLOR) && streq(str, "transparent"))
          || ((job->flags & GVRENDER_NO_WHITE_BG) && dfltColor))) {
	char* clrs[2];
	float frac;

	if ((findStopColor (str, clrs, &frac))) {
	    int filled, istyle = 0;
            gvrender_set_fillcolor(job, clrs[0]);
            gvrender_set_pencolor(job, "transparent");
	    checkClusterStyle(g, &istyle);
	    if (clrs[1]) 
		gvrender_set_gradient_vals(job,clrs[1],late_int(g,G_gradientangle,0,0), frac);
	    else 
		gvrender_set_gradient_vals(job,DEFAULT_COLOR,late_int(g,G_gradientangle,0,0), frac);
	    if (istyle & RADIAL)
		filled = RGRADIENT;
	    else
		filled = GRADIENT;
	    gvrender_box(job, job->clip, filled);
	    free (clrs[0]);
	}
	else {
            gvrender_set_fillcolor(job, str);
    	    gvrender_set_pencolor(job, "transparent");
	    gvrender_box(job, job->clip, FILL);	/* filled */
	}
    }

    if ((xd = GD_drawing(g)->xdots))
	emit_xdot (job, xd);
}

static void setup_page(GVJ_t * job)
{
    point pagesArrayElem = job->pagesArrayElem, pagesArraySize = job->pagesArraySize;
	
    if (job->rotation) {
	pagesArrayElem = exch_xy(pagesArrayElem);
	pagesArraySize = exch_xy(pagesArraySize);
    }

    /* establish current box in graph units */
    job->pageBox.LL.x = pagesArrayElem.x * job->pageSize.x - job->pad.x;
    job->pageBox.LL.y = pagesArrayElem.y * job->pageSize.y - job->pad.y;
    job->pageBox.UR.x = job->pageBox.LL.x + job->pageSize.x;
    job->pageBox.UR.y = job->pageBox.LL.y + job->pageSize.y;

    /* maximum boundingBox in device units and page orientation */
    if (job->common->viewNum == 0)
        job->boundingBox = job->pageBoundingBox;
    else
        EXPANDBB(job->boundingBox, job->pageBoundingBox);

    if (job->flags & GVDEVICE_EVENTS) {
        job->clip.LL.x = job->focus.x - job->view.x / 2.;
        job->clip.LL.y = job->focus.y - job->view.y / 2.;
        job->clip.UR.x = job->focus.x + job->view.x / 2.;
        job->clip.UR.y = job->focus.y + job->view.y / 2.;
    }
    else {
        job->clip.LL.x = job->focus.x + job->pageSize.x * (pagesArrayElem.x - pagesArraySize.x / 2.);
        job->clip.LL.y = job->focus.y + job->pageSize.y * (pagesArrayElem.y - pagesArraySize.y / 2.);
        job->clip.UR.x = job->clip.LL.x + job->pageSize.x;
        job->clip.UR.y = job->clip.LL.y + job->pageSize.y;
    }

    /* CAUTION - job->translation was difficult to get right. */
    // Test with and without asymmetric margins, e.g: -Gmargin="1,0"
    if (job->rotation) {
	job->translation.y = - job->clip.UR.y - job->canvasBox.LL.y / job->zoom;
        if ((job->flags & GVRENDER_Y_GOES_DOWN) || Y_invert)
            job->translation.x = - job->clip.UR.x - job->canvasBox.LL.x / job->zoom;
        else
            job->translation.x = - job->clip.LL.x + job->canvasBox.LL.x / job->zoom;
    }
    else {
	/* pre unscale margins to keep them constant under scaling */
        job->translation.x = - job->clip.LL.x + job->canvasBox.LL.x / job->zoom;
        if ((job->flags & GVRENDER_Y_GOES_DOWN) || Y_invert)
            job->translation.y = - job->clip.UR.y - job->canvasBox.LL.y / job->zoom;
        else
            job->translation.y = - job->clip.LL.y + job->canvasBox.LL.y / job->zoom;
    }
}

static bool node_in_layer(GVJ_t *job, graph_t * g, node_t * n)
{
    char *pn, *pe;
    edge_t *e;

    if (job->numLayers <= 1)
	return true;
    pn = late_string(n, N_layer, "");
    if (selectedlayer(job, pn))
	return true;
    if (pn[0])
	return false;		/* Only check edges if pn = "" */
    if ((e = agfstedge(g, n)) == NULL)
	return true;
    for (e = agfstedge(g, n); e; e = agnxtedge(g, e, n)) {
	pe = late_string(e, E_layer, "");
	if (pe[0] == '\0' || selectedlayer(job, pe))
	    return true;
    }
    return false;
}

static bool edge_in_layer(GVJ_t *job, edge_t * e)
{
    char *pe, *pn;
    int cnt;

    if (job->numLayers <= 1)
	return true;
    pe = late_string(e, E_layer, "");
    if (selectedlayer(job, pe))
	return true;
    if (pe[0])
	return false;
    for (cnt = 0; cnt < 2; cnt++) {
	pn = late_string(cnt < 1 ? agtail(e) : aghead(e), N_layer, "");
	if (pn[0] == '\0' || selectedlayer(job, pn))
	    return true;
    }
    return false;
}

static bool clust_in_layer(GVJ_t *job, graph_t * sg)
{
    char *pg;
    node_t *n;

    if (job->numLayers <= 1)
	return true;
    pg = late_string(sg, agattr(sg, AGRAPH, "layer", 0), "");
    if (selectedlayer(job, pg))
	return true;
    if (pg[0])
	return false;
    for (n = agfstnode(sg); n; n = agnxtnode(sg, n))
	if (node_in_layer(job, sg, n))
	    return true;
    return false;
}

static bool node_in_box(node_t *n, boxf b)
{
    return boxf_overlap(ND_bb(n), b) != 0;
}

static void emit_begin_node(GVJ_t * job, node_t * n)
{
    obj_state_t *obj;
    int flags = job->flags;
    int sides, shape;
    size_t nump = 0;
    polygon_t *poly = NULL;
    pointf *vertices, *p = NULL;
    pointf coord;
    char *s;

    obj = push_obj_state(job);
    obj->type = NODE_OBJTYPE;
    obj->u.n = n;
    obj->emit_state = EMIT_NDRAW;

    if (flags & GVRENDER_DOES_Z) {
	if (GD_odim(agraphof(n)) >=3)
            obj->z = POINTS(ND_pos(n)[2]);
	else
            obj->z = 0.0;
    }
    initObjMapData (job, ND_label(n), n);
    if ((flags & (GVRENDER_DOES_MAPS | GVRENDER_DOES_TOOLTIPS))
           && (obj->url || obj->explicit_tooltip)) {

        /* checking shape of node */
        shape = shapeOf(n);
        /* node coordinate */
        coord = ND_coord(n);
        /* checking if filled style has been set for node */
        bool filled = isFilled(n);

        bool is_rect = false;
        if (shape == SH_POLY || shape == SH_POINT) {
            poly = ND_shape_info(n);

            /* checking if polygon is regular rectangle */
            if (isRect(poly) && (poly->peripheries || filled))
                is_rect = true;
        }

        /* When node has polygon shape and requested output supports polygons
         * we use a polygon to map the clickable region that is a:
         * circle, ellipse, polygon with n side, or point.
         * For regular rectangular shape we have use node's bounding box to map clickable region
         */
        if (poly && !is_rect && (flags & GVRENDER_DOES_MAP_POLYGON)) {

            if (poly->sides < 3)
                sides = 1;
            else
                sides = poly->sides;

            int peripheries = poly->peripheries < 2 ? 1 : poly->peripheries;

            vertices = poly->vertices;

            int nump_int = 0;
            if ((s = agget(n, "samplepoints")))
                nump_int = atoi(s);
            /* We want at least 4 points. For server-side maps, at most 100
             * points are allowed. To simplify things to fit with the 120 points
             * used for skewed ellipses, we set the bound at 60.
             */
            nump = (nump_int < 4 || nump_int > 60) ? DFLT_SAMPLE : (size_t)nump_int;
            /* use bounding box of text label or node image for mapping
             * when polygon has no peripheries and node is not filled
             */
            if (poly->peripheries == 0 && !filled) {
                obj->url_map_shape = MAP_RECTANGLE;
                nump = 2;
                p = gv_calloc(nump, sizeof(pointf));
                P2RECT(coord, p, ND_lw(n), ND_ht(n) / 2.0 );
            }
            /* circle or ellipse */
            else if (poly->sides < 3 && poly->skew == 0.0 && poly->distortion == 0.0) {
                if (poly->regular) {
                    obj->url_map_shape = MAP_CIRCLE;
                    nump = 2;              /* center of circle and top right corner of bb */
                    p = gv_calloc(nump, sizeof(pointf));
                    p[0].x = coord.x;
                    p[0].y = coord.y;
		    /* even vertices contain LL corner of bb */
		    /* odd vertices contain UR corner of bb */
                    p[1].x = coord.x + vertices[2*peripheries - 1].x;
                    p[1].y = coord.y + vertices[2*peripheries - 1].y;
                }
                else { /* ellipse is treated as polygon */
                    obj->url_map_shape= MAP_POLYGON;
                    p = pEllipse(vertices[2 * peripheries - 1].x,
                                 vertices[2 * peripheries - 1].y, nump);
                    for (size_t i = 0; i < nump; i++) {
                        p[i].x += coord.x;
                        p[i].y += coord.y;
                    }
		}
            }
            /* all other polygonal shape */
            else {
                assert(peripheries >= 1);
                size_t offset = (size_t)((peripheries - 1) * poly->sides);
                obj->url_map_shape = MAP_POLYGON;
                /* distorted or skewed ellipses and circles are polygons with 120
                 * sides. For mapping we convert them into polygon with sample sides
                 */
                if (poly->sides >= 0 && (size_t)poly->sides >= nump) {
                    assert(poly->sides >= 0);
                    size_t delta = (size_t)poly->sides / nump;
                    p = gv_calloc(nump, sizeof(pointf));
                    for (size_t i = 0, j = 0; j < nump; i += delta, j++) {
                        p[j].x = coord.x + vertices[i + offset].x;
                        p[j].y = coord.y + vertices[i + offset].y;
                    }
                } else {
                    assert(sides >= 0);
                    nump = (size_t)sides;
                    p = gv_calloc(nump, sizeof(pointf));
                    for (size_t i = 0; i < nump; i++) {
                        p[i].x = coord.x + vertices[i + offset].x;
                        p[i].y = coord.y + vertices[i + offset].y;
                    }
                }
            }
        }
        else {
            /* we have to use the node's bounding box to map clickable region
             * when requested output format is not capable of polygons.
             */
            obj->url_map_shape = MAP_RECTANGLE;
            nump = 2;
            p = gv_calloc(nump, sizeof(pointf));
            p[0].x = coord.x - ND_lw(n);
            p[0].y = coord.y - (ND_ht(n) / 2);
            p[1].x = coord.x + ND_rw(n);
            p[1].y = coord.y + (ND_ht(n) / 2);
        }
        if (! (flags & GVRENDER_DOES_TRANSFORM))
            gvrender_ptf_A(job, p, p, nump);
        obj->url_map_p = p;
        obj->url_map_n = nump;
    }

    setColorScheme (agget (n, "colorscheme"));
    gvrender_begin_node(job);
}

static void emit_end_node(GVJ_t * job)
{
    gvrender_end_node(job);
    pop_obj_state(job);
}

static void emit_node(GVJ_t * job, node_t * n)
{
    GVC_t *gvc = job->gvc;
    char *s;
    char *style;
    char **styles = NULL;
    char **sp;
    char *p;

    if (ND_shape(n) 				     /* node has a shape */
	    && node_in_layer(job, agraphof(n), n)    /* and is in layer */
	    && node_in_box(n, job->clip)             /* and is in page/view */
	    && ND_state(n) != gvc->common.viewNum) /* and not already drawn */
    {
	ND_state(n) = gvc->common.viewNum; 	     /* mark node as drawn */

        gvrender_comment(job, agnameof(n));
	s = late_string(n, N_comment, "");
	if (s[0])
	    gvrender_comment(job, s);
        
	style = late_string(n, N_style, "");
	if (style[0]) {
	    styles = parse_style(style);
	    sp = styles;
	    while ((p = *sp++)) {
		if (streq(p, "invis")) return;
	    }
	}

	emit_begin_node(job, n);
	ND_shape(n)->fns->codefn(job, n);
	if (ND_xlabel(n) && ND_xlabel(n)->set)
	    emit_label(job, EMIT_NLABEL, ND_xlabel(n));
	emit_end_node(job);
    }
}

/* calculate an offset vector, length d, perpendicular to line p,q */
static pointf computeoffset_p(pointf p, pointf q, double d)
{
    pointf res;
    double x = p.x - q.x, y = p.y - q.y;

    /* keep d finite as line length approaches 0 */
    d /= sqrt(x * x + y * y + EPSILON);
    res.x = y * d;
    res.y = -x * d;
    return res;
}

/* calculate offset vector, length d, perpendicular to spline p,q,r,s at q&r */
static pointf computeoffset_qr(pointf p, pointf q, pointf r, pointf s,
			       double d)
{
    pointf res;
    double len;
    double x = q.x - r.x, y = q.y - r.y;

    len = hypot(x, y);
    if (len < EPSILON) {
	/* control points are on top of each other
	   use slope between endpoints instead */
	x = p.x - s.x, y = p.y - s.y;
	/* keep d finite as line length approaches 0 */
	len = sqrt(x * x + y * y + EPSILON);
    }
    d /= len;
    res.x = y * d;
    res.y = -x * d;
    return res;
}

static void emit_attachment(GVJ_t * job, textlabel_t * lp, splines * spl)
{
    pointf sz, AF[3];
    const char *s;

    for (s = lp->text; *s; s++) {
	if (!isspace((int)*s))
	    break;
    }
    if (*s == '\0')
	return;

    sz = lp->dimen;
    AF[0] = (pointf){lp->pos.x + sz.x / 2., lp->pos.y - sz.y / 2.};
    AF[1] = (pointf){AF[0].x - sz.x, AF[0].y};
    AF[2] = dotneato_closest(spl, lp->pos);
    /* Don't use edge style to draw attachment */
    gvrender_set_style(job, job->gvc->defaultlinestyle);
    /* Use font color to draw attachment
       - need something unambiguous in case of multicolored parallel edges
       - defaults to black for html-like labels
     */
    gvrender_set_pencolor(job, lp->fontcolor);
    gvrender_polyline(job, AF, 3);
}

/* edges colors can be multiple colors separated by ":"
 * so we commpute a default pencolor with the same number of colors. */
static char* default_pencolor(char *pencolor, char *deflt)
{
    static agxbuf buf;
    char *p;
    size_t ncol = 1;
    for (p = pencolor; *p; p++) {
	if (*p == ':')
	    ncol++;
    }
    agxbput(&buf, deflt);
    while(--ncol) {
	agxbprint(&buf, ":%s", deflt);
    }
    return agxbuse(&buf);
}

/* approxLen:
 */
static double approxLen (pointf* pts)
{
    double d = DIST(pts[0],pts[1]);
    d += DIST(pts[1],pts[2]);
    d += DIST(pts[2],pts[3]);
    return d;
}
 
/* splitBSpline:
 * Given B-spline bz and 0 < t < 1, split bz so that left corresponds to
 * the fraction t of the arc length. The new parts are store in left and right.
 * The caller needs to free the allocated points.
 *
 * In the current implementation, we find the Bezier that should contain t by
 * treating the control points as a polyline.
 * We then split that Bezier.
 */
static void splitBSpline (bezier* bz, float t, bezier* left, bezier* right)
{
    int i, j, k, cnt = (bz->size - 1)/3;
    double last, len, sum;
    pointf* pts;
    float r;

    if (cnt == 1) {
	left->size = 4;
	left->list = gv_calloc(4, sizeof(pointf));
	right->size = 4;
	right->list = gv_calloc(4, sizeof(pointf));
	Bezier (bz->list, 3, t, left->list, right->list);
	return;
    }
    
    double* lens = gv_calloc(cnt, sizeof(double));
    sum = 0;
    pts = bz->list;
    for (i = 0; i < cnt; i++) {
	lens[i] = approxLen (pts);
	sum += lens[i];
	pts += 3;
    }
    len = t*sum;
    sum = 0;
    for (i = 0; i < cnt; i++) {
	sum += lens[i];
	if (sum >= len)
	    break;
    }

    left->size = 3*(i+1) + 1;
    left->list = gv_calloc(left->size, sizeof(pointf));
    right->size = 3*(cnt-i) + 1;
    right->list = gv_calloc(right->size, sizeof(pointf));
    for (j = 0; j < left->size; j++)
	left->list[j] = bz->list[j];
    k = j - 4;
    for (j = 0; j < right->size; j++)
	right->list[j] = bz->list[k++];

    last = lens[i];
    r = (len - (sum - last))/last;
    Bezier (bz->list + 3*i, 3, r, left->list + 3*i, right->list);

    free (lens);
}

/* multicolor:
 * Draw an edge as a sequence of colors.
 * Not sure how to handle multiple B-splines, so do a naive
 * implementation.
 * Return non-zero if color spec is incorrect
 */
static int multicolor (GVJ_t * job, edge_t * e, char** styles, char* colors, int num, double arrowsize, double penwidth)
{ 
    bezier bz;
    bezier bz0, bz_l, bz_r;
    int i, rv;
    colorsegs_t segs;
    char* endcolor = NULL;
    double left;
    int first;  /* first segment with t > 0 */

    rv = parseSegs (colors, num, &segs);
    if (rv > 1) {
	Agraph_t* g = agraphof(agtail(e));
	agerr (AGPREV, "in edge %s%s%s\n", agnameof(agtail(e)), (agisdirected(g)?" -> ":" -- "), agnameof(aghead(e)));

	if (rv == 2)
	    return 1;
    }
    else if (rv == 1)
	return 1;
    

    for (i = 0; i < ED_spl(e)->size; i++) {
	left = 1;
	bz = ED_spl(e)->list[i];
	first = 1;
	for (colorseg_t *s = segs.segs; s->color; s++) {
	    if (AEQ0(s->t)) continue;
    	    gvrender_set_pencolor(job, s->color);
	    left -= s->t;
	    endcolor = s->color;
	    if (first) {
		first = 0;
		splitBSpline (&bz, s->t, &bz_l, &bz_r);
		gvrender_beziercurve(job, bz_l.list, bz_l.size, 0);
		free (bz_l.list);
		if (AEQ0(left)) {
		    free (bz_r.list);
		    break;
		}
	    }
	    else if (AEQ0(left)) {
		gvrender_beziercurve(job, bz_r.list, bz_r.size, 0);
		free (bz_r.list);
		break;
	    }
	    else {
		bz0 = bz_r;
		splitBSpline(&bz0, s->t / (left + s->t), &bz_l, &bz_r);
		free (bz0.list);
		gvrender_beziercurve(job, bz_l.list, bz_l.size, 0);
		free (bz_l.list);
	    }
		
	}
                /* arrow_gen resets the job style  (How?  FIXME)
                 * If we have more splines to do, restore the old one.
                 * Use local copy of penwidth to work around reset.
                 */
	if (bz.sflag) {
    	    gvrender_set_pencolor(job, segs.segs->color);
    	    gvrender_set_fillcolor(job, segs.segs->color);
	    arrow_gen(job, EMIT_TDRAW, bz.sp, bz.list[0], arrowsize, penwidth, bz.sflag);
	}
	if (bz.eflag) {
    	    gvrender_set_pencolor(job, endcolor);
    	    gvrender_set_fillcolor(job, endcolor);
	    arrow_gen(job, EMIT_HDRAW, bz.ep, bz.list[bz.size - 1], arrowsize, penwidth, bz.eflag);
	}
	if (ED_spl(e)->size > 1 && (bz.sflag || bz.eflag) && styles)
	    gvrender_set_style(job, styles);
    }
    freeSegs(&segs);
    return 0;
}

static void free_stroke(stroke_t sp) {
  free(sp.vertices);
}

typedef double (*radfunc_t)(double,double,double);

static double forfunc (double curlen, double totallen, double initwid)
{
    return (1 - curlen / totallen) * initwid / 2.0;
}

static double revfunc (double curlen, double totallen, double initwid)
{
    return curlen / totallen * initwid / 2.0;
}

static double nonefunc (double curlen, double totallen, double initwid)
{
    (void)curlen;
    (void)totallen;

    return initwid / 2.0;
}

static double bothfunc (double curlen, double totallen, double initwid)
{
    double fr = curlen/totallen;
    if (fr <= 0.5) return fr * initwid;
    return (1 - fr) * initwid;
}

static radfunc_t 
taperfun (edge_t* e)
{
    char* attr;
    if (E_dir && ((attr = agxget(e, E_dir)))[0]) {
	if (streq(attr, "forward")) return forfunc;
	if (streq(attr, "back")) return revfunc;
	if (streq(attr, "both")) return bothfunc;
	if (streq(attr, "none")) return nonefunc;
    }
    return agisdirected(agraphof(aghead(e))) ? forfunc : nonefunc;
}

static void emit_edge_graphics(GVJ_t * job, edge_t * e, char** styles)
{
    int i, j, cnum, numc = 0, numsemi = 0;
    char *color, *pencolor, *fillcolor;
    char *headcolor, *tailcolor, *lastcolor;
    char *colors = NULL;
    bezier bz;
    splines offspl, tmpspl;
    pointf pf0, pf1, pf2 = { 0, 0 }, pf3, *offlist, *tmplist;
    double arrowsize, numc2, penwidth=job->obj->penwidth;
    char* p;
    bool tapered = false;

#define SEP 2.0

    setColorScheme (agget (e, "colorscheme"));
    if (ED_spl(e)) {
	arrowsize = late_double(e, E_arrowsz, 1.0, 0.0);
	color = late_string(e, E_color, "");

	if (styles) {
	    char** sp = styles;
	    while ((p = *sp++)) {
		if (streq(p, "tapered")) {
		    tapered = true;
		    break;
		}
	    }
	}

	/* need to know how many colors separated by ':' */
	for (p = color; *p; p++) {
	    if (*p == ':')
		numc++;
	    else if (*p == ';')
		numsemi++;
	}

	if (numsemi && numc) {
	    if (multicolor (job, e, styles, color, numc+1, arrowsize, penwidth)) {
		color = DEFAULT_COLOR;
	    }
	    else
		return;
	}

	fillcolor = pencolor = color;
	if (ED_gui_state(e) & GUI_STATE_ACTIVE) {
	    pencolor = late_nnstring(e, E_activepencolor,
			default_pencolor(pencolor, DEFAULT_ACTIVEPENCOLOR));
	    fillcolor = late_nnstring(e, E_activefillcolor, DEFAULT_ACTIVEFILLCOLOR);
	}
	else if (ED_gui_state(e) & GUI_STATE_SELECTED) {
	    pencolor = late_nnstring(e, E_selectedpencolor,
			default_pencolor(pencolor, DEFAULT_SELECTEDPENCOLOR));
	    fillcolor = late_nnstring(e, E_selectedfillcolor, DEFAULT_SELECTEDFILLCOLOR);
	}
	else if (ED_gui_state(e) & GUI_STATE_DELETED) {
	    pencolor = late_nnstring(e, E_deletedpencolor,
			default_pencolor(pencolor, DEFAULT_DELETEDPENCOLOR));
	    fillcolor = late_nnstring(e, E_deletedfillcolor, DEFAULT_DELETEDFILLCOLOR);
	}
	else if (ED_gui_state(e) & GUI_STATE_VISITED) {
	    pencolor = late_nnstring(e, E_visitedpencolor,
			default_pencolor(pencolor, DEFAULT_VISITEDPENCOLOR));
	    fillcolor = late_nnstring(e, E_visitedfillcolor, DEFAULT_VISITEDFILLCOLOR);
	}
	else
	    fillcolor = late_nnstring(e, E_fillcolor, color);
	if (pencolor != color)
    	    gvrender_set_pencolor(job, pencolor);
	if (fillcolor != color)
	    gvrender_set_fillcolor(job, fillcolor);
	color = pencolor;

	if (tapered) {
	    if (*color == '\0') color = DEFAULT_COLOR;
	    if (*fillcolor == '\0') fillcolor = DEFAULT_COLOR;
    	    gvrender_set_pencolor(job, "transparent");
	    gvrender_set_fillcolor(job, color);
	    bz = ED_spl(e)->list[0];
	    stroke_t stp = taper(&bz, taperfun (e), penwidth);
	    assert(stp.nvertices <= INT_MAX);
	    gvrender_polygon(job, stp.vertices, (int)stp.nvertices, TRUE);
	    free_stroke(stp);
    	    gvrender_set_pencolor(job, color);
	    if (fillcolor != color)
		gvrender_set_fillcolor(job, fillcolor);
	    if (bz.sflag) {
		arrow_gen(job, EMIT_TDRAW, bz.sp, bz.list[0], arrowsize, penwidth, bz.sflag);
	    }
	    if (bz.eflag) {
		arrow_gen(job, EMIT_HDRAW, bz.ep, bz.list[bz.size - 1], arrowsize, penwidth, bz.eflag);
	    }
	}
	/* if more than one color - then generate parallel beziers, one per color */
	else if (numc) {
	    /* calculate and save offset vector spline and initialize first offset spline */
	    tmpspl.size = offspl.size = ED_spl(e)->size;
	    offspl.list = gv_calloc(offspl.size, sizeof(bezier));
	    tmpspl.list = gv_calloc(tmpspl.size, sizeof(bezier));
	    numc2 = (2 + numc) / 2.0;
	    for (i = 0; i < offspl.size; i++) {
		bz = ED_spl(e)->list[i];
		tmpspl.list[i].size = offspl.list[i].size = bz.size;
		offlist = offspl.list[i].list = gv_calloc(bz.size, sizeof(pointf));
		tmplist = tmpspl.list[i].list = gv_calloc(bz.size, sizeof(pointf));
		pf3 = bz.list[0];
		for (j = 0; j < bz.size - 1; j += 3) {
		    pf0 = pf3;
		    pf1 = bz.list[j + 1];
		    /* calculate perpendicular vectors for each bezier point */
		    if (j == 0)	/* first segment, no previous pf2 */
			offlist[j] = computeoffset_p(pf0, pf1, SEP);
		    else	/* i.e. pf2 is available from previous segment */
			offlist[j] = computeoffset_p(pf2, pf1, SEP);
		    pf2 = bz.list[j + 2];
		    pf3 = bz.list[j + 3];
		    offlist[j + 1] = offlist[j + 2] =
			computeoffset_qr(pf0, pf1, pf2, pf3, SEP);
		    /* initialize tmpspl to outermost position */
		    tmplist[j].x = pf0.x - numc2 * offlist[j].x;
		    tmplist[j].y = pf0.y - numc2 * offlist[j].y;
		    tmplist[j + 1].x = pf1.x - numc2 * offlist[j + 1].x;
		    tmplist[j + 1].y = pf1.y - numc2 * offlist[j + 1].y;
		    tmplist[j + 2].x = pf2.x - numc2 * offlist[j + 2].x;
		    tmplist[j + 2].y = pf2.y - numc2 * offlist[j + 2].y;
		}
		/* last segment, no next pf1 */
		offlist[j] = computeoffset_p(pf2, pf3, SEP);
		tmplist[j].x = pf3.x - numc2 * offlist[j].x;
		tmplist[j].y = pf3.y - numc2 * offlist[j].y;
	    }
	    lastcolor = headcolor = tailcolor = color;
	    colors = gv_strdup(color);
	    for (cnum = 0, color = strtok(colors, ":"); color;
		cnum++, color = strtok(0, ":")) {
		if (!color[0])
		    color = DEFAULT_COLOR;
		if (color != lastcolor) {
	            if (! (ED_gui_state(e) & (GUI_STATE_ACTIVE | GUI_STATE_SELECTED))) {
		        gvrender_set_pencolor(job, color);
		        gvrender_set_fillcolor(job, color);
		    }
		    lastcolor = color;
		}
		if (cnum == 0)
		    headcolor = tailcolor = color;
		if (cnum == 1)
		    tailcolor = color;
		for (i = 0; i < tmpspl.size; i++) {
		    tmplist = tmpspl.list[i].list;
		    offlist = offspl.list[i].list;
		    for (j = 0; j < tmpspl.list[i].size; j++) {
			tmplist[j].x += offlist[j].x;
			tmplist[j].y += offlist[j].y;
		    }
		    gvrender_beziercurve(job, tmplist, tmpspl.list[i].size, 0);
		}
	    }
	    if (bz.sflag) {
		if (color != tailcolor) {
		    color = tailcolor;
	            if (! (ED_gui_state(e) & (GUI_STATE_ACTIVE | GUI_STATE_SELECTED))) {
		        gvrender_set_pencolor(job, color);
		        gvrender_set_fillcolor(job, color);
		    }
		}
		arrow_gen(job, EMIT_TDRAW, bz.sp, bz.list[0],
			arrowsize, penwidth, bz.sflag);
	    }
	    if (bz.eflag) {
		if (color != headcolor) {
		    color = headcolor;
	            if (! (ED_gui_state(e) & (GUI_STATE_ACTIVE | GUI_STATE_SELECTED))) {
		        gvrender_set_pencolor(job, color);
		        gvrender_set_fillcolor(job, color);
		    }
		}
		arrow_gen(job, EMIT_HDRAW, bz.ep, bz.list[bz.size - 1],
			arrowsize, penwidth, bz.eflag);
	    }
	    free(colors);
	    for (i = 0; i < offspl.size; i++) {
		free(offspl.list[i].list);
		free(tmpspl.list[i].list);
	    }
	    free(offspl.list);
	    free(tmpspl.list);
	} else {
	    if (! (ED_gui_state(e) & (GUI_STATE_ACTIVE | GUI_STATE_SELECTED))) {
	        if (color[0]) {
		    gvrender_set_pencolor(job, color);
		    gvrender_set_fillcolor(job, fillcolor);
	        } else {
		    gvrender_set_pencolor(job, DEFAULT_COLOR);
		    if (fillcolor[0])
			gvrender_set_fillcolor(job, fillcolor);
		    else
			gvrender_set_fillcolor(job, DEFAULT_COLOR);
	        }
	    }
	    for (i = 0; i < ED_spl(e)->size; i++) {
		bz = ED_spl(e)->list[i];
		gvrender_beziercurve(job, bz.list, bz.size, 0);
		if (bz.sflag) {
		    arrow_gen(job, EMIT_TDRAW, bz.sp, bz.list[0],
		              arrowsize, penwidth, bz.sflag);
		}
		if (bz.eflag) {
		    arrow_gen(job, EMIT_HDRAW, bz.ep, bz.list[bz.size - 1],
		              arrowsize, penwidth, bz.eflag);
		}
		if (ED_spl(e)->size > 1 && (bz.sflag || bz.eflag) && styles)
		    gvrender_set_style(job, styles);
		}
	}
    }
}

static bool edge_in_box(edge_t *e, boxf b)
{
    splines *spl;
    textlabel_t *lp;

    spl = ED_spl(e);
    if (spl && boxf_overlap(spl->bb, b))
        return true;

    lp = ED_label(e);
    if (lp && overlap_label(lp, b))
        return true;

    lp = ED_xlabel(e);
    if (lp && lp->set && overlap_label(lp, b))
        return true;

    return false;
}

static void emit_begin_edge(GVJ_t *job, edge_t *e, char **styles) {
  obj_state_t *obj;
  int flags = job->flags;
  char *s;
  textlabel_t *lab = NULL, *tlab = NULL, *hlab = NULL;
  pointf *pbs = NULL;
  int i, *pbs_n = NULL, pbs_poly_n = 0;
  char *dflt_url = NULL;
  char *dflt_target = NULL;
  double penwidth;

  obj = push_obj_state(job);
  obj->type = EDGE_OBJTYPE;
  obj->u.e = e;
  obj->emit_state = EMIT_EDRAW;
  if (ED_label(e) && !ED_label(e)->html &&
      mapBool(agget(e, "labelaligned"), false))
    obj->labeledgealigned = true;

  /* We handle the edge style and penwidth here because the width
   * is needed below for calculating polygonal image maps
   */
  if (styles && ED_spl(e))
    gvrender_set_style(job, styles);

  if (E_penwidth && (s = agxget(e, E_penwidth)) && s[0]) {
    penwidth = late_double(e, E_penwidth, 1.0, 0.0);
    gvrender_set_penwidth(job, penwidth);
  }

  if (flags & GVRENDER_DOES_Z) {
    if (GD_odim(agraphof(agtail(e))) >= 3) {
      obj->tail_z = POINTS(ND_pos(agtail(e))[2]);
      obj->head_z = POINTS(ND_pos(aghead(e))[2]);
    } else {
      obj->tail_z = obj->head_z = 0.0;
    }
  }

  if (flags & GVRENDER_DOES_LABELS) {
    if ((lab = ED_label(e)))
      obj->label = lab->text;
    obj->taillabel = obj->headlabel = obj->xlabel = obj->label;
    if ((tlab = ED_xlabel(e)))
      obj->xlabel = tlab->text;
    if ((tlab = ED_tail_label(e)))
      obj->taillabel = tlab->text;
    if ((hlab = ED_head_label(e)))
      obj->headlabel = hlab->text;
  }

  if (flags & GVRENDER_DOES_MAPS) {
    agxbuf xb = {0};

    s = getObjId(job, e, &xb);
    obj->id = strdup_and_subst_obj(s, e);
    agxbfree(&xb);

    if (((s = agget(e, "href")) && s[0]) || ((s = agget(e, "URL")) && s[0]))
      dflt_url = strdup_and_subst_obj(s, e);
    if (((s = agget(e, "edgehref")) && s[0]) ||
        ((s = agget(e, "edgeURL")) && s[0]))
      obj->url = strdup_and_subst_obj(s, e);
    else if (dflt_url)
      obj->url = gv_strdup(dflt_url);
    if (((s = agget(e, "labelhref")) && s[0]) ||
        ((s = agget(e, "labelURL")) && s[0]))
      obj->labelurl = strdup_and_subst_obj(s, e);
    else if (dflt_url)
      obj->labelurl = gv_strdup(dflt_url);
    if (((s = agget(e, "tailhref")) && s[0]) ||
        ((s = agget(e, "tailURL")) && s[0])) {
      obj->tailurl = strdup_and_subst_obj(s, e);
      obj->explicit_tailurl = true;
    } else if (dflt_url)
      obj->tailurl = gv_strdup(dflt_url);
    if (((s = agget(e, "headhref")) && s[0]) ||
        ((s = agget(e, "headURL")) && s[0])) {
      obj->headurl = strdup_and_subst_obj(s, e);
      obj->explicit_headurl = true;
    } else if (dflt_url)
      obj->headurl = gv_strdup(dflt_url);
  }

  if (flags & GVRENDER_DOES_TARGETS) {
    if ((s = agget(e, "target")) && s[0])
      dflt_target = strdup_and_subst_obj(s, e);
    if ((s = agget(e, "edgetarget")) && s[0]) {
      obj->explicit_edgetarget = true;
      obj->target = strdup_and_subst_obj(s, e);
    } else if (dflt_target)
      obj->target = gv_strdup(dflt_target);
    if ((s = agget(e, "labeltarget")) && s[0])
      obj->labeltarget = strdup_and_subst_obj(s, e);
    else if (dflt_target)
      obj->labeltarget = gv_strdup(dflt_target);
    if ((s = agget(e, "tailtarget")) && s[0]) {
      obj->tailtarget = strdup_and_subst_obj(s, e);
      obj->explicit_tailtarget = true;
    } else if (dflt_target)
      obj->tailtarget = gv_strdup(dflt_target);
    if ((s = agget(e, "headtarget")) && s[0]) {
      obj->explicit_headtarget = true;
      obj->headtarget = strdup_and_subst_obj(s, e);
    } else if (dflt_target)
      obj->headtarget = gv_strdup(dflt_target);
  }

  if (flags & GVRENDER_DOES_TOOLTIPS) {
    if (((s = agget(e, "tooltip")) && s[0]) ||
        ((s = agget(e, "edgetooltip")) && s[0])) {
      char *tooltip = preprocessTooltip(s, e);
      obj->tooltip = strdup_and_subst_obj(tooltip, e);
      free(tooltip);
      obj->explicit_tooltip = true;
    } else if (obj->label)
      obj->tooltip = gv_strdup(obj->label);

    if ((s = agget(e, "labeltooltip")) && s[0]) {
      char *tooltip = preprocessTooltip(s, e);
      obj->labeltooltip = strdup_and_subst_obj(tooltip, e);
      free(tooltip);
      obj->explicit_labeltooltip = true;
    } else if (obj->label)
      obj->labeltooltip = gv_strdup(obj->label);

    if ((s = agget(e, "tailtooltip")) && s[0]) {
      char *tooltip = preprocessTooltip(s, e);
      obj->tailtooltip = strdup_and_subst_obj(tooltip, e);
      free(tooltip);
      obj->explicit_tailtooltip = true;
    } else if (obj->taillabel)
      obj->tailtooltip = gv_strdup(obj->taillabel);

    if ((s = agget(e, "headtooltip")) && s[0]) {
      char *tooltip = preprocessTooltip(s, e);
      obj->headtooltip = strdup_and_subst_obj(tooltip, e);
      free(tooltip);
      obj->explicit_headtooltip = true;
    } else if (obj->headlabel)
      obj->headtooltip = gv_strdup(obj->headlabel);
  }

  free(dflt_url);
  free(dflt_target);

  if (flags & (GVRENDER_DOES_MAPS | GVRENDER_DOES_TOOLTIPS)) {
    if (ED_spl(e) && (obj->url || obj->tooltip) &&
        (flags & GVRENDER_DOES_MAP_POLYGON)) {
      int ns;
      splines *spl;
      double w2 = MAX(job->obj->penwidth / 2.0, 2.0);

      spl = ED_spl(e);
      ns = spl->size; /* number of splines */
      for (i = 0; i < ns; i++)
        map_output_bspline(&pbs, &pbs_n, &pbs_poly_n, spl->list + i, w2);
      obj->url_bsplinemap_poly_n = pbs_poly_n;
      obj->url_bsplinemap_n = pbs_n;
      if (!(flags & GVRENDER_DOES_TRANSFORM)) {
        size_t nump;
        for (nump = 0, i = 0; i < pbs_poly_n; i++) {
          assert(pbs_n[i] >= 0);
          nump += (size_t)pbs_n[i];
        }
        gvrender_ptf_A(job, pbs, pbs, nump);
      }
      obj->url_bsplinemap_p = pbs;
      obj->url_map_shape = MAP_POLYGON;
      obj->url_map_p = pbs;
      assert(pbs_n[0] >= 0);
      obj->url_map_n = (size_t)pbs_n[0];
    }
  }

  gvrender_begin_edge(job);
  if (obj->url || obj->explicit_tooltip)
    gvrender_begin_anchor(job, obj->url, obj->tooltip, obj->target, obj->id);
}

static void
emit_edge_label(GVJ_t* job, textlabel_t* lbl, emit_state_t lkind, int explicit,
    char* url, char* tooltip, char* target, char *id, splines* spl)
{
    int flags = job->flags;
    emit_state_t old_emit_state;
    char* newid;
    agxbuf xb = {0};
    char* type;

    if (lbl == NULL || !lbl->set) return;
    if (id) { /* non-NULL if needed */
	switch (lkind) {
	case EMIT_ELABEL :
	    type = "label";
	    break;
	case EMIT_HLABEL :
	    type = "headlabel";
	    break;
	case EMIT_TLABEL :
	    type = "taillabel";
	    break;
	default :
	    UNREACHABLE();
	}
	agxbprint(&xb, "%s-%s", id, type);
	newid = agxbuse(&xb);
    }
    else
	newid = NULL;
    old_emit_state = job->obj->emit_state;
    job->obj->emit_state = lkind;
    if ((url || explicit) && !(flags & EMIT_CLUSTERS_LAST)) {
	map_label(job, lbl);
	gvrender_begin_anchor(job, url, tooltip, target, newid);
    }
    emit_label(job, lkind, lbl);
    if (spl) emit_attachment(job, lbl, spl);
    if (url || explicit) {
	if (flags & EMIT_CLUSTERS_LAST) {
	    map_label(job, lbl);
	    gvrender_begin_anchor(job, url, tooltip, target, newid);
	}
	gvrender_end_anchor(job);
    }
    agxbfree(&xb);
    job->obj->emit_state = old_emit_state;
}

/* nodeIntersect:
 * Common logic for setting hot spots at the beginning and end of 
 * an edge.
 * If we are given a value (url, tooltip, target) explicitly set for
 * the head/tail, we use that. 
 * Otherwise, if we are given a value explicitly set for the edge,
 * we use that.
 * Otherwise, we use whatever the argument value is.
 * We also note whether or not the tooltip was explicitly set.
 * If the url is non-NULL or the tooltip was explicit, we set
 * a hot spot around point p.
 */
static void nodeIntersect(GVJ_t *job, pointf p, bool explicit_iurl, char *iurl,
                          bool explicit_itooltip) {
    obj_state_t *obj = job->obj;
    char* url;
    bool explicit;

    if (explicit_iurl) url = iurl;
    else url = obj->url;
    if (explicit_itooltip) {
	explicit = true;
    }
    else if (obj->explicit_tooltip) {
	explicit = true;
    }
    else {
	explicit = false;
    }

    if (url || explicit) {
	map_point(job, p);
    }
}

static void emit_end_edge(GVJ_t * job)
{
    obj_state_t *obj = job->obj;
    edge_t *e = obj->u.e;
    int i, nump;

    if (obj->url || obj->explicit_tooltip) {
	gvrender_end_anchor(job);
	if (obj->url_bsplinemap_poly_n) {
	    for ( nump = obj->url_bsplinemap_n[0], i = 1; i < obj->url_bsplinemap_poly_n; i++) {
		/* additional polygon maps around remaining bezier pieces */
		assert(obj->url_bsplinemap_n[i] >= 0);
		obj->url_map_n = (size_t)obj->url_bsplinemap_n[i];
		obj->url_map_p = &(obj->url_bsplinemap_p[nump]);
		gvrender_begin_anchor(job,
			obj->url, obj->tooltip, obj->target, obj->id);
		gvrender_end_anchor(job);
		nump += obj->url_bsplinemap_n[i];
	    }
	}
    }
    obj->url_map_n = 0;       /* null out copy so that it doesn't get freed twice */
    obj->url_map_p = NULL;

    if (ED_spl(e)) {
	pointf p;
	bezier bz;

	/* process intersection with tail node */
	bz = ED_spl(e)->list[0];
	if (bz.sflag) /* Arrow at start of splines */
	    p = bz.sp;
	else /* No arrow at start of splines */
	    p = bz.list[0];
	nodeIntersect(job, p, obj->explicit_tailurl != 0, obj->tailurl,
	              obj->explicit_tailtooltip != 0);
        
	/* process intersection with head node */
	bz = ED_spl(e)->list[ED_spl(e)->size - 1];
	if (bz.eflag) /* Arrow at end of splines */
	    p = bz.ep;
	else /* No arrow at end of splines */
	    p = bz.list[bz.size - 1];
	nodeIntersect(job, p, obj->explicit_headurl != 0, obj->headurl,
	              obj->explicit_headtooltip != 0);
    }

    emit_edge_label(job, ED_label(e), EMIT_ELABEL,
	obj->explicit_labeltooltip, 
	obj->labelurl, obj->labeltooltip, obj->labeltarget, obj->id, 
	((mapbool(late_string(e, E_decorate, "false")) && ED_spl(e)) ? ED_spl(e) : 0));
    emit_edge_label(job, ED_xlabel(e), EMIT_ELABEL,
	obj->explicit_labeltooltip, 
	obj->labelurl, obj->labeltooltip, obj->labeltarget, obj->id, 
	((mapbool(late_string(e, E_decorate, "false")) && ED_spl(e)) ? ED_spl(e) : 0));
    emit_edge_label(job, ED_head_label(e), EMIT_HLABEL, 
	obj->explicit_headtooltip,
	obj->headurl, obj->headtooltip, obj->headtarget, obj->id,
	0);
    emit_edge_label(job, ED_tail_label(e), EMIT_TLABEL, 
	obj->explicit_tailtooltip,
	obj->tailurl, obj->tailtooltip, obj->tailtarget, obj->id,
	0);

    gvrender_end_edge(job);
    pop_obj_state(job);
}

static void emit_edge(GVJ_t * job, edge_t * e)
{
    char *s;
    char *style;
    char **styles = NULL;
    char **sp;
    char *p;

    if (edge_in_box(e, job->clip) && edge_in_layer(job, e) ) {

	agxbuf edge = {0};
	agxbput(&edge, agnameof(agtail(e)));
	if (agisdirected(agraphof(aghead(e))))
	    agxbput(&edge, "->");
	else
	    agxbput(&edge, "--");
	agxbput(&edge, agnameof(aghead(e)));
	gvrender_comment(job, agxbuse(&edge));
	agxbfree(&edge);

	s = late_string(e, E_comment, "");
	if (s[0])
	    gvrender_comment(job, s);

	style = late_string(e, E_style, "");
	/* We shortcircuit drawing an invisible edge because the arrowhead
	 * code resets the style to solid, and most of the code generators
	 * (except PostScript) won't honor a previous style of invis.
	 */
	if (style[0]) {
	    styles = parse_style(style);
	    sp = styles;
	    while ((p = *sp++)) {
		if (streq(p, "invis")) return;
	    }
	}

	emit_begin_edge(job, e, styles);
	emit_edge_graphics (job, e, styles);
	emit_end_edge(job);
    }
}

static char adjust[] = {'l', 'n', 'r'};

static void
expandBB (boxf* bb, pointf p)
{
    bb->UR.x = fmax(bb->UR.x, p.x);
    bb->LL.x = fmin(bb->LL.x, p.x);
    bb->UR.y = fmax(bb->UR.y, p.y);
    bb->LL.y = fmin(bb->LL.y, p.y);
}

static boxf ptsBB(xdot_point *inpts, size_t numpts, boxf *bb) {
    boxf opbb;

    opbb.LL.x = opbb.UR.x = inpts->x;
    opbb.LL.y = opbb.UR.y = inpts->y;
    for (size_t i = 1; i < numpts; i++) {
	inpts++;
	if (inpts->x < opbb.LL.x)
	    opbb.LL.x = inpts->x;
	else if (inpts->x > opbb.UR.x)
	    opbb.UR.x = inpts->x;
	if (inpts->y < opbb.LL.y)
	    opbb.LL.y = inpts->y;
	else if (inpts->y > opbb.UR.y)
	    opbb.UR.y = inpts->y;

    }
    expandBB (bb, opbb.LL);
    expandBB (bb, opbb.UR);
    return opbb;
}

static boxf
textBB (double x, double y, textspan_t* span)
{
    boxf bb;
    pointf sz = span->size;

    switch (span->just) {
    case 'l':
	bb.LL.x = x;
	bb.UR.x = bb.LL.x + sz.x;
	break; 
    case 'n':
	bb.LL.x = x - sz.x / 2.0; 
	bb.UR.x = x + sz.x / 2.0; 
	break; 
    case 'r':
	bb.UR.x = x; 
	bb.LL.x = bb.UR.x - sz.x;
	break; 
    }
    bb.UR.y = y + span->yoffset_layout;
    bb.LL.y = bb.UR.y - sz.y;
    return bb;
}

static void
freePara (exdot_op* op)
{
    if (op->op.kind == xd_text)
	free_textspan (op->span, 1);
}

boxf xdotBB (Agraph_t* g)
{
    GVC_t *gvc = GD_gvc(g);
    exdot_op* op;
    double fontsize = 0.0;
    char* fontname = NULL;
    pointf pts[2];
    boxf bb0;
    boxf bb = GD_bb(g);
    xdot* xd = GD_drawing(g)->xdots;
    textfont_t tf, null_tf = {0};
    int fontflags = 0;

    if (!xd) return bb;

    if (bb.LL.x == bb.UR.x && bb.LL.y == bb.UR.y) {
	bb.LL.x = bb.LL.y = MAXDOUBLE;
	bb.UR.x = bb.UR.y = -MAXDOUBLE;
    }

    op = (exdot_op*)xd->ops;
    for (size_t i = 0; i < xd->cnt; i++) {
	tf = null_tf;
	switch (op->op.kind) {
	case xd_filled_ellipse :
	case xd_unfilled_ellipse :
	    pts[0].x = op->op.u.ellipse.x - op->op.u.ellipse.w;
	    pts[0].y = op->op.u.ellipse.y - op->op.u.ellipse.h;
	    pts[1].x = op->op.u.ellipse.x + op->op.u.ellipse.w;
	    pts[1].y = op->op.u.ellipse.y + op->op.u.ellipse.h;
	    op->bb.LL = pts[0];
	    op->bb.UR = pts[1];
	    expandBB (&bb, pts[0]);
	    expandBB (&bb, pts[1]);
	    break;
	case xd_filled_polygon :
	case xd_unfilled_polygon :
	    op->bb = ptsBB (op->op.u.polygon.pts, op->op.u.polygon.cnt, &bb);
	    break;
	case xd_filled_bezier :
	case xd_unfilled_bezier :
	    op->bb = ptsBB (op->op.u.polygon.pts, op->op.u.polygon.cnt, &bb);
	    break;
	case xd_polyline :
	    op->bb = ptsBB (op->op.u.polygon.pts, op->op.u.polygon.cnt, &bb);
	    break;
	case xd_text :
	    op->span = gv_alloc(sizeof(textspan_t));
	    op->span->str = gv_strdup (op->op.u.text.text);
	    op->span->just = adjust [op->op.u.text.align];
	    tf.name = fontname;
	    tf.size = fontsize;
	    tf.flags = fontflags;
            op->span->font = dtinsert(gvc->textfont_dt, &tf);
	    textspan_size (gvc, op->span);
	    bb0 = textBB (op->op.u.text.x, op->op.u.text.y, op->span);
	    op->bb = bb0;
	    expandBB (&bb, bb0.LL);
	    expandBB (&bb, bb0.UR);
	    if (!xd->freefunc)
		xd->freefunc = (freefunc_t)freePara;
	    break;
	case xd_font :
	    fontsize = op->op.u.font.size;
	    fontname = op->op.u.font.name;
	    break;
	case xd_fontchar :
	    fontflags = op->op.u.fontchar;
	    break;
	default :
	    break;
	}
	op++;
    }
    return bb;
}

static void init_gvc(GVC_t * gvc, graph_t * g)
{
    double xf, yf;
    char *p;
    int i;

    gvc->g = g;

    /* margins */
    gvc->graph_sets_margin = false;
    if ((p = agget(g, "margin"))) {
        i = sscanf(p, "%lf,%lf", &xf, &yf);
        if (i > 0) {
            gvc->margin.x = gvc->margin.y = xf * POINTS_PER_INCH;
            if (i > 1)
                gvc->margin.y = yf * POINTS_PER_INCH;
            gvc->graph_sets_margin = true;
        }
    }

    /* pad */
    gvc->graph_sets_pad = false;
    if ((p = agget(g, "pad"))) {
        i = sscanf(p, "%lf,%lf", &xf, &yf);
        if (i > 0) {
            gvc->pad.x = gvc->pad.y = xf * POINTS_PER_INCH;
            if (i > 1)
                gvc->pad.y = yf * POINTS_PER_INCH;
            gvc->graph_sets_pad = true;
        }
    }

    /* pagesize */
    gvc->graph_sets_pageSize = false;
    gvc->pageSize = GD_drawing(g)->page;
    if (GD_drawing(g)->page.x > 0.001 && GD_drawing(g)->page.y > 0.001)
        gvc->graph_sets_pageSize = true;

    /* rotation */
    if (GD_drawing(g)->landscape)
	gvc->rotation = 90;
    else 
	gvc->rotation = 0;

    /* pagedir */
    gvc->pagedir = "BL";
    if ((p = agget(g, "pagedir")) && p[0])
            gvc->pagedir = p;


    /* bounding box */
    gvc->bb = GD_bb(g);

    /* clusters have peripheries */
    G_peripheries = agfindgraphattr(g, "peripheries");
    G_penwidth = agfindgraphattr(g, "penwidth");

    /* default font */
    gvc->defaultfontname = late_nnstring(NULL,
                N_fontname, DEFAULT_FONTNAME);
    gvc->defaultfontsize = late_double(NULL,
                N_fontsize, DEFAULT_FONTSIZE, MIN_FONTSIZE);

    /* default line style */
    gvc->defaultlinestyle = defaultlinestyle;

    gvc->graphname = agnameof(g);
}

static void init_job_pad(GVJ_t *job)
{
    GVC_t *gvc = job->gvc;
    
    if (gvc->graph_sets_pad) {
	job->pad = gvc->pad;
    }
    else {
	switch (job->output_lang) {
	case GVRENDER_PLUGIN:
	    job->pad.x = job->pad.y = job->render.features->default_pad;
	    break;
	default:
	    job->pad.x = job->pad.y = DEFAULT_GRAPH_PAD;
	    break;
	}
    }
}

static void init_job_margin(GVJ_t *job)
{
    GVC_t *gvc = job->gvc;
    
    if (gvc->graph_sets_margin) {
	job->margin = gvc->margin;
    }
    else {
        /* set default margins depending on format */
        switch (job->output_lang) {
        case GVRENDER_PLUGIN:
            job->margin = job->device.features->default_margin;
            break;
        case PCL: case MIF: case METAPOST: case VTX: case QPDF:
            job->margin.x = job->margin.y = DEFAULT_PRINT_MARGIN;
            break;
        default:
            job->margin.x = job->margin.y = DEFAULT_EMBED_MARGIN;
            break;
        }
    }

}

static void init_job_dpi(GVJ_t *job, graph_t *g)
{
    GVJ_t *firstjob = job->gvc->active_jobs;

    if (GD_drawing(g)->dpi != 0) {
        job->dpi.x = job->dpi.y = (double)GD_drawing(g)->dpi;
    }
    else if (firstjob && firstjob->device_sets_dpi) {
        job->dpi = firstjob->device_dpi;   /* some devices set dpi in initialize() */
    }
    else {
        /* set default margins depending on format */
        switch (job->output_lang) {
        case GVRENDER_PLUGIN:
            job->dpi = job->device.features->default_dpi;
            break;
        default:
            job->dpi.x = job->dpi.y = (double)DEFAULT_DPI;
            break;
        }
    }
}

static void init_job_viewport(GVJ_t * job, graph_t * g)
{
    GVC_t *gvc = job->gvc;
    pointf LL, UR, size, sz;
    double X, Y, Z, x, y;
    int rv;
    Agnode_t *n;
    char *str, *nodename = NULL;

    UR = gvc->bb.UR;
    LL = gvc->bb.LL;
    job->bb.LL.x = LL.x - job->pad.x;           /* job->bb is bb of graph and padding - graph units */
    job->bb.LL.y = LL.y - job->pad.y;
    job->bb.UR.x = UR.x + job->pad.x;
    job->bb.UR.y = UR.y + job->pad.y;
    sz.x = job->bb.UR.x - job->bb.LL.x;   /* size, including padding - graph units */
    sz.y = job->bb.UR.y - job->bb.LL.y;

    /* determine final drawing size and scale to apply. */
    /* N.B. size given by user is not rotated by landscape mode */
    /* start with "natural" size of layout */

    Z = 1.0;
    if (GD_drawing(g)->size.x > 0.001 && GD_drawing(g)->size.y > 0.001) { /* graph size was given by user... */
	size = GD_drawing(g)->size;
	if (sz.x == 0) sz.x = size.x;
	if (sz.y == 0) sz.y = size.y;
	if (size.x < sz.x || size.y < sz.y /* drawing is too big (in either axis) ... */
	    || (GD_drawing(g)->filled /* or ratio=filled requested and ... */
		&& size.x > sz.x && size.y > sz.y)) /* drawing is too small (in both axes) ... */
	    Z = MIN(size.x/sz.x, size.y/sz.y);
    }
    
    /* default focus, in graph units = center of bb */
    x = (LL.x + UR.x) / 2.;
    y = (LL.y + UR.y) / 2.;

    /* rotate and scale bb to give default absolute size in points*/
    job->rotation = job->gvc->rotation;
    X = sz.x * Z;
    Y = sz.y * Z;

    /* user can override */
    if ((str = agget(g, "viewport"))) {
        nodename = gv_alloc(strlen(str) + 1);
	rv = sscanf(str, "%lf,%lf,%lf,\'%[^\']\'", &X, &Y, &Z, nodename);
	if (rv == 4) {
	    n = agfindnode(g->root, nodename);
	    if (n) {
		x = ND_coord(n).x;
		y = ND_coord(n).y;
	    }
	}
	else {
	    char junk;
	    rv = sscanf(str, "%lf,%lf,%lf,%[^,]%c", &X, &Y, &Z, nodename, &junk);
	    if (rv == 4) {
                n = agfindnode(g->root, nodename);
                if (n) {
                    x = ND_coord(n).x;
                    y = ND_coord(n).y;
		}
	    }
	    else {
	        rv = sscanf(str, "%lf,%lf,%lf,%lf,%lf", &X, &Y, &Z, &x, &y);
	    }
        }
	free (nodename);
    }
    /* rv is ignored since args retain previous values if not scanned */

    /* job->view gives port size in graph units, unscaled or rotated
     * job->zoom gives scaling factor.
     * job->focus gives the position in the graph of the center of the port
     */
    job->view.x = X;
    job->view.y = Y;
    job->zoom = Z;              /* scaling factor */
    job->focus.x = x;
    job->focus.y = y;
}

static void emit_cluster_colors(GVJ_t * job, graph_t * g)
{
    graph_t *sg;
    int c;
    char *str;

    for (c = 1; c <= GD_n_cluster(g); c++) {
	sg = GD_clust(g)[c];
	emit_cluster_colors(job, sg);
	if (((str = agget(sg, "color")) != 0) && str[0])
	    gvrender_set_pencolor(job, str);
	if (((str = agget(sg, "pencolor")) != 0) && str[0])
	    gvrender_set_pencolor(job, str);
	if (((str = agget(sg, "bgcolor")) != 0) && str[0])
	    gvrender_set_pencolor(job, str);
	if (((str = agget(sg, "fillcolor")) != 0) && str[0])
	    gvrender_set_fillcolor(job, str);
	if (((str = agget(sg, "fontcolor")) != 0) && str[0])
	    gvrender_set_pencolor(job, str);
    }
}

static void emit_colors(GVJ_t * job, graph_t * g)
{
    node_t *n;
    edge_t *e;
    char *str, *colors;

    gvrender_set_fillcolor(job, DEFAULT_FILL);
    if (((str = agget(g, "bgcolor")) != 0) && str[0])
	gvrender_set_fillcolor(job, str);
    if (((str = agget(g, "fontcolor")) != 0) && str[0])
	gvrender_set_pencolor(job, str);
  
    emit_cluster_colors(job, g);
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (((str = agget(n, "color")) != 0) && str[0])
	    gvrender_set_pencolor(job, str);
	if (((str = agget(n, "pencolor")) != 0) && str[0])
	    gvrender_set_fillcolor(job, str);
	if (((str = agget(n, "fillcolor")) != 0) && str[0]) {
	    if (strchr(str, ':')) {
		colors = gv_strdup(str);
		for (str = strtok(colors, ":"); str;
		    str = strtok(0, ":")) {
		    if (str[0])
			gvrender_set_pencolor(job, str);
		}
		free(colors);
	    }
	    else {
		gvrender_set_pencolor(job, str);
	    }
	}
	if (((str = agget(n, "fontcolor")) != 0) && str[0])
	    gvrender_set_pencolor(job, str);
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    if (((str = agget(e, "color")) != 0) && str[0]) {
		if (strchr(str, ':')) {
		    colors = gv_strdup(str);
		    for (str = strtok(colors, ":"); str;
			str = strtok(0, ":")) {
			if (str[0])
			    gvrender_set_pencolor(job, str);
		    }
		    free(colors);
		}
		else {
		    gvrender_set_pencolor(job, str);
		}
	    }
	    if (((str = agget(e, "fontcolor")) != 0) && str[0])
		gvrender_set_pencolor(job, str);
	}
    }
}

static void emit_view(GVJ_t * job, graph_t * g, int flags)
{
    GVC_t * gvc = job->gvc;
    node_t *n;
    edge_t *e;

    gvc->common.viewNum++;
    /* when drawing, lay clusters down before nodes and edges */
    if (!(flags & EMIT_CLUSTERS_LAST))
	emit_clusters(job, g, flags);
    if (flags & EMIT_SORTED) {
	/* output all nodes, then all edges */
	gvrender_begin_nodes(job);
	for (n = agfstnode(g); n; n = agnxtnode(g, n))
	    emit_node(job, n);
	gvrender_end_nodes(job);
	gvrender_begin_edges(job);
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    for (e = agfstout(g, n); e; e = agnxtout(g, e))
		emit_edge(job, e);
	}
	gvrender_end_edges(job);
    } else if (flags & EMIT_EDGE_SORTED) {
	/* output all edges, then all nodes */
	gvrender_begin_edges(job);
	for (n = agfstnode(g); n; n = agnxtnode(g, n))
	    for (e = agfstout(g, n); e; e = agnxtout(g, e))
		emit_edge(job, e);
	gvrender_end_edges(job);
	gvrender_begin_nodes(job);
	for (n = agfstnode(g); n; n = agnxtnode(g, n))
	    emit_node(job, n);
	gvrender_end_nodes(job);
    } else if (flags & EMIT_PREORDER) {
	gvrender_begin_nodes(job);
	for (n = agfstnode(g); n; n = agnxtnode(g, n))
	    if (write_node_test(g, n))
		emit_node(job, n);
	gvrender_end_nodes(job);
	gvrender_begin_edges(job);

	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
		if (write_edge_test(g, e))
		    emit_edge(job, e);
	    }
	}
	gvrender_end_edges(job);
    } else {
	/* output in breadth first graph walk order */
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    emit_node(job, n);
	    for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
		emit_node(job, aghead(e));
		emit_edge(job, e);
	    }
	}
    }
    /* when mapping, detect events on clusters after nodes and edges */
    if (flags & EMIT_CLUSTERS_LAST)
	emit_clusters(job, g, flags);
}

static void emit_begin_graph(GVJ_t * job, graph_t * g)
{
    obj_state_t *obj;

    obj = push_obj_state(job);
    obj->type = ROOTGRAPH_OBJTYPE;
    obj->u.g = g;
    obj->emit_state = EMIT_GDRAW;

    initObjMapData (job, GD_label(g), g);

    gvrender_begin_graph(job);
}

static void emit_end_graph(GVJ_t * job)
{
    gvrender_end_graph(job);
    pop_obj_state(job);
}

#define NotFirstPage(j) (((j)->layerNum>1)||((j)->pagesArrayElem.x > 0)||((j)->pagesArrayElem.x > 0))

static void emit_page(GVJ_t * job, graph_t * g)
{
    obj_state_t *obj = job->obj;
    int flags = job->flags;
    size_t nump = 0;
    textlabel_t *lab;
    pointf *p = NULL;
    char* saveid;
    agxbuf xb = {0};

    /* For the first page, we can use the values generated in emit_begin_graph. 
     * For multiple pages, we need to generate a new id.
     */
    if (NotFirstPage(job)) {
	saveid = obj->id;
	layerPagePrefix (job, &xb);
	agxbput (&xb, saveid);
	obj->id = agxbuse(&xb);
    }
    else
	saveid = NULL;

    setColorScheme (agget (g, "colorscheme"));
    setup_page(job);
    gvrender_begin_page(job);
    gvrender_set_pencolor(job, DEFAULT_COLOR);
    gvrender_set_fillcolor(job, DEFAULT_FILL);
    if ((flags & (GVRENDER_DOES_MAPS | GVRENDER_DOES_TOOLTIPS))
	    && (obj->url || obj->explicit_tooltip)) {
	if (flags & (GVRENDER_DOES_MAP_RECTANGLE | GVRENDER_DOES_MAP_POLYGON)) {
	    if (flags & GVRENDER_DOES_MAP_RECTANGLE) {
		obj->url_map_shape = MAP_RECTANGLE;
		nump = 2;
	    }
	    else {
		obj->url_map_shape = MAP_POLYGON;
		nump = 4;
	    }
	    p = gv_calloc(nump, sizeof(pointf));
	    p[0] = job->pageBox.LL;
	    p[1] = job->pageBox.UR;
	    if (! (flags & (GVRENDER_DOES_MAP_RECTANGLE)))
		rect2poly(p);
	}
	if (! (flags & GVRENDER_DOES_TRANSFORM))
	    gvrender_ptf_A(job, p, p, nump);
	obj->url_map_p = p;
	obj->url_map_n = nump;
    }
    if ((flags & GVRENDER_DOES_LABELS) && ((lab = GD_label(g))))
	/* do graph label on every page and rely on clipping to show it on the right one(s) */
	obj->label = lab->text;
	/* If EMIT_CLUSTERS_LAST is set, we assume any URL or tooltip
	 * attached to the root graph is emitted either in begin_page
	 * or end_page of renderer.
	 */
    if (!(flags & EMIT_CLUSTERS_LAST) && (obj->url || obj->explicit_tooltip)) {
	emit_map_rect(job, job->clip);
	gvrender_begin_anchor(job, obj->url, obj->tooltip, obj->target, obj->id);
    }
    /* if (numPhysicalLayers(job) == 1) */
	emit_background(job, g);
    if (GD_label(g))
	emit_label(job, EMIT_GLABEL, GD_label(g));
    if (!(flags & EMIT_CLUSTERS_LAST) && (obj->url || obj->explicit_tooltip))
	gvrender_end_anchor(job);
    emit_view(job,g,flags);
    gvrender_end_page(job);
    if (saveid) {
	obj->id = saveid;
    }
    agxbfree(&xb);
}

void emit_graph(GVJ_t * job, graph_t * g)
{
    node_t *n;
    char *s;
    int flags = job->flags;
    int* lp;

    /* device dpi is now known */
    job->scale.x = job->zoom * job->dpi.x / POINTS_PER_INCH;
    job->scale.y = job->zoom * job->dpi.y / POINTS_PER_INCH;

    job->devscale.x = job->dpi.x / POINTS_PER_INCH;
    job->devscale.y = job->dpi.y / POINTS_PER_INCH;
    if ((job->flags & GVRENDER_Y_GOES_DOWN) || (Y_invert))
	job->devscale.y *= -1;

    /* compute current view in graph units */
    if (job->rotation) {
	job->view.y = job->width / job->scale.y;
	job->view.x = job->height / job->scale.x;
    }
    else {
	job->view.x = job->width / job->scale.x;
	job->view.y = job->height / job->scale.y;
    }

    s = late_string(g, agattr(g, AGRAPH, "comment", 0), "");
    gvrender_comment(job, s);

    job->layerNum = 0;
    emit_begin_graph(job, g);

    if (flags & EMIT_COLORS)
	emit_colors(job,g);

    /* reset node state */
    for (n = agfstnode(g); n; n = agnxtnode(g, n))
	ND_state(n) = 0;
    /* iterate layers */
    for (firstlayer(job,&lp); validlayer(job); nextlayer(job,&lp)) {
	if (numPhysicalLayers (job) > 1)
	    gvrender_begin_layer(job);

	/* iterate pages */
	for (firstpage(job); validpage(job); nextpage(job))
	    emit_page(job, g);

	if (numPhysicalLayers (job) > 1)
	    gvrender_end_layer(job);
    } 
    emit_end_graph(job);
}

/* support for stderr_once */
static void free_string_entry(char *key, Dtdisc_t *disc) {
    (void)disc;
    free(key);
}

static Dict_t *strings;
static Dtdisc_t stringdict = {
    .link = -1, // link - allocate separate holder objects
    .freef = (Dtfree_f)free_string_entry,
};

bool emit_once(char *str) {
    if (strings == 0)
	strings = dtopen(&stringdict, Dtoset);
    if (!dtsearch(strings, str)) {
	dtinsert(strings, gv_strdup(str));
	return true;
    }
    return false;
}

void emit_once_reset(void)
{
    if (strings) {
	dtclose(strings);
	strings = 0;
    }
}

static void emit_begin_cluster(GVJ_t * job, Agraph_t * sg)
{
    obj_state_t *obj;

    obj = push_obj_state(job);
    obj->type = CLUSTER_OBJTYPE;
    obj->u.sg = sg;
    obj->emit_state = EMIT_CDRAW;

    initObjMapData (job, GD_label(sg), sg);
    
    gvrender_begin_cluster(job);
}

static void emit_end_cluster(GVJ_t *job) {
    gvrender_end_cluster(job);
    pop_obj_state(job);
}

void emit_clusters(GVJ_t * job, Agraph_t * g, int flags)
{
    int doPerim, c, istyle, filled;
    pointf AF[4];
    char *color, *fillcolor, *pencolor, **style, *s;
    graph_t *sg;
    node_t *n;
    edge_t *e;
    obj_state_t *obj;
    textlabel_t *lab;
    int doAnchor;
    double penwidth;
    char* clrs[2];
    
    for (c = 1; c <= GD_n_cluster(g); c++) {
	sg = GD_clust(g)[c];
	if (!clust_in_layer(job, sg))
	    continue;
	/* when mapping, detect events on clusters after sub_clusters */
	if (flags & EMIT_CLUSTERS_LAST)
	    emit_clusters(job, sg, flags);
	emit_begin_cluster(job, sg);
	obj = job->obj;
	doAnchor = obj->url || obj->explicit_tooltip;
	setColorScheme (agget (sg, "colorscheme"));
	if (doAnchor && !(flags & EMIT_CLUSTERS_LAST)) {
	    emit_map_rect(job, GD_bb(sg));
	    gvrender_begin_anchor(job, obj->url, obj->tooltip, obj->target, obj->id);
	}
	filled = FALSE;
	istyle = 0;
	if ((style = checkClusterStyle(sg, &istyle))) {
	    gvrender_set_style(job, style);
	    if (istyle & FILLED)
		filled = FILL;
	}
	fillcolor = pencolor = 0;

	if (GD_gui_state(sg) & GUI_STATE_ACTIVE) {
	    pencolor = late_nnstring(sg, G_activepencolor, DEFAULT_ACTIVEPENCOLOR);
	    fillcolor = late_nnstring(sg, G_activefillcolor, DEFAULT_ACTIVEFILLCOLOR);
	    filled = TRUE;
	}
	else if (GD_gui_state(sg) & GUI_STATE_SELECTED) {
	    pencolor = late_nnstring(sg, G_activepencolor, DEFAULT_SELECTEDPENCOLOR);
	    fillcolor = late_nnstring(sg, G_activefillcolor, DEFAULT_SELECTEDFILLCOLOR);
	    filled = TRUE;
	}
	else if (GD_gui_state(sg) & GUI_STATE_DELETED) {
	    pencolor = late_nnstring(sg, G_deletedpencolor, DEFAULT_DELETEDPENCOLOR);
	    fillcolor = late_nnstring(sg, G_deletedfillcolor, DEFAULT_DELETEDFILLCOLOR);
	    filled = TRUE;
	}
	else if (GD_gui_state(sg) & GUI_STATE_VISITED) {
	    pencolor = late_nnstring(sg, G_visitedpencolor, DEFAULT_VISITEDPENCOLOR);
	    fillcolor = late_nnstring(sg, G_visitedfillcolor, DEFAULT_VISITEDFILLCOLOR);
	    filled = TRUE;
	}
	else {
	    if ((color = agget(sg, "color")) != 0 && color[0])
		fillcolor = pencolor = color;
	    if ((color = agget(sg, "pencolor")) != 0 && color[0])
		pencolor = color;
	    if ((color = agget(sg, "fillcolor")) != 0 && color[0])
		fillcolor = color;
	    /* bgcolor is supported for backward compatibility 
	       if fill is set, fillcolor trumps bgcolor, so
               don't bother checking.
               if gradient is set fillcolor trumps bgcolor
             */
	    if ((!filled || !fillcolor) && (color = agget(sg, "bgcolor")) != 0 && color[0]) {
		fillcolor = color;
	        filled = FILL;
            }

	}
	if (!pencolor) pencolor = DEFAULT_COLOR;
	if (!fillcolor) fillcolor = DEFAULT_FILL;
	clrs[0] = NULL;
	if (filled) {
	    float frac;
	    if (findStopColor (fillcolor, clrs, &frac)) {
        	gvrender_set_fillcolor(job, clrs[0]);
		if (clrs[1]) 
		    gvrender_set_gradient_vals(job,clrs[1],late_int(sg,G_gradientangle,0,0), frac);
		else 
		    gvrender_set_gradient_vals(job,DEFAULT_COLOR,late_int(sg,G_gradientangle,0,0), frac);
		if (istyle & RADIAL)
		    filled = RGRADIENT;
	 	else
		    filled = GRADIENT;
	    }
	    else
        	gvrender_set_fillcolor(job, fillcolor);
	}

	if (G_penwidth && ((s=ag_xget(sg,G_penwidth)) && s[0])) {
	    penwidth = late_double(sg, G_penwidth, 1.0, 0.0);
            gvrender_set_penwidth(job, penwidth);
	}

	if (istyle & ROUNDED) {
	    if ((doPerim = late_int(sg, G_peripheries, 1, 0)) || filled) {
		AF[0] = GD_bb(sg).LL;
		AF[2] = GD_bb(sg).UR;
		AF[1].x = AF[2].x;
		AF[1].y = AF[0].y;
		AF[3].x = AF[0].x;
		AF[3].y = AF[2].y;
		if (doPerim)
    		    gvrender_set_pencolor(job, pencolor);
		else
        	    gvrender_set_pencolor(job, "transparent");
		round_corners(job, AF, 4, istyle, filled);
	    }
	}
	else if (istyle & STRIPED) {
	    int rv;
	    AF[0] = GD_bb(sg).LL;
	    AF[2] = GD_bb(sg).UR;
	    AF[1].x = AF[2].x;
	    AF[1].y = AF[0].y;
	    AF[3].x = AF[0].x;
	    AF[3].y = AF[2].y;
	    if (late_int(sg, G_peripheries, 1, 0) == 0)
        	gvrender_set_pencolor(job, "transparent");
	    else
    		gvrender_set_pencolor(job, pencolor);
	    rv = stripedBox (job, AF, fillcolor, 0);
	    if (rv > 1)
		agerr (AGPREV, "in cluster %s\n", agnameof(sg));
	    gvrender_box(job, GD_bb(sg), 0);
	}
	else {
	    if (late_int(sg, G_peripheries, 1, 0)) {
    		gvrender_set_pencolor(job, pencolor);
		gvrender_box(job, GD_bb(sg), filled);
	    }
	    else if (filled) {
        	gvrender_set_pencolor(job, "transparent");
		gvrender_box(job, GD_bb(sg), filled);
	    }
	}

	free (clrs[0]);
	if ((lab = GD_label(sg)))
	    emit_label(job, EMIT_CLABEL, lab);

	if (doAnchor) {
	    if (flags & EMIT_CLUSTERS_LAST) {
		emit_map_rect(job, GD_bb(sg));
		gvrender_begin_anchor(job, obj->url, obj->tooltip, obj->target, obj->id);
	    }
	    gvrender_end_anchor(job);
	}

	if (flags & EMIT_PREORDER) {
	    for (n = agfstnode(sg); n; n = agnxtnode(sg, n)) {
		emit_node(job, n);
		for (e = agfstout(sg, n); e; e = agnxtout(sg, e))
		    emit_edge(job, e);
	    }
	}
	emit_end_cluster(job);
	/* when drawing, lay down clusters before sub_clusters */
	if (!(flags & EMIT_CLUSTERS_LAST))
	    emit_clusters(job, sg, flags);
    }
}

static bool is_style_delim(int c)
{
    switch (c) {
    case '(':
    case ')':
    case ',':
    case '\0':
	return true;
    default:
	return false;
    }
}

#define SID 1

/// Recognized token, returned from `style_token`
///
/// The token content fields, `.start` and `.size` are only populated with
/// useful values when `.type` is `SID`, an identifier.
typedef struct {
  int type; ///< Token category
  const char *start; ///< Beginning of the token content
  size_t size; ///< Number of bytes in the token content
} token_t;

static token_t style_token(char **s) {
    char *p = *s;
    int token;

    while (*p && (isspace((int)*p) || *p == ','))
	p++;
    const char *start = p;
    switch (*p) {
    case '\0':
	token = 0;
	break;
    case '(':
    case ')':
	token = *p++;
	break;
    default:
	token = SID;
	while (!is_style_delim(*p)) {
	    p++;
	}
    }
    *s = p;
    assert(start <= p);
    size_t size = (size_t)(p - start);
    return (token_t){.type = token, .start = start, .size = size};
}

#define FUNLIMIT 64

/* parse_style:
 * This is one of the worst internal designs in graphviz.
 * The use of '\0' characters within strings seems cute but it
 * makes all of the standard functions useless if not dangerous.
 * Plus the function uses static memory for both the array and
 * the character buffer. One hopes all of the values are used
 * before the function is called again.
 */
char **parse_style(char *s)
{
    static char *parse[FUNLIMIT];
    size_t parse_offsets[sizeof(parse) / sizeof(parse[0])];
    size_t fun = 0;
    bool in_parens = false;
    char *p;
    static agxbuf ps_xb;

    p = s;
    while (true) {
	token_t c = style_token(&p);
	if (c.type == 0) {
	    break;
	}
	switch (c.type) {
	case '(':
	    if (in_parens) {
		agerr(AGERR, "nesting not allowed in style: %s\n", s);
		parse[0] = NULL;
		return parse;
	    }
	    in_parens = true;
	    break;

	case ')':
	    if (!in_parens) {
		agerr(AGERR, "unmatched ')' in style: %s\n", s);
		parse[0] = NULL;
		return parse;
	    }
	    in_parens = false;
	    break;

	default:
	    if (!in_parens) {
		if (fun == FUNLIMIT - 1) {
		    agerr(AGWARN, "truncating style '%s'\n", s);
		    parse[fun] = NULL;
		    return parse;
		}
		agxbputc(&ps_xb, '\0');	/* terminate previous */
		parse_offsets[fun++] = agxblen(&ps_xb);
	    }
	    agxbput_n(&ps_xb, c.start, c.size);
	    agxbputc(&ps_xb, '\0');
	}
    }

    if (in_parens) {
	agerr(AGERR, "unmatched '(' in style: %s\n", s);
	parse[0] = NULL;
	return parse;
    }

    char *base = agxbuse(&ps_xb); // add final '\0' to buffer

    // construct list of style strings
    for (size_t i = 0; i < fun; ++i) {
        parse[i] = base + parse_offsets[i];
    }
    parse[fun] = NULL;

    return parse;
}

static boxf bezier_bb(bezier bz)
{
    int i;
    pointf p, p1, p2;
    boxf bb;

    assert(bz.size > 0);
    assert(bz.size % 3 == 1);
    bb.LL = bb.UR = bz.list[0];
    for (i = 1; i < bz.size;) {
	/* take mid-point between two control points for bb calculation */
	p1=bz.list[i];
	i++;
	p2=bz.list[i];
	i++;
	p.x = ( p1.x + p2.x ) / 2;
	p.y = ( p1.y + p2.y ) / 2;
        EXPANDBP(bb,p);

	p=bz.list[i];
        EXPANDBP(bb,p);
	i++;
    }
    return bb;
}

static void init_splines_bb(splines *spl)
{
    int i;
    bezier bz;
    boxf bb, b;

    assert(spl->size > 0);
    bz = spl->list[0];
    bb = bezier_bb(bz);
    for (i = 0; i < spl->size; i++) {
        if (i > 0) {
            bz = spl->list[i];
            b = bezier_bb(bz);
            EXPANDBB(bb, b);
        }
        if (bz.sflag) {
            b = arrow_bb(bz.sp, bz.list[0], 1);
            EXPANDBB(bb, b);
        }
        if (bz.eflag) {
            b = arrow_bb(bz.ep, bz.list[bz.size - 1], 1);
            EXPANDBB(bb, b);
        }
    }
    spl->bb = bb;
}

static void init_bb_edge(edge_t *e)
{
    splines *spl;

    spl = ED_spl(e);
    if (spl)
        init_splines_bb(spl);
}

static void init_bb_node(graph_t *g, node_t *n)
{
    edge_t *e;

    ND_bb(n).LL.x = ND_coord(n).x - ND_lw(n);
    ND_bb(n).LL.y = ND_coord(n).y - ND_ht(n) / 2.;
    ND_bb(n).UR.x = ND_coord(n).x + ND_rw(n);
    ND_bb(n).UR.y = ND_coord(n).y + ND_ht(n) / 2.;

    for (e = agfstout(g, n); e; e = agnxtout(g, e))
        init_bb_edge(e);

    /* IDEA - could also save in the node the bb of the node and
    all of its outedges, then the scan time would be proportional
    to just the number of nodes for many graphs.
    Wouldn't work so well if the edges are sprawling all over the place
    because then the boxes would overlap a lot and require more tests,
    but perhaps that wouldn't add much to the cost before trying individual
    nodes and edges. */
}

static void init_bb(graph_t *g)
{
    node_t *n;

    for (n = agfstnode(g); n; n = agnxtnode(g, n))
        init_bb_node(g, n);
}

extern gvevent_key_binding_t gvevent_key_binding[];
extern int gvevent_key_binding_size;
extern gvdevice_callbacks_t gvdevice_callbacks;

/* gv_fixLocale:
 * Set LC_NUMERIC to "C" to get expected interpretation of %f
 * in printf functions. Languages like postscript and dot expect
 * floating point numbers to use a decimal point.
 * 
 * If set is non-zero, the "C" locale set;
 * if set is zero, the original locale is reset.
 * Calls to the function can nest.
 */
void gv_fixLocale (int set)
{
    static char* save_locale;
    static int cnt;

    if (set) {
	cnt++;
	if (cnt == 1) {
	    save_locale = gv_strdup(setlocale (LC_NUMERIC, NULL));
	    setlocale (LC_NUMERIC, "C");
	}
    }
    else if (cnt > 0) {
	cnt--;
	if (cnt == 0) {
	    setlocale (LC_NUMERIC, save_locale);
	    free (save_locale);
	}
    }
}


#define FINISH() if (Verbose) fprintf(stderr,"gvRenderJobs %s: %.2f secs.\n", agnameof(g), elapsed_sec())

int gvRenderJobs (GVC_t * gvc, graph_t * g)
{
    static GVJ_t *prevjob;
    GVJ_t *job, *firstjob;

    if (Verbose)
	start_timer();
    
    if (!LAYOUT_DONE(g)) {
        agerr (AGERR, "Layout was not done.  Missing layout plugins? \n");
	FINISH();
        return -1;
    }

    init_bb(g);
    init_gvc(gvc, g);
    init_layering(gvc, g);

    gv_fixLocale (1);
    for (job = gvjobs_first(gvc); job; job = gvjobs_next(gvc)) {
	if (gvc->gvg) {
	    job->input_filename = gvc->gvg->input_filename;
	    job->graph_index = gvc->gvg->graph_index;
	}
	else {
	    job->input_filename = NULL;
	    job->graph_index = 0;
	}
	job->common = &(gvc->common);
	job->layout_type = gvc->layout.type;
	job->keybindings = gvevent_key_binding;
	job->numkeys = gvevent_key_binding_size;
	if (!GD_drawing(g)) {
	    agerr (AGERR, "layout was not done\n");
	    gv_fixLocale (0);
	    FINISH();
	    return -1;
	}

        job->output_lang = gvrender_select(job, job->output_langname);
        if (job->output_lang == NO_SUPPORT) {
            agerr (AGERR, "renderer for %s is unavailable\n", job->output_langname);
	    gv_fixLocale (0);
	    FINISH();
            return -1;
        }

        switch (job->output_lang) {
        case VTX:
            /* output sorted, i.e. all nodes then all edges */
            job->flags |= EMIT_SORTED;
            break;
        default:
            job->flags |= chkOrder(g);
            break;
        }

	// if we already have an active job list and the device doesn't support
	// multiple output files, or we are about to write to a different output
	// device
        firstjob = gvc->active_jobs;
        if (firstjob) {
	    if (! (firstjob->flags & GVDEVICE_DOES_PAGES)
	      || strcmp(job->output_langname, firstjob->output_langname)) {

	        gvrender_end_job(firstjob);
	    
            	gvc->active_jobs = NULL; /* clear active list */
	    	gvc->common.viewNum = 0;
	    	prevjob = NULL;
            }
        }
        else {
	    prevjob = NULL;
        }

	if (prevjob) {
            prevjob->next_active = job;  /* insert job in active list */
	    job->output_file = prevjob->output_file;  /* FIXME - this is dumb ! */
	}
	else {
	    if (gvrender_begin_job(job))
		continue;
	    gvc->active_jobs = job;   /* first job of new list */
	}
	job->next_active = NULL;      /* terminate active list */
	job->callbacks = &gvdevice_callbacks;

	init_job_pad(job);
	init_job_margin(job);
	init_job_dpi(job, g);
	init_job_viewport(job, g);
	init_job_pagination(job, g);

	if (! (job->flags & GVDEVICE_EVENTS)) {
#ifdef DEBUG
    		/* Show_boxes is not defined, if at all, 
                 * until splines are generated in dot 
                 */
	    show_boxes_append(&Show_boxes, NULL);
	    job->common->show_boxes = Show_boxes.data;
#endif
	    emit_graph(job, g);
	}

        /* the last job, after all input graphs are processed,
         *      is finalized from gvFinalize()
         */
	prevjob = job;
    }
    gv_fixLocale (0);
    FINISH();
    return 0;
}

/* findStopColor:
 * Check for colon in colorlist. If one exists, and not the first
 * character, store the characters before the colon in clrs[0] and
 * the characters after the colon (and before the next or end-of-string)
 * in clrs[1]. If there are no characters after the first colon, clrs[1]
 * is NULL. Return TRUE.
 * If there is no non-trivial string before a first colon, set clrs[0] to
 * NULL and return FALSE.
 *
 * Note that memory is allocated as a single block stored in clrs[0] and
 * must be freed by calling function.
 */
bool findStopColor (char* colorlist, char* clrs[2], float* frac)
{
    colorsegs_t segs = {0};
    int rv;

    rv = parseSegs (colorlist, 0, &segs);
    if (rv || segs.numc < 2 || segs.segs[0].color == NULL) {
	clrs[0] = NULL;
	freeSegs(&segs);
	return false;
    }

    if (segs.numc > 2)
	agerr (AGWARN, "More than 2 colors specified for a gradient - ignoring remaining\n");

    clrs[0] = gv_calloc(strlen(colorlist) + 1, sizeof(char));
    strcpy(clrs[0], segs.segs[0].color);
    if (segs.segs[1].color) {
	clrs[1] = clrs[0] + (strlen(clrs[0])+1);
	strcpy(clrs[1], segs.segs[1].color);
    }
    else
	clrs[1] = NULL;

    if (segs.segs[0].hasFraction)
	*frac = segs.segs[0].t;
    else if (segs.segs[1].hasFraction)
	*frac = 1 - segs.segs[1].t;
    else 
	*frac = 0;

    freeSegs(&segs);
    return true;
}

