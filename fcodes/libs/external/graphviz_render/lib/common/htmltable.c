/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/


/* Implementation of HTML-like tables.
 * 
 * The (now purged) CodeGen graphics model, especially with integral coodinates, is
 * not adequate to handle this as we would like. In particular, it is
 * difficult to handle notions of adjacency and correct rounding to pixels.
 * For example, if 2 adjacent boxes bb1.UR.x == bb2.LL.x, the rectangles
 * may be drawn overlapping. However, if we use bb1.UR.x+1 == bb2.LL.x
 * there may or may not be a gap between them, even in the same device
 * depending on their positions. When CELLSPACING > 1, this isn't as much
 * of a problem.
 *
 * We allow negative spacing as a hack to allow overlapping cell boundaries.
 * For the reasons discussed above, this is difficult to get correct.
 * This is an important enough case we should extend the table model to
 * support it correctly. This could be done by allowing a table attribute,
 * e.g., CELLGRID=n, which sets CELLBORDER=0 and has the border drawing
 * handled correctly by the table.
 */

#include <assert.h>
#include <common/render.h>
#include <common/htmltable.h>
#include <cgraph/agxbuf.h>
#include <cgraph/prisize_t.h>
#include <common/pointset.h>
#include <common/intset.h>
#include <cdt/cdt.h>
#include <cgraph/alloc.h>
#include <cgraph/exit.h>
#include <cgraph/strcasecmp.h>
#include <cgraph/unreachable.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#define DEFAULT_BORDER    1
#define DEFAULT_CELLPADDING  2
#define DEFAULT_CELLSPACING  2

typedef struct {
    char *url;
    char *tooltip;
    char *target;
    char *id;
    bool explicit_tooltip;
    point LL;
    point UR;
} htmlmap_data_t;

#ifdef DEBUG
static void printCell(htmlcell_t * cp, int ind);
#endif

/* pushFontInfo:
 * Replace current font attributes in env with ones from fp,
 * storing old attributes in savp. We only deal with attributes
 * set in env. The attributes are restored via popFontInfo.
 */
static void
pushFontInfo(htmlenv_t * env, textfont_t * fp, textfont_t * savp)
{
    if (env->finfo.name) {
	if (fp->name) {
	    savp->name = env->finfo.name;
	    env->finfo.name = fp->name;
	} else
	    savp->name = NULL;
    }
    if (env->finfo.color) {
	if (fp->color) {
	    savp->color = env->finfo.color;
	    env->finfo.color = fp->color;
	} else
	    savp->color = NULL;
    }
    if (env->finfo.size >= 0) {
	if (fp->size >= 0) {
	    savp->size = env->finfo.size;
	    env->finfo.size = fp->size;
	} else
	    savp->size = -1.0;
    }
}

/* popFontInfo:
 * Restore saved font attributes.
 * Copy only set values.
 */
static void popFontInfo(htmlenv_t * env, textfont_t * savp)
{
    if (savp->name)
	env->finfo.name = savp->name;
    if (savp->color)
	env->finfo.color = savp->color;
    if (savp->size >= 0.0)
	env->finfo.size = savp->size;
}

static void
emit_htextspans(GVJ_t *job, size_t nspans, htextspan_t *spans, pointf p,
		double halfwidth_x, textfont_t finfo, boxf b, int simple)
{
    double center_x, left_x, right_x;
    textspan_t tl;
    textfont_t tf;
    pointf p_ = { 0.0, 0.0 };
    textspan_t *ti;

    center_x = p.x;
    left_x = center_x - halfwidth_x;
    right_x = center_x + halfwidth_x;

    /* Initial p is in center of text block; set initial baseline
     * to top of text block.
     */
    p_.y = p.y + (b.UR.y - b.LL.y) / 2.0;

    gvrender_begin_label(job, LABEL_HTML);
    for (size_t i = 0; i < nspans; i++) {
	/* set p.x to leftmost point where the line of text begins */
	switch (spans[i].just) {
	case 'l':
	    p.x = left_x;
	    break;
	case 'r':
	    p.x = right_x - spans[i].size;
	    break;
	default:
	case 'n':
	    p.x = center_x - spans[i].size / 2.0;
	    break;
	}
	p_.y -= spans[i].lfsize;	/* move to current base line */

	ti = spans[i].items;
	for (size_t j = 0; j < spans[i].nitems; j++) {
	    if (ti->font && ti->font->size > 0)
		tf.size = ti->font->size;
	    else
		tf.size = finfo.size;
	    if (ti->font && ti->font->name)
		tf.name = ti->font->name;
	    else
		tf.name = finfo.name;
	    if (ti->font && ti->font->color)
		tf.color = ti->font->color;
	    else
		tf.color = finfo.color;
	    if (ti->font && ti->font->flags)
		tf.flags = ti->font->flags;
	    else
		tf.flags = 0;

	    gvrender_set_pencolor(job, tf.color);

	    tl.str = ti->str;
	    tl.font = &tf;
	    tl.yoffset_layout = ti->yoffset_layout;
	    if (simple)
		tl.yoffset_centerline = ti->yoffset_centerline;
	    else
		tl.yoffset_centerline = 1;
	    tl.font->postscript_alias = ti->font->postscript_alias;
	    tl.layout = ti->layout;
	    tl.size.x = ti->size.x;
	    tl.size.y = spans[i].lfsize;
	    tl.just = 'l';

	    p_.x = p.x;
	    gvrender_textspan(job, p_, &tl);
	    p.x += ti->size.x;
	    ti++;
	}
    }

    gvrender_end_label(job);
}

static void emit_html_txt(GVJ_t * job, htmltxt_t * tp, htmlenv_t * env)
{
    double halfwidth_x;
    pointf p;

    /* make sure that there is something to do */
    if (tp->nspans < 1)
	return;

    halfwidth_x = (tp->box.UR.x - tp->box.LL.x) / 2.0;
    p.x = env->pos.x + (tp->box.UR.x + tp->box.LL.x) / 2.0;
    p.y = env->pos.y + (tp->box.UR.y + tp->box.LL.y) / 2.0;

    emit_htextspans(job, tp->nspans, tp->spans, p, halfwidth_x, env->finfo,
		    tp->box, tp->simple);
}

static void doSide(GVJ_t * job, pointf p, double wd, double ht)
{
    boxf BF;

    BF.LL = p;
    BF.UR.x = p.x + wd;
    BF.UR.y = p.y + ht;
    gvrender_box(job, BF, 1);
}

/* mkPts:
 * Convert boxf into four corner points
 * If border is > 1, inset the points by half the border.
 * It is assumed AF is pointf[4], so the data is store there
 * and AF is returned.
 */
static pointf *mkPts(pointf * AF, boxf b, int border)
{
    AF[0] = b.LL;
    AF[2] = b.UR;
    if (border > 1) {
	double delta = (double)border / 2.0;
	AF[0].x += delta;
	AF[0].y += delta;
	AF[2].x -= delta;
	AF[2].y -= delta;
    }
    AF[1].x = AF[2].x;
    AF[1].y = AF[0].y;
    AF[3].x = AF[0].x;
    AF[3].y = AF[2].y;

    return AF;
}

/* doBorder:
 * Draw a rectangular border for the box b.
 * Handles dashed and dotted styles, rounded corners.
 * Also handles thick lines.
 * Assume dp->border > 0
 */
static void doBorder(GVJ_t * job, htmldata_t * dp, boxf b)
{
    pointf AF[7];
    char *sptr[2];
    char *color = dp->pencolor ? dp->pencolor : DEFAULT_COLOR;
    unsigned short sides;

    gvrender_set_pencolor(job, color);
    if (dp->style & (DASHED | DOTTED)) {
	sptr[0] = sptr[1] = NULL;
	if (dp->style & DASHED)
	    sptr[0] = "dashed";
	else if (dp->style & DOTTED)
	    sptr[0] = "dotted";
	gvrender_set_style(job, sptr);
    } else
	gvrender_set_style(job, job->gvc->defaultlinestyle);
    gvrender_set_penwidth(job, dp->border);

    if (dp->style & ROUNDED)
	round_corners(job, mkPts(AF, b, dp->border), 4, ROUNDED, 0);
    else if ((sides = (dp->flags & BORDER_MASK))) {
	mkPts (AF+1, b, dp->border);  /* AF[1-4] has LL=SW,SE,UR=NE,NW */
	switch (sides) {
	case BORDER_BOTTOM :
	    gvrender_polyline(job, AF+1, 2);
	    break;
	case BORDER_RIGHT :
	    gvrender_polyline(job, AF+2, 2);
	    break;
	case BORDER_TOP :
	    gvrender_polyline(job, AF+3, 2);
	    break;
	case BORDER_LEFT :
	    AF[0] = AF[4];
	    gvrender_polyline(job, AF, 2);
	    break;
	case BORDER_BOTTOM|BORDER_RIGHT :
	    gvrender_polyline(job, AF+1, 3);
	    break;
	case BORDER_RIGHT|BORDER_TOP :
	    gvrender_polyline(job, AF+2, 3);
	    break;
	case BORDER_TOP|BORDER_LEFT :
	    AF[5] = AF[1];
	    gvrender_polyline(job, AF+3, 3);
	    break;
	case BORDER_LEFT|BORDER_BOTTOM :
	    AF[0] = AF[4];
	    gvrender_polyline(job, AF, 3);
	    break;
	case BORDER_BOTTOM|BORDER_RIGHT|BORDER_TOP :
	    gvrender_polyline(job, AF+1, 4);
	    break;
	case BORDER_RIGHT|BORDER_TOP|BORDER_LEFT :
	    AF[5] = AF[1];
	    gvrender_polyline(job, AF+2, 4);
	    break;
	case BORDER_TOP|BORDER_LEFT|BORDER_BOTTOM :
	    AF[5] = AF[1];
	    AF[6] = AF[2];
	    gvrender_polyline(job, AF+3, 4);
	    break;
	case BORDER_LEFT|BORDER_BOTTOM|BORDER_RIGHT :
	    AF[0] = AF[4];
	    gvrender_polyline(job, AF, 4);
	    break;
	case BORDER_TOP|BORDER_BOTTOM :
	    gvrender_polyline(job, AF+1, 2);
	    gvrender_polyline(job, AF+3, 2);
	    break;
	case BORDER_LEFT|BORDER_RIGHT :
	    AF[0] = AF[4];
	    gvrender_polyline(job, AF, 2);
	    gvrender_polyline(job, AF+2, 2);
	    break;
	}
    } else {
	if (dp->border > 1) {
	    double delta = (double)dp->border / 2.0;
	    b.LL.x += delta;
	    b.LL.y += delta;
	    b.UR.x -= delta;
	    b.UR.y -= delta;
	}
	gvrender_box(job, b, 0);
    }
}

/* setFill:
 * Set up fill values from given color; make pen transparent.
 * Return type of fill required.
 */
static int
setFill(GVJ_t * job, char *color, int angle, int style, char *clrs[2])
{
    int filled;
    float frac;
    if (findStopColor(color, clrs, &frac)) {
	gvrender_set_fillcolor(job, clrs[0]);
	if (clrs[1])
	    gvrender_set_gradient_vals(job, clrs[1], angle, frac);
	else
	    gvrender_set_gradient_vals(job, DEFAULT_COLOR, angle, frac);
	if (style & RADIAL)
	    filled = RGRADIENT;
	else
	    filled = GRADIENT;
    } else {
	gvrender_set_fillcolor(job, color);
	filled = FILL;
    }
    gvrender_set_pencolor(job, "transparent");
    return filled;
}

/* initAnchor:
 * Save current map values.
 * Initialize fields in job->obj pertaining to anchors.
 * In particular, this also sets the output rectangle.
 * If there is something to do, 
 * start the anchor and returns 1.
 * Otherwise, it returns 0.
 *
 * FIX: Should we provide a tooltip if none is set, as is done
 * for nodes, edges, etc. ?
 */
static int
initAnchor(GVJ_t * job, htmlenv_t * env, htmldata_t * data, boxf b,
  htmlmap_data_t * save)
{
    obj_state_t *obj = job->obj;
    int changed;
    char *id;
    static int anchorId;
    agxbuf xb = {0};

    save->url = obj->url;
    save->tooltip = obj->tooltip;
    save->target = obj->target;
    save->id = obj->id;
    save->explicit_tooltip = obj->explicit_tooltip != 0;
    id = data->id;
    if (!id || !*id) {		/* no external id, so use the internal one */
	if (!env->objid) {
	    env->objid = gv_strdup(getObjId(job, obj->u.n, &xb));
	    env->objid_set = true;
	}
	agxbprint(&xb, "%s_%d", env->objid, anchorId++);
	id = agxbuse(&xb);
    }
    changed =
	initMapData(job, NULL, data->href, data->title, data->target, id,
		    obj->u.g);
    agxbfree(&xb);

    if (changed) {
	if (obj->url || obj->explicit_tooltip) {
	    emit_map_rect(job, b);
	    gvrender_begin_anchor(job,
				  obj->url, obj->tooltip, obj->target,
				  obj->id);
	}
    }
    return changed;
}

#define RESET(fld) \
  if(obj->fld != save->fld) {free(obj->fld); obj->fld = save->fld;}

/* endAnchor:
 * Pop context pushed by initAnchor.
 * This is done by ending current anchor, restoring old values and
 * freeing new.
 *
 * NB: We don't save or restore geometric map info. This is because
 * this preservation of map context is only necessary for SVG-like
 * systems where graphical items are wrapped in an anchor, and we map
 * top-down. For ordinary map anchors, this is all done bottom-up, so
 * the geometric map info at the higher level hasn't been emitted yet.
 */
static void endAnchor(GVJ_t * job, htmlmap_data_t * save)
{
    obj_state_t *obj = job->obj;

    if (obj->url || obj->explicit_tooltip)
	gvrender_end_anchor(job);
    RESET(url);
    RESET(tooltip);
    RESET(target);
    RESET(id);
    obj->explicit_tooltip = save->explicit_tooltip;
}

/* forward declaration */
static void emit_html_cell(GVJ_t * job, htmlcell_t * cp, htmlenv_t * env);

/* emit_html_rules:
 * place vertical and horizontal lines between adjacent cells and
 * extend the lines to intersect the rounded table boundary 
 */
static void
emit_html_rules(GVJ_t * job, htmlcell_t * cp, htmlenv_t * env, char *color, htmlcell_t* nextc)
{
    pointf rule_pt;
    double rule_length;
    double base;
    boxf pts = cp->data.box;
    pointf pos = env->pos;

    if (!color)
	color = DEFAULT_COLOR;
    gvrender_set_fillcolor(job, color);
    gvrender_set_pencolor(job, color);

    pts = cp->data.box;
    pts.LL.x += pos.x;
    pts.UR.x += pos.x;
    pts.LL.y += pos.y;
    pts.UR.y += pos.y;

    //Determine vertical line coordinate and length
    if ((cp->ruled & HTML_VRULE) && cp->col + cp->colspan < cp->parent->column_count) {
	if (cp->row == 0) {	// first row
	    // extend to center of table border and add half cell spacing
	    base = cp->parent->data.border + cp->parent->data.space / 2;
	    rule_pt.y = pts.LL.y - cp->parent->data.space / 2;
	} else if (cp->row + cp->rowspan == cp->parent->row_count) { // bottom row
	    // extend to center of table border and add half cell spacing
	    base = cp->parent->data.border + cp->parent->data.space / 2;
	    rule_pt.y = pts.LL.y - cp->parent->data.space / 2 - base;
	} else {
	    base = 0;
	    rule_pt.y = pts.LL.y - cp->parent->data.space / 2;
	}
	rule_pt.x = pts.UR.x + cp->parent->data.space / 2;
	rule_length = base + pts.UR.y - pts.LL.y + cp->parent->data.space;
	doSide(job, rule_pt, 0, rule_length);
    }
    //Determine the horizontal coordinate and length
    if ((cp->ruled & HTML_HRULE) && cp->row + cp->rowspan < cp->parent->row_count) {
	if (cp->col == 0) {	// first column 
	    // extend to center of table border and add half cell spacing
	    base = cp->parent->data.border + cp->parent->data.space / 2;
	    rule_pt.x = pts.LL.x - base - cp->parent->data.space / 2;
	    if (cp->col + cp->colspan == cp->parent->column_count)	// also last column
		base *= 2;
	    /* incomplete row of cells; extend line to end */
	    else if (nextc && nextc->row != cp->row) {
		base += cp->parent->data.box.UR.x + pos.x - (pts.UR.x + cp->parent->data.space / 2);
	    }
	} else if (cp->col + cp->colspan == cp->parent->column_count) {	// last column
	    // extend to center of table border and add half cell spacing
	    base = cp->parent->data.border + cp->parent->data.space / 2;
	    rule_pt.x = pts.LL.x - cp->parent->data.space / 2;
	} else {
	    base = 0;
	    rule_pt.x = pts.LL.x - cp->parent->data.space / 2;
	    /* incomplete row of cells; extend line to end */
	    if (nextc && nextc->row != cp->row) {
		base += cp->parent->data.box.UR.x + pos.x - (pts.UR.x + cp->parent->data.space / 2);
	    }
	}
	rule_pt.y = pts.LL.y - cp->parent->data.space / 2;
	rule_length = base + pts.UR.x - pts.LL.x + cp->parent->data.space;
	doSide(job, rule_pt, rule_length, 0);
    }
}

static void emit_html_tbl(GVJ_t * job, htmltbl_t * tbl, htmlenv_t * env)
{
    boxf pts = tbl->data.box;
    pointf pos = env->pos;
    htmlcell_t **cells = tbl->u.n.cells;
    htmlcell_t *cp;
    static textfont_t savef;
    htmlmap_data_t saved;
    int anchor;			/* if true, we need to undo anchor settings. */
    int doAnchor = (tbl->data.href || tbl->data.target);
    pointf AF[4];

    if (tbl->font)
	pushFontInfo(env, tbl->font, &savef);

    pts.LL.x += pos.x;
    pts.UR.x += pos.x;
    pts.LL.y += pos.y;
    pts.UR.y += pos.y;

    if (doAnchor && !(job->flags & EMIT_CLUSTERS_LAST))
	anchor = initAnchor(job, env, &tbl->data, pts, &saved);
    else
	anchor = 0;

    if (!(tbl->data.style & INVISIBLE)) {

	/* Fill first */
	if (tbl->data.bgcolor) {
	    char *clrs[2];
	    int filled =
		setFill(job, tbl->data.bgcolor, tbl->data.gradientangle,
			tbl->data.style, clrs);
	    if (tbl->data.style & ROUNDED) {
		round_corners(job, mkPts(AF, pts, tbl->data.border), 4,
			      ROUNDED, filled);
	    } else
		gvrender_box(job, pts, filled);
	    free(clrs[0]);
	}

	while (*cells) {
	    emit_html_cell(job, *cells, env);
	    cells++;
	}

	/* Draw table rules and border.
	 * Draw after cells so we can draw over any fill.
	 * At present, we set the penwidth to 1 for rules until we provide the calculations to take
	 * into account wider rules.
	 */
	cells = tbl->u.n.cells;
	gvrender_set_penwidth(job, 1.0);
	while ((cp = *cells++)) {
	    if (cp->ruled)
		emit_html_rules(job, cp, env, tbl->data.pencolor, *cells);
	}

	if (tbl->data.border)
	    doBorder(job, &tbl->data, pts);

    }

    if (anchor)
	endAnchor(job, &saved);

    if (doAnchor && (job->flags & EMIT_CLUSTERS_LAST)) {
	if (initAnchor(job, env, &tbl->data, pts, &saved))
	    endAnchor(job, &saved);
    }

    if (tbl->font)
	popFontInfo(env, &savef);
}

/* emit_html_img:
 * The image will be centered in the given box.
 * Scaling is determined by either the image's scale attribute,
 * or the imagescale attribute of the graph object being drawn.
 */
static void emit_html_img(GVJ_t * job, htmlimg_t * cp, htmlenv_t * env)
{
    pointf A[4];
    boxf bb = cp->box;
    char *scale;

    bb.LL.x += env->pos.x;
    bb.LL.y += env->pos.y;
    bb.UR.x += env->pos.x;
    bb.UR.y += env->pos.y;

    A[0] = bb.UR;
    A[2] = bb.LL;
    A[1].x = A[2].x;
    A[1].y = A[0].y;
    A[3].x = A[0].x;
    A[3].y = A[2].y;

    if (cp->scale)
	scale = cp->scale;
    else
	scale = env->imgscale;
    assert(cp->src);
    assert(cp->src[0]);
    gvrender_usershape(job, cp->src, A, 4, true, scale, "mc");
}

static void emit_html_cell(GVJ_t * job, htmlcell_t * cp, htmlenv_t * env)
{
    htmlmap_data_t saved;
    boxf pts = cp->data.box;
    pointf pos = env->pos;
    int inAnchor, doAnchor = (cp->data.href || cp->data.target);
    pointf AF[4];

    pts.LL.x += pos.x;
    pts.UR.x += pos.x;
    pts.LL.y += pos.y;
    pts.UR.y += pos.y;

    if (doAnchor && !(job->flags & EMIT_CLUSTERS_LAST))
	inAnchor = initAnchor(job, env, &cp->data, pts, &saved);
    else
	inAnchor = 0;

    if (!(cp->data.style & INVISIBLE)) {
	if (cp->data.bgcolor) {
	    char *clrs[2];
	    int filled =
		setFill(job, cp->data.bgcolor, cp->data.gradientangle,
			cp->data.style, clrs);
	    if (cp->data.style & ROUNDED) {
		round_corners(job, mkPts(AF, pts, cp->data.border), 4,
			      ROUNDED, filled);
	    } else
		gvrender_box(job, pts, filled);
	    free(clrs[0]);
	}

	if (cp->data.border)
	    doBorder(job, &cp->data, pts);

	if (cp->child.kind == HTML_TBL)
	    emit_html_tbl(job, cp->child.u.tbl, env);
	else if (cp->child.kind == HTML_IMAGE)
	    emit_html_img(job, cp->child.u.img, env);
	else
	    emit_html_txt(job, cp->child.u.txt, env);
    }

    if (inAnchor)
	endAnchor(job, &saved);

    if (doAnchor && (job->flags & EMIT_CLUSTERS_LAST)) {
	if (initAnchor(job, env, &cp->data, pts, &saved))
	    endAnchor(job, &saved);
    }
}

/* allocObj:
 * Push new obj on stack to be used in common by all 
 * html elements with anchors.
 * This inherits the type, emit_state, and object of the
 * parent, as well as the url, explicit, target and tooltip.
 */
static void allocObj(GVJ_t * job)
{
    obj_state_t *obj;
    obj_state_t *parent;

    obj = push_obj_state(job);
    parent = obj->parent;
    obj->type = parent->type;
    obj->emit_state = parent->emit_state;
    switch (obj->type) {
    case NODE_OBJTYPE:
	obj->u.n = parent->u.n;
	break;
    case ROOTGRAPH_OBJTYPE:
	obj->u.g = parent->u.g;
	break;
    case CLUSTER_OBJTYPE:
	obj->u.sg = parent->u.sg;
	break;
    case EDGE_OBJTYPE:
	obj->u.e = parent->u.e;
	break;
    }
    obj->url = parent->url;
    obj->tooltip = parent->tooltip;
    obj->target = parent->target;
    obj->explicit_tooltip = parent->explicit_tooltip;
}

static void freeObj(GVJ_t * job)
{
    obj_state_t *obj = job->obj;

    obj->url = NULL;
    obj->tooltip = NULL;
    obj->target = NULL;
    obj->id = NULL;
    pop_obj_state(job);
}

static double
heightOfLbl (htmllabel_t * lp)
{
    double sz = 0.0;

    switch (lp->kind) {
    case HTML_TBL:
	sz  = lp->u.tbl->data.box.UR.y - lp->u.tbl->data.box.LL.y;
	break;
    case HTML_IMAGE:
	sz  = lp->u.img->box.UR.y - lp->u.img->box.LL.y;
	break;
    case HTML_TEXT:
	sz  = lp->u.txt->box.UR.y - lp->u.txt->box.LL.y;
	break;
    default:
	UNREACHABLE();
    }
    return sz;
}

/* emit_html_label:
 */
void emit_html_label(GVJ_t * job, htmllabel_t * lp, textlabel_t * tp)
{
    htmlenv_t env;
    pointf p;

    allocObj(job);

    p = tp->pos;
    switch (tp->valign) {
	case 't':
    	    p.y = tp->pos.y + (tp->space.y - heightOfLbl(lp))/ 2.0 - 1;
	    break;
	case 'b':
    	    p.y = tp->pos.y - (tp->space.y - heightOfLbl(lp))/ 2.0 - 1;
	    break;
	default:	
    	    /* no-op */
	    break;
    }
    env.pos = p;
    env.finfo.color = tp->fontcolor;
    env.finfo.name = tp->fontname;
    env.finfo.size = tp->fontsize;
    env.imgscale = agget(job->obj->u.n, "imagescale");
    env.objid = job->obj->id;
    env.objid_set = false;
    if (env.imgscale == NULL || env.imgscale[0] == '\0')
	env.imgscale = "false";
    if (lp->kind == HTML_TBL) {
	htmltbl_t *tbl = lp->u.tbl;

	/* set basic graphics context */
	/* Need to override line style set by node. */
	gvrender_set_style(job, job->gvc->defaultlinestyle);
	if (tbl->data.pencolor)
	    gvrender_set_pencolor(job, tbl->data.pencolor);
	else
	    gvrender_set_pencolor(job, DEFAULT_COLOR);
	emit_html_tbl(job, tbl, &env);
    } else {
	emit_html_txt(job, lp->u.txt, &env);
    }
    if (env.objid_set)
	free(env.objid);
    freeObj(job);
}

void free_html_data(htmldata_t * dp)
{
    free(dp->href);
    free(dp->port);
    free(dp->target);
    free(dp->id);
    free(dp->title);
    free(dp->bgcolor);
    free(dp->pencolor);
}

void free_html_text(htmltxt_t * t)
{
    htextspan_t *tl;
    textspan_t *ti;

    if (!t)
	return;

    tl = t->spans;
    for (size_t i = 0; i < t->nspans; i++) {
	ti = tl->items;
	for (size_t j = 0; j < tl->nitems; j++) {
	    free(ti->str);
	    if (ti->layout && ti->free_layout)
		ti->free_layout(ti->layout);
	    ti++;
	}
	tl++;
    }
    free(t->spans);
    free(t);
}

static void free_html_img(htmlimg_t * ip)
{
    free(ip->src);
    free(ip);
}

static void free_html_cell(htmlcell_t * cp)
{
    free_html_label(&cp->child, 0);
    free_html_data(&cp->data);
    free(cp);
}

/* free_html_tbl:
 * If tbl->n_rows is negative, table is in initial state from
 * HTML parse, with data stored in u.p. Once run through processTbl,
 * data is stored in u.n and tbl->n_rows is > 0.
 */
static void free_html_tbl(htmltbl_t * tbl)
{
    htmlcell_t **cells;

    if (tbl->row_count == SIZE_MAX) { // raw, parsed table
	dtclose(tbl->u.p.rows);
    } else {
	cells = tbl->u.n.cells;

	free(tbl->heights);
	free(tbl->widths);
	while (*cells) {
	    free_html_cell(*cells);
	    cells++;
	}
	free(tbl->u.n.cells);
    }
    free_html_data(&tbl->data);
    free(tbl);
}

void free_html_label(htmllabel_t * lp, int root)
{
    if (lp->kind == HTML_TBL)
	free_html_tbl(lp->u.tbl);
    else if (lp->kind == HTML_IMAGE)
	free_html_img(lp->u.img);
    else
	free_html_text(lp->u.txt);
    if (root)
	free(lp);
}

static htmldata_t *portToTbl(htmltbl_t *, char *);	/* forward declaration */

static htmldata_t *portToCell(htmlcell_t * cp, char *id)
{
    htmldata_t *rv;

    if (cp->data.port && strcasecmp(cp->data.port, id) == 0)
	rv = &cp->data;
    else if (cp->child.kind == HTML_TBL)
	rv = portToTbl(cp->child.u.tbl, id);
    else
	rv = NULL;

    return rv;
}

/* portToTbl:
 * See if tp or any of its child cells has the given port id.
 * If true, return corresponding box.
 */
static htmldata_t *portToTbl(htmltbl_t * tp, char *id)
{
    htmldata_t *rv;
    htmlcell_t **cells;
    htmlcell_t *cp;

    if (tp->data.port && strcasecmp(tp->data.port, id) == 0)
	rv = &tp->data;
    else {
	rv = NULL;
	cells = tp->u.n.cells;
	while ((cp = *cells++)) {
	    if ((rv = portToCell(cp, id)))
		break;
	}
    }

    return rv;
}

/* html_port:
 * See if edge port corresponds to part of the html node.
 * Assume pname != "".
 * If successful, return pointer to port's box.
 * Else return NULL.
 */
boxf *html_port(node_t *n, char *pname, unsigned char *sides){
    htmldata_t *tp;
    htmllabel_t *lbl = ND_label(n)->u.html;
    boxf *rv = NULL;

    if (lbl->kind == HTML_TEXT)
	return NULL;

    tp = portToTbl(lbl->u.tbl, pname);
    if (tp) {
	rv = &tp->box;
	*sides = tp->sides;
    }
    return rv;

}

static int size_html_txt(GVC_t *gvc, htmltxt_t * ftxt, htmlenv_t * env)
{
    double xsize = 0.0;		/* width of text block */
    double ysize = 0.0;		/* height of text block */
    double lsize;		/* height of current line */
    double mxfsize = 0.0;	/* max. font size for the current line */
    double curbline = 0.0;	/* dist. of current base line from top */
    pointf sz;
    double width;
    textspan_t lp;
    textfont_t tf = {NULL,NULL,NULL,0.0,0,0};
    double maxoffset, mxysize = 0.0;
    bool simple = true; // one item per span, same font size/face, no flags
    double prev_fsize = -1;
    char* prev_fname = NULL;

    for (size_t i = 0; i < ftxt->nspans; i++) {
	if (ftxt->spans[i].nitems > 1) {
	    simple = false;
	    break;
	}
	if (ftxt->spans[i].items[0].font) {
	    if (ftxt->spans[i].items[0].font->flags) {
		simple = false;
		break;
	    }
	    if (ftxt->spans[i].items[0].font->size > 0)
		tf.size = ftxt->spans[i].items[0].font->size;
	    else
		tf.size = env->finfo.size;
	    if (ftxt->spans[i].items[0].font->name)
		tf.name = ftxt->spans[i].items[0].font->name;
	    else
		tf.name = env->finfo.name;
	}
	else {
	    tf.size = env->finfo.size;
	    tf.name = env->finfo.name;
	}
	if (i == 0)
	    prev_fsize = tf.size;
	else if (tf.size != prev_fsize) {
	    simple = false;
	    break;
	}
	if (prev_fname == NULL)
	    prev_fname = tf.name;
	else if (strcmp(tf.name,prev_fname)) {
	    simple = false;
	    break;
	}
    }
    ftxt->simple = simple;

    for (size_t i = 0; i < ftxt->nspans; i++) {
	width = 0;
	mxysize = maxoffset = mxfsize = 0;
	for (size_t j = 0; j < ftxt->spans[i].nitems; j++) {
	    lp.str =
		strdup_and_subst_obj(ftxt->spans[i].items[j].str,
				     env->obj);
	    if (ftxt->spans[i].items[j].font) {
		if (ftxt->spans[i].items[j].font->flags)
		    tf.flags = ftxt->spans[i].items[j].font->flags;
		else if (env->finfo.flags > 0)
		    tf.flags = env->finfo.flags;
		else
		    tf.flags = 0;
		if (ftxt->spans[i].items[j].font->size > 0)
		    tf.size = ftxt->spans[i].items[j].font->size;
		else
		    tf.size = env->finfo.size;
		if (ftxt->spans[i].items[j].font->name)
		    tf.name = ftxt->spans[i].items[j].font->name;
		else
		    tf.name = env->finfo.name;
		if (ftxt->spans[i].items[j].font->color)
		    tf.color = ftxt->spans[i].items[j].font->color;
		else
		    tf.color = env->finfo.color;
	    } else {
		tf.size = env->finfo.size;
		tf.name = env->finfo.name;
		tf.color = env->finfo.color;
		tf.flags = env->finfo.flags;
	    }
	    lp.font = dtinsert(gvc->textfont_dt, &tf);
	    sz = textspan_size(gvc, &lp);
	    free(ftxt->spans[i].items[j].str);
	    ftxt->spans[i].items[j].str = lp.str;
	    ftxt->spans[i].items[j].size.x = sz.x;
	    ftxt->spans[i].items[j].yoffset_layout = lp.yoffset_layout;
	    ftxt->spans[i].items[j].yoffset_centerline = lp.yoffset_centerline;
            ftxt->spans[i].items[j].font = lp.font;
	    ftxt->spans[i].items[j].layout = lp.layout;
	    ftxt->spans[i].items[j].free_layout = lp.free_layout;
	    width += sz.x;
	    mxfsize = MAX(tf.size, mxfsize);
	    mxysize = MAX(sz.y, mxysize);
	    maxoffset = MAX(lp.yoffset_centerline, maxoffset);
	}
	/* lsize = mxfsize * LINESPACING; */
	ftxt->spans[i].size = width;
	/* ysize - curbline is the distance from the previous
	 * baseline to the bottom of the previous line.
	 * Then, in the current line, we set the baseline to
	 * be 5/6 of the max. font size. Thus, lfsize gives the
	 * distance from the previous baseline to the new one.
	 */
	/* ftxt->spans[i].lfsize = 5*mxfsize/6 + ysize - curbline; */
	if (simple) {
	    lsize = mxysize;
	    if (i == 0)
		ftxt->spans[i].lfsize = mxfsize;
	    else
		ftxt->spans[i].lfsize = mxysize;
	}
	else {
	    lsize = mxfsize;
	    if (i == 0)
		ftxt->spans[i].lfsize = mxfsize - maxoffset;
	    else
		ftxt->spans[i].lfsize = mxfsize + ysize - curbline - maxoffset;
	}
	curbline += ftxt->spans[i].lfsize;
	xsize = MAX(width, xsize);
	ysize += lsize;
    }
    ftxt->box.UR.x = xsize;
    if (ftxt->nspans == 1)
	ftxt->box.UR.y = mxysize;
    else
	ftxt->box.UR.y = ysize;
    return 0;
}

/* forward declarion for recursive usage */
static int size_html_tbl(graph_t * g, htmltbl_t * tbl, htmlcell_t * parent,
			 htmlenv_t * env);

/* size_html_img:
 */
static int size_html_img(htmlimg_t * img, htmlenv_t * env)
{
    box b;
    int rv;

    b.LL.x = b.LL.y = 0;
    b.UR = gvusershape_size(env->g, img->src);
    if (b.UR.x == -1 && b.UR.y == -1) {
	rv = 1;
	b.UR.x = b.UR.y = 0;
	agerr(AGERR, "No or improper image file=\"%s\"\n", img->src);
    } else {
	rv = 0;
	GD_has_images(env->g) = true;
    }

    B2BF(b, img->box);
    return rv;
}

/* size_html_cell:
 */
static int
size_html_cell(graph_t * g, htmlcell_t * cp, htmltbl_t * parent,
	       htmlenv_t * env)
{
    int rv;
    pointf sz, child_sz;
    int margin;

    cp->parent = parent;
    if (!(cp->data.flags & PAD_SET)) {
	if (parent->data.flags & PAD_SET)
	    cp->data.pad = parent->data.pad;
	else
	    cp->data.pad = DEFAULT_CELLPADDING;
    }
    if (!(cp->data.flags & BORDER_SET)) {
	if (parent->cellborder >= 0)
	    cp->data.border = (unsigned char)parent->cellborder;
	else if (parent->data.flags & BORDER_SET)
	    cp->data.border = parent->data.border;
	else
	    cp->data.border = DEFAULT_BORDER;
    }

    if (cp->child.kind == HTML_TBL) {
	rv = size_html_tbl(g, cp->child.u.tbl, cp, env);
	child_sz = cp->child.u.tbl->data.box.UR;
    } else if (cp->child.kind == HTML_IMAGE) {
	rv = size_html_img(cp->child.u.img, env);
	child_sz = cp->child.u.img->box.UR;
    } else {
	rv = size_html_txt(GD_gvc(g), cp->child.u.txt, env);
	child_sz = cp->child.u.txt->box.UR;
    }

    margin = 2 * (cp->data.pad + cp->data.border);
    sz.x = child_sz.x + margin;
    sz.y = child_sz.y + margin;

    if (cp->data.flags & FIXED_FLAG) {
	if (cp->data.width && cp->data.height) {
	    if ((cp->data.width < sz.x || cp->data.height < sz.y) && cp->child.kind != HTML_IMAGE) {
		agerr(AGWARN, "cell size too small for content\n");
		rv = 1;
	    }
	    sz.x = sz.y = 0;

	} else {
	    agerr(AGWARN,
		  "fixed cell size with unspecified width or height\n");
	    rv = 1;
	}
    }
    cp->data.box.UR.x = MAX(sz.x, cp->data.width);
    cp->data.box.UR.y = MAX(sz.y, cp->data.height);
    return rv;
}

static int findCol(PointSet * ps, int row, int col, htmlcell_t * cellp)
{
    int notFound = 1;
    int lastc;
    int i, j, c;
    int end = cellp->colspan - 1;

    while (notFound) {
	lastc = col + end;
	for (c = lastc; c >= col; c--) {
	    if (isInPS(ps, c, row))
		break;
	}
	if (c >= col)		/* conflict : try column after */
	    col = c + 1;
	else
	    notFound = 0;
    }
    for (j = col; j < col + cellp->colspan; j++) {
	for (i = row; i < row + cellp->rowspan; i++) {
	    addPS(ps, j, i);
	}
    }
    return col;
}

/* processTbl:
 * Convert parser representation of cells into final form.
 * Find column and row positions of cells.
 * Recursively size cells.
 * Return 1 if problem sizing a cell.
 */
static int processTbl(graph_t * g, htmltbl_t * tbl, htmlenv_t * env)
{
    pitem *rp;
    pitem *cp;
    Dt_t *cdict;
    htmlcell_t *cellp;
    htmlcell_t **cells;
    Dt_t *rows = tbl->u.p.rows;
    int rv = 0;
    size_t n_rows = 0;
    size_t n_cols = 0;
    PointSet *ps = newPS();
    Dt_t *is = openIntSet();

    rp = (pitem *) dtflatten(rows);
    size_t cnt = 0;
    uint16_t r = 0;
    while (rp) {
	cdict = rp->u.rp;
	cp = (pitem *) dtflatten(cdict);
	while (cp) {
	    cellp = cp->u.cp;
	    cnt++;
	    cp = (pitem *)dtlink(cdict, cp);
	}
	if (rp->ruled) {
	    addIntSet(is, r + 1);
	}
	rp = (pitem *)dtlink(rows, rp);
	r++;
    }

    cells = tbl->u.n.cells = N_NEW(cnt + 1, htmlcell_t *);
    rp = (pitem *) dtflatten(rows);
    r = 0;
    while (rp) {
	cdict = rp->u.rp;
	cp = (pitem *) dtflatten(cdict);
	uint16_t c = 0;
	while (cp) {
	    cellp = cp->u.cp;
	    *cells++ = cellp;
	    rv |= size_html_cell(g, cellp, tbl, env);
	    c = findCol(ps, r, c, cellp);
	    cellp->row = r;
	    cellp->col = c;
	    c += cellp->colspan;
	    n_cols = MAX(c, n_cols);
	    n_rows = MAX(r + cellp->rowspan, n_rows);
	    if (inIntSet(is, r + cellp->rowspan))
		cellp->ruled |= HTML_HRULE;
	    cp = (pitem *)dtlink(cdict, cp);
	}
	rp = (pitem *)dtlink(rows, rp);
	r++;
    }
    tbl->row_count = n_rows;
    tbl->column_count = n_cols;
    dtclose(rows);
    dtclose(is);
    freePS(ps);
    return rv;
}

/* Split size x over n pieces with spacing s.
 * We subtract s*(n-1) from x, divide by n and 
 * take the ceiling.
 */
#define SPLIT(x,n,s) (((x) - ((s)-1)*((n)-1)) / (n))

/* sizeLinearArray:
 * Determine sizes of rows and columns. The size of a column is the
 * maximum width of any cell in it. Similarly for rows.
 * A cell spanning columns contributes proportionately to each column
 * it is in.
 */
static void sizeLinearArray(htmltbl_t * tbl)
{
    htmlcell_t *cp;
    htmlcell_t **cells;
    int i;

    tbl->heights = N_NEW(tbl->row_count + 1, double);
    tbl->widths = N_NEW(tbl->column_count + 1, double);

    for (cells = tbl->u.n.cells; *cells; cells++) {
	cp = *cells;
	double ht;
	if (cp->rowspan == 1)
	    ht = cp->data.box.UR.y;
	else {
	    ht = SPLIT(cp->data.box.UR.y, cp->rowspan, tbl->data.space);
	    ht = fmax(ht, 1);
	}
	double wd;
	if (cp->colspan == 1)
	    wd = cp->data.box.UR.x;
	else {
	    wd = SPLIT(cp->data.box.UR.x, cp->colspan, tbl->data.space);
	    wd = fmax(wd, 1);
	}
	for (i = cp->row; i < cp->row + cp->rowspan; i++) {
	    tbl->heights[i] = fmax(tbl->heights[i], ht);
	}
	for (i = cp->col; i < cp->col + cp->colspan; i++) {
	    tbl->widths[i] = fmax(tbl->widths[i], wd);
	}
    }
}

/* closeGraphs:
 * Clean up graphs made for setting column and row widths.
 */
static void closeGraphs(graph_t * rowg, graph_t * colg)
{
    node_t *n;
    for (n = GD_nlist(colg); n; n = ND_next(n)) {
	free_list(ND_in(n));
	free_list(ND_out(n));
    }

    agclose(rowg);
    agclose(colg);
}

/* checkChain:
 * For each pair of nodes in the node list, add an edge if none exists.
 * Assumes node list has nodes ordered correctly.
 */
static void checkChain(graph_t * g)
{
    node_t *t;
    node_t *h;
    edge_t *e;
    t = GD_nlist(g);
    for (h = ND_next(t); h; h = ND_next(h)) {
	if (!agfindedge(g, t, h)) {
	    e = agedge(g, t, h, NULL, 1);
	    agbindrec(e, "Agedgeinfo_t", sizeof(Agedgeinfo_t), true);
	    ED_minlen(e) = 0;
	    elist_append(e, ND_out(t));
	    elist_append(e, ND_in(h));
	}
	t = h;
    }
}

/* checkEdge:
 * Check for edge in g. If it exists, set its minlen to max of sz and
 * current minlen. Else, create it and set minlen to sz.
 */
static void
checkEdge (graph_t* g, node_t* t, node_t* h, int sz)
{
    edge_t* e;

    e = agfindedge (g, t, h);
    if (e)
	ED_minlen(e) = MAX(ED_minlen(e), sz);
    else {
	e = agedge(g, t, h, NULL, 1);
	agbindrec(e, "Agedgeinfo_t", sizeof(Agedgeinfo_t), true);
	ED_minlen(e) = sz;
	elist_append(e, ND_out(t));
	elist_append(e, ND_in(h));
    }
}

/* makeGraphs:
 * Generate dags modeling the row and column constraints.
 * If the table has column_count columns, we create the graph
 *  0 -> 1 -> 2 -> ... -> column_count
 * and if a cell starts in column c with span colspan, with
 * width w, we add the edge c -> c+colspan [minlen = w].
 * Ditto for rows.
 *
 */
static void makeGraphs(htmltbl_t * tbl, graph_t * rowg, graph_t * colg)
{
    htmlcell_t *cp;
    htmlcell_t **cells;
    node_t *t;
    node_t *lastn;
    node_t *h;
    agxbuf value_buffer = {0};

    lastn = NULL;
    for (size_t i = 0; i <= tbl->column_count; i++) {
	agxbprint(&value_buffer, "%" PRISIZE_T, i);
	t = agnode(colg, agxbuse(&value_buffer), 1);
	agbindrec(t, "Agnodeinfo_t", sizeof(Agnodeinfo_t), true);
	alloc_elist(tbl->row_count, ND_in(t));
	alloc_elist(tbl->row_count, ND_out(t));
	if (lastn) {
	    ND_next(lastn) = t;
	    lastn = t;
	} else {
	    lastn = GD_nlist(colg) = t;
	}
    }
    lastn = NULL;
    for (size_t i = 0; i <= tbl->row_count; i++) {
	agxbprint(&value_buffer, "%" PRISIZE_T, i);
	t = agnode(rowg, agxbuse(&value_buffer), 1);
	agbindrec(t, "Agnodeinfo_t", sizeof(Agnodeinfo_t), true);
	alloc_elist(tbl->column_count, ND_in(t));
	alloc_elist(tbl->column_count, ND_out(t));
	if (lastn) {
	    ND_next(lastn) = t;
	    lastn = t;
	} else {
	    lastn = GD_nlist(rowg) = t;
	}
    }

    for (cells = tbl->u.n.cells; *cells; cells++) {
	cp = *cells;
	agxbprint(&value_buffer, "%" PRIu16, cp->col);
	t = agfindnode(colg, agxbuse(&value_buffer));
	agxbprint(&value_buffer, "%d", cp->col + cp->colspan);
	h = agfindnode(colg, agxbuse(&value_buffer));
	checkEdge (colg, t, h, cp->data.box.UR.x);

	agxbprint(&value_buffer, "%" PRIu16, cp->row);
	t = agfindnode(rowg, agxbuse(&value_buffer));
	agxbprint(&value_buffer, "%d", cp->row + cp->rowspan);
	h = agfindnode(rowg, agxbuse(&value_buffer));
	checkEdge (rowg, t, h, cp->data.box.UR.y);
    }

    agxbfree(&value_buffer);

    /* Make sure that 0 <= 1 <= 2 ...k. This implies graph connected. */
    checkChain(colg);
    checkChain(rowg);
}

/* setSizes:
 * Use rankings to determine cell dimensions. The rank values
 * give the coordinate, so to get the width/height, we have
 * to subtract the previous value.
 */
static void setSizes(htmltbl_t * tbl, graph_t * rowg, graph_t * colg)
{
    int i;
    node_t *n;
    int prev;

    prev = 0;
    n = GD_nlist(rowg);
    for (i = 0, n = ND_next(n); n; i++, n = ND_next(n)) {
	tbl->heights[i] = ND_rank(n) - prev;
	prev = ND_rank(n);
    }
    prev = 0;
    n = GD_nlist(colg);
    for (i = 0, n = ND_next(n); n; i++, n = ND_next(n)) {
	tbl->widths[i] = ND_rank(n) - prev;
	prev = ND_rank(n);
    }

}

/* sizeArray:
 * Set column and row sizes. Optimize for minimum width and
 * height. Where there is slack, try to distribute evenly.
 * We do this by encoding cells as edges with min length is
 * a dag on a chain. We then run network simplex, using
 * LR_balance.
 */
static void sizeArray(htmltbl_t * tbl)
{
    graph_t *rowg;
    graph_t *colg;
    Agdesc_t dir = Agstrictdirected;

    /* Do the 1D cases by hand */
    if (tbl->row_count == 1 || tbl->column_count == 1) {
	sizeLinearArray(tbl);
	return;
    }

    tbl->heights = N_NEW(tbl->row_count + 1, double);
    tbl->widths = N_NEW(tbl->column_count + 1, double);

    rowg = agopen("rowg", dir, NULL);
    colg = agopen("colg", dir, NULL);
    /* Only need GD_nlist */
    agbindrec(rowg, "Agraphinfo_t", sizeof(Agraphinfo_t), true);	// graph custom data
    agbindrec(colg, "Agraphinfo_t", sizeof(Agraphinfo_t), true);	// graph custom data
    makeGraphs(tbl, rowg, colg);
    rank(rowg, 2, INT_MAX);
    rank(colg, 2, INT_MAX);
    setSizes(tbl, rowg, colg);
    closeGraphs(rowg, colg);
}

static void pos_html_tbl(htmltbl_t *, boxf, int);	/* forward declaration */

/* pos_html_img:
 * Place image in cell
 * storing allowed space handed by parent cell.
 * How this space is used is handled in emit_html_img.
 */
static void pos_html_img(htmlimg_t * cp, boxf pos)
{
    cp->box = pos;
}

/* pos_html_txt:
 * Set default alignment.
 */
static void pos_html_txt(htmltxt_t * ftxt, char c)
{
    for (size_t i = 0; i < ftxt->nspans; i++) {
	if (ftxt->spans[i].just == UNSET_ALIGN)	/* unset */
	    ftxt->spans[i].just = c;
    }
}

/* pos_html_cell:
 */
static void pos_html_cell(htmlcell_t * cp, boxf pos, int sides)
{
    double delx, dely;
    pointf oldsz;
    boxf cbox;

    if (!cp->data.pencolor && cp->parent->data.pencolor)
	cp->data.pencolor = gv_strdup(cp->parent->data.pencolor);

    /* If fixed, align cell */
    if (cp->data.flags & FIXED_FLAG) {
	oldsz = cp->data.box.UR;
	delx = pos.UR.x - pos.LL.x - oldsz.x;
	if (delx > 0) {
	    switch (cp->data.flags & HALIGN_MASK) {
	    case HALIGN_LEFT:
		pos.UR.x = pos.LL.x + oldsz.x;
		break;
	    case HALIGN_RIGHT:
		pos.UR.x += delx;
		pos.LL.x += delx;
		break;
	    default:
		pos.LL.x += delx / 2;
		pos.UR.x -= delx / 2;
		break;
	    }
	}
	dely = pos.UR.y - pos.LL.y - oldsz.y;
	if (dely > 0) {
	    switch (cp->data.flags & VALIGN_MASK) {
	    case VALIGN_BOTTOM:
		pos.UR.y = pos.LL.y + oldsz.y;
		break;
	    case VALIGN_TOP:
		pos.UR.y += dely;
		pos.LL.y += dely;
		break;
	    default:
		pos.LL.y += dely / 2;
		pos.UR.y -= dely / 2;
		break;
	    }
	}
    }
    cp->data.box = pos;
    cp->data.sides = sides;

    /* set up child's position */
    cbox.LL.x = pos.LL.x + cp->data.border + cp->data.pad;
    cbox.LL.y = pos.LL.y + cp->data.border + cp->data.pad;
    cbox.UR.x = pos.UR.x - cp->data.border - cp->data.pad;
    cbox.UR.y = pos.UR.y - cp->data.border - cp->data.pad;

    if (cp->child.kind == HTML_TBL) {
	pos_html_tbl(cp->child.u.tbl, cbox, sides);
    } else if (cp->child.kind == HTML_IMAGE) {
	/* Note that alignment trumps scaling */
	oldsz = cp->child.u.img->box.UR;
	delx = cbox.UR.x - cbox.LL.x - oldsz.x;
	if (delx > 0) {
	    switch (cp->data.flags & HALIGN_MASK) {
	    case HALIGN_LEFT:
		cbox.UR.x -= delx;
		break;
	    case HALIGN_RIGHT:
		cbox.LL.x += delx;
		break;
	    }
	}

	dely = cbox.UR.y - cbox.LL.y - oldsz.y;
	if (dely > 0) {
	    switch (cp->data.flags & VALIGN_MASK) {
	    case VALIGN_BOTTOM:
		cbox.UR.y -= dely;
		break;
	    case VALIGN_TOP:
		cbox.LL.y += dely;
		break;
	    }
	}
	pos_html_img(cp->child.u.img, cbox);
    } else {
	char dfltalign;
	int af;

	oldsz = cp->child.u.txt->box.UR;
	delx = cbox.UR.x - cbox.LL.x - oldsz.x;
	/* If the cell is larger than the text block and alignment is 
	 * done at textblock level, the text box is shrunk accordingly. 
	 */
	if (delx > 0 && (af = (cp->data.flags & HALIGN_MASK)) != HALIGN_TEXT) {
	    switch (af) {
	    case HALIGN_LEFT:
		cbox.UR.x -= delx;
		break;
	    case HALIGN_RIGHT:
		cbox.LL.x += delx;
		break;
	    default:
		cbox.LL.x += delx / 2;
		cbox.UR.x -= delx / 2;
		break;
	    }
	}

	dely = cbox.UR.y - cbox.LL.y - oldsz.y;
	if (dely > 0) {
	    switch (cp->data.flags & VALIGN_MASK) {
	    case VALIGN_BOTTOM:
		cbox.UR.y -= dely;
		break;
	    case VALIGN_TOP:
		cbox.LL.y += dely;
		break;
	    default:
		cbox.LL.y += dely / 2;
		cbox.UR.y -= dely / 2;
		break;
	    }
	}
	cp->child.u.txt->box = cbox;

	/* Set default text alignment
	 */
	switch (cp->data.flags & BALIGN_MASK) {
	case BALIGN_LEFT:
	    dfltalign = 'l';
	    break;
	case BALIGN_RIGHT:
	    dfltalign = 'r';
	    break;
	default:
	    dfltalign = 'n';
	    break;
	}
	pos_html_txt(cp->child.u.txt, dfltalign);
    }
}

/* pos_html_tbl:
 * Position table given its box, then calculate
 * the position of each cell. In addition, set the sides
 * attribute indicating which external sides of the node
 * are accessible to the table.
 */
static void pos_html_tbl(htmltbl_t * tbl, boxf pos, int sides)
{
    int plus;
    htmlcell_t **cells = tbl->u.n.cells;
    htmlcell_t *cp;
    boxf cbox;

    if (tbl->u.n.parent && tbl->u.n.parent->data.pencolor
	&& !tbl->data.pencolor)
	tbl->data.pencolor = gv_strdup(tbl->u.n.parent->data.pencolor);

    double oldsz = tbl->data.box.UR.x;
    double delx = fmax(pos.UR.x - pos.LL.x - oldsz, 0);
    oldsz = tbl->data.box.UR.y;
    double dely = fmax(pos.UR.y - pos.LL.y - oldsz, 0);

    /* If fixed, align box */
    if (tbl->data.flags & FIXED_FLAG) {
	if (delx > 0) {
	    switch (tbl->data.flags & HALIGN_MASK) {
	    case HALIGN_LEFT:
		pos.UR.x = pos.LL.x + oldsz;
		break;
	    case HALIGN_RIGHT:
		pos.UR.x += delx;
		pos.LL.x += delx;
		break;
	    default:
		pos.LL.x += delx / 2;
		pos.UR.x -= delx / 2;
		break;
	    }
	    delx = 0;
	}
	if (dely > 0) {
	    switch (tbl->data.flags & VALIGN_MASK) {
	    case VALIGN_BOTTOM:
		pos.UR.y = pos.LL.y + oldsz;
		break;
	    case VALIGN_TOP:
		pos.LL.y += dely;
		pos.UR.y = pos.LL.y + oldsz;
		break;
	    default:
		pos.LL.y += dely / 2;
		pos.UR.y -= dely / 2;
		break;
	    }
	    dely = 0;
	}
    }

    /* change sizes to start positions and distribute extra space */
    double x = pos.LL.x + tbl->data.border + tbl->data.space;
    assert(tbl->column_count <= DBL_MAX);
    double extra = delx / (double)tbl->column_count;
    plus = ROUND(delx - extra * (double)tbl->column_count);
    for (size_t i = 0; i <= tbl->column_count; i++) {
	delx = tbl->widths[i] + extra + ((i <= INT_MAX && (int)i < plus) ? 1 : 0);
	tbl->widths[i] = x;
	x += delx + tbl->data.space;
    }
    double y = pos.UR.y - tbl->data.border - tbl->data.space;
    assert(tbl->row_count <= DBL_MAX);
    extra = dely / (double)tbl->row_count;
    plus = ROUND(dely - extra * (double)tbl->row_count);
    for (size_t i = 0; i <= tbl->row_count; i++) {
	dely = tbl->heights[i] + extra + ((i <= INT_MAX && (int)i < plus) ? 1 : 0);
	tbl->heights[i] = y;
	y -= dely + tbl->data.space;
    }

    while ((cp = *cells++)) {
	int mask = 0;
	if (sides) {
	    if (cp->col == 0)
		mask |= LEFT;
	    if (cp->row == 0)
		mask |= TOP;
	    if (cp->col + cp->colspan == tbl->column_count)
		mask |= RIGHT;
	    if (cp->row + cp->rowspan == tbl->row_count)
		mask |= BOTTOM;
	}
	cbox.LL.x = tbl->widths[cp->col];
	cbox.UR.x = tbl->widths[cp->col + cp->colspan] - tbl->data.space;
	cbox.UR.y = tbl->heights[cp->row];
	cbox.LL.y = tbl->heights[cp->row + cp->rowspan] + tbl->data.space;
	pos_html_cell(cp, cbox, sides & mask);
    }

    tbl->data.sides = sides;
    tbl->data.box = pos;
}

/* size_html_tbl:
 * Determine the size of a table by first determining the
 * size of each cell.
 */
static int
size_html_tbl(graph_t * g, htmltbl_t * tbl, htmlcell_t * parent,
	      htmlenv_t * env)
{
    int rv = 0;
    static textfont_t savef;

    if (tbl->font)
	pushFontInfo(env, tbl->font, &savef);
    tbl->u.n.parent = parent;
    rv = processTbl(g, tbl, env);

    /* Set up border and spacing */
    if (!(tbl->data.flags & SPACE_SET)) {
	tbl->data.space = DEFAULT_CELLSPACING;
    }
    if (!(tbl->data.flags & BORDER_SET)) {
	tbl->data.border = DEFAULT_BORDER;
    }

    sizeArray(tbl);

    assert(tbl->column_count <= DBL_MAX);
    double wd = ((double)tbl->column_count + 1) * tbl->data.space + 2 * tbl->data.border;
    assert(tbl->row_count <= DBL_MAX);
    double ht = ((double)tbl->row_count + 1) * tbl->data.space + 2 * tbl->data.border;
    for (size_t i = 0; i < tbl->column_count; i++)
	wd += tbl->widths[i];
    for (size_t i = 0; i < tbl->row_count; i++)
	ht += tbl->heights[i];

    if (tbl->data.flags & FIXED_FLAG) {
	if (tbl->data.width && tbl->data.height) {
	    if (tbl->data.width < wd || tbl->data.height < ht) {
		agerr(AGWARN, "table size too small for content\n");
		rv = 1;
	    }
	    wd = 0;
	    ht = 0;
	} else {
	    agerr(AGWARN,
		  "fixed table size with unspecified width or height\n");
	    rv = 1;
	}
    }
    tbl->data.box.UR.x = fmax(wd, tbl->data.width);
    tbl->data.box.UR.y = fmax(ht, tbl->data.height);

    if (tbl->font)
	popFontInfo(env, &savef);
    return rv;
}

static char *nameOf(void *obj, agxbuf * xb)
{
    Agedge_t *ep;
    switch (agobjkind(obj)) {
    case AGRAPH:
	agxbput(xb, agnameof(obj));
	break;
    case AGNODE:
	agxbput(xb, agnameof(obj));
	break;
    case AGEDGE:
	ep = obj;
	agxbput(xb, agnameof(agtail(ep)));
	agxbput(xb, agnameof(aghead(ep)));
	if (agisdirected(agraphof(aghead(ep))))
	    agxbput(xb, "->");
	else
	    agxbput(xb, "--");
	break;
    }
    return agxbuse(xb);
}

#ifdef DEBUG
void indent(int i)
{
    while (i--)
	fprintf(stderr, "  ");
}

void printBox(boxf b)
{
    fprintf(stderr, "(%f,%f)(%f,%f)", b.LL.x, b.LL.y, b.UR.x, b.UR.y);
}

void printImage(htmlimg_t * ip, int ind)
{
    indent(ind);
    fprintf(stderr, "img: %s\n", ip->src);
}

void printTxt(htmltxt_t * txt, int ind)
{
    indent(ind);
    fprintf(stderr, "txt spans = %" PRISIZE_T " \n", txt->nspans);
    for (size_t i = 0; i < txt->nspans; i++) {
	indent(ind + 1);
	fprintf(stderr, "[%" PRISIZE_T "] %" PRISIZE_T " items\n", i,
	        txt->spans[i].nitems);
	for (size_t j = 0; j < txt->spans[i].nitems; j++) {
	    indent(ind + 2);
	    fprintf(stderr, "[%" PRISIZE_T "] (%f,%f) \"%s\" ",
		    j, txt->spans[i].items[j].size.x,
            txt->spans[i].items[j].size.y,
		    txt->spans[i].items[j].str);
	    if (txt->spans[i].items[j].font)
		fprintf(stderr, "font %s color %s size %f\n",
			txt->spans[i].items[j].font->name,
			txt->spans[i].items[j].font->color,
			txt->spans[i].items[j].font->size);
	    else
		fprintf(stderr, "\n");
	}
    }
}

void printData(htmldata_t * dp)
{
    unsigned char flags = dp->flags;
    char c;

    fprintf(stderr, "s%d(%d) ", dp->space, (flags & SPACE_SET ? 1 : 0));
    fprintf(stderr, "b%d(%d) ", dp->border, (flags & BORDER_SET ? 1 : 0));
    fprintf(stderr, "p%d(%d) ", dp->pad, (flags & PAD_SET ? 1 : 0));
    switch (flags & HALIGN_MASK) {
    case HALIGN_RIGHT:
	c = 'r';
	break;
    case HALIGN_LEFT:
	c = 'l';
	break;
    default:
	c = 'n';
	break;
    }
    fprintf(stderr, "%c", c);
    switch (flags & VALIGN_MASK) {
    case VALIGN_TOP:
	c = 't';
	break;
    case VALIGN_BOTTOM:
	c = 'b';
	break;
    default:
	c = 'c';
	break;
    }
    fprintf(stderr, "%c ", c);
    printBox(dp->box);
}

void printTbl(htmltbl_t * tbl, int ind)
{
    htmlcell_t **cells = tbl->u.n.cells;
    indent(ind);
    fprintf(stderr, "tbl (%p) %" PRISIZE_T " %" PRISIZE_T " ", tbl, tbl->column_count, tbl->row_count);
    printData(&tbl->data);
    fputs("\n", stderr);
    while (*cells)
	printCell(*cells++, ind + 1);
}

static void printCell(htmlcell_t * cp, int ind)
{
    indent(ind);
    fprintf(stderr, "cell %" PRIu16 " %" PRIu16 " %" PRIu16 " %" PRIu16 " ", cp->colspan,
            cp->colspan, cp->rowspan, cp->col, cp->row);
    printData(&cp->data);
    fputs("\n", stderr);
    switch (cp->child.kind) {
    case HTML_TBL:
	printTbl(cp->child.u.tbl, ind + 1);
	break;
    case HTML_TEXT:
	printTxt(cp->child.u.txt, ind + 1);
	break;
    case HTML_IMAGE:
	printImage(cp->child.u.img, ind + 1);
	break;
    default:
	break;
    }
}

void printLbl(htmllabel_t * lbl)
{
    if (lbl->kind == HTML_TBL)
	printTbl(lbl->u.tbl, 0);
    else
	printTxt(lbl->u.txt, 0);
}
#endif				/* DEBUG */

static char *getPenColor(void *obj)
{
    char *str;

    if ((str = agget(obj, "pencolor")) != 0 && str[0])
	return str;
    else if ((str = agget(obj, "color")) != 0 && str[0])
	return str;
    else
	return NULL;
}

/* make_html_label:
 * Return non-zero if problem parsing HTML. In this case, use object name.
 */
int make_html_label(void *obj, textlabel_t * lp)
{
    int rv;
    double wd2, ht2;
    graph_t *g;
    htmllabel_t *lbl;
    htmlenv_t env;
    char *s;

    env.obj = obj;
    switch (agobjkind(obj)) {
    case AGRAPH:
	env.g = ((Agraph_t *) obj)->root;
	break;
    case AGNODE:
	env.g = agraphof(obj);
	break;
    case AGEDGE:
	env.g = agraphof(aghead(((Agedge_t *) obj)));
	break;
    }
    g = env.g->root;

    env.finfo.size = lp->fontsize;
    env.finfo.name = lp->fontname;
    env.finfo.color = lp->fontcolor;
    env.finfo.flags = 0;
    lbl = parseHTML(lp->text, &rv, &env);
    if (!lbl) {
	if (rv == 3) {
	    // fatal error
	    graphviz_exit(EXIT_FAILURE);
	}
	/* Parse of label failed; revert to simple text label */
	agxbuf xb = {0};
	lp->html = false;
	lp->text = gv_strdup(nameOf(obj, &xb));
	switch (lp->charset) {
	case CHAR_LATIN1:
	    s = latin1ToUTF8(lp->text);
	    break;
	default:		/* UTF8 */
	    s = htmlEntityUTF8(lp->text, env.g);
	    break;
	}
	free(lp->text);
	lp->text = s;
	make_simple_label(GD_gvc(g), lp);
	agxbfree(&xb);
	return rv;
    }

    if (lbl->kind == HTML_TBL) {
	if (!lbl->u.tbl->data.pencolor && getPenColor(obj))
	    lbl->u.tbl->data.pencolor = gv_strdup(getPenColor(obj));
	rv |= size_html_tbl(g, lbl->u.tbl, NULL, &env);
	wd2 = lbl->u.tbl->data.box.UR.x / 2;
	ht2 = lbl->u.tbl->data.box.UR.y / 2;
	boxf b = {{-wd2, -ht2}, {wd2, ht2}};
	pos_html_tbl(lbl->u.tbl, b, BOTTOM | RIGHT | TOP | LEFT);
	lp->dimen.x = b.UR.x - b.LL.x;
	lp->dimen.y = b.UR.y - b.LL.y;
    } else {
	rv |= size_html_txt(GD_gvc(g), lbl->u.txt, &env);
	wd2 = lbl->u.txt->box.UR.x  / 2;
	ht2 = lbl->u.txt->box.UR.y  / 2;
	boxf b = {{-wd2, -ht2}, {wd2, ht2}};
	lbl->u.txt->box = b;
	lp->dimen.x = b.UR.x - b.LL.x;
	lp->dimen.y = b.UR.y - b.LL.y;
    }

    lp->u.html = lbl;

    /* If the label is a table, replace label text because this may
     * be used for the title and alt fields in image maps.
     */
    if (lbl->kind == HTML_TBL) {
	free(lp->text);
	lp->text = gv_strdup("<TABLE>");
    }

    return rv;
}
