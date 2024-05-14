/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/


/* dbg.c:
 * Written by Emden R. Gansner
 *
 * Simple debugging infrastructure
 */
#ifdef DEBUG

#define FDP_PRIVATE

#include <cgraph/exit.h>
#include <fdpgen/dbg.h>
#include <neatogen/neatoprocs.h>
#include <fdpgen/fdp.h>
#include <math.h>

static int indent = -1;

void incInd()
{
    indent++;
}

void decInd()
{
    if (indent >= 0)
	indent--;
}

void prIndent(void)
{
    int i;
    for (i = 0; i < indent; i++)
	fputs("  ", stderr);
}

void prEdge(edge_t *e,char *s)
{
	fprintf(stderr,"%s --", agnameof(agtail(e)));
	fprintf(stderr,"%s%s", agnameof(aghead(e)),s);
}

static void dumpBB(graph_t * g)
{
    boxf bb;
    boxf b;

    bb = BB(g);
    b = GD_bb(g);
    prIndent();
    fprintf(stderr, "  LL (%f,%f)  UR (%f,%f)\n", bb.LL.x, bb.LL.y,
	    bb.UR.x, bb.UR.y);
    prIndent();
    fprintf(stderr, "  LL (%f,%f)  UR (%f,%f)\n", b.LL.x, b.LL.y,
	    b.UR.x, b.UR.y);
}

static void dumpSG(graph_t * g)
{
    graph_t *subg;
    int i;

    if (GD_n_cluster(g) == 0)
	return;
    prIndent();
    fprintf(stderr, "  {\n");
    for (i = 1; i <= GD_n_cluster(g); i++) {
	subg = (GD_clust(g))[i];
	prIndent();
	fprintf(stderr, "  subgraph %s : %d nodes\n", agnameof(subg),
		agnnodes(subg));
	dumpBB(subg);
	incInd ();
	dumpSG(subg);
	decInd ();
    }
    prIndent();
    fprintf(stderr, "  }\n");
}

/* dump:
 */
void dump(graph_t *g, int level) {
    node_t *n;
    boxf bb;
    double w, h;
    pointf pos;

    if (Verbose < level)
	return;
    prIndent();
    fprintf(stderr, "Graph %s : %d nodes\n", agnameof(g), agnnodes(g));
    dumpBB(g);
    if (Verbose > level) {
	incInd();
	dumpSG(g);
	decInd();
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    pos.x = ND_pos(n)[0];
	    pos.y = ND_pos(n)[1];
	    prIndent();
	    w = ND_width(n);
	    h = ND_height(n);
	    fprintf(stderr, "%s: (%f,%f) (%f,%f) \n",
	            agnameof(n), pos.x, pos.y, w, h);
	}
    }
}

void dumpG(graph_t * g, char *fname, int expMode)
{
    FILE *fp;

    fp = fopen(fname, "w");
    if (!fp) {
	fprintf(stderr, "Couldn not open %s \n", fname);
	graphviz_exit(1);
    }
    outputGraph(g, fp, expMode);
    fclose(fp);
}

static const double ArrowScale = 1.0;

#define         ARROW_LENGTH    10
#define         ARROW_WIDTH      5

static char *plog = "%!PS-Adobe-2.0\n\n\
/Times-Roman findfont 14 scalefont setfont\n\
/lLabel {\n\
\tmoveto\n\
\tgsave\n\
\tshow\n\
\tgrestore\n\
} def\n\
/inch {\n\
\t72 mul\n\
} def\n\
/doBox {\n\
\tnewpath\n\
\tmoveto\n\
\t/ht exch def\n\
\t/wd exch def\n\
\t0 ht rlineto\n\
\twd 0 rlineto\n\
\t0 0 ht sub rlineto\n\
\tclosepath\n\
\tgsave\n\
\t\t.9 setgray\n\
\t\tfill\n\
\tgrestore\n\
\tstroke\n\
} def\n\
/drawCircle {\n\
\t/r exch def\n\
\t/y exch def\n\
\t/x exch def\n\
\tnewpath\n\
\tx y r 0 360 arc\n\
\tstroke\n\
} def\n\
/fillCircle {\n\
\t/r exch def\n\
\t/y exch def\n\
\t/x exch def\n\
\tnewpath\n\
\tx y r 0 360 arc\n\
\tfill\n\
} def\n";

static char *elog = "showpage\n";

static double PSWidth = 550.0;
static double PSHeight = 756.0;

static void pswrite(Agraph_t * g, FILE * fp, int expMode)
{
    Agnode_t *n;
    Agnode_t *h;
    Agedge_t *e;
    double minx, miny, maxx, maxy;
    double scale, width, height;
    int do_arrow;
    int angle;
    char *p;
    double theta;
    double arrow_w, arrow_l;
    int portColor;

    fprintf(fp, "%s", plog);

    do_arrow = 0;

    n = agfstnode(g);
    minx = ND_pos(n)[0];
    miny = ND_pos(n)[1];
    maxx = ND_pos(n)[0];
    maxy = ND_pos(n)[1];
    n = agnxtnode(g, n);
    for (; n; n = agnxtnode(g, n)) {
	minx = fmin(minx, ND_pos(n)[0]);
	miny = fmin(miny, ND_pos(n)[1]);
	maxx = fmax(maxx, ND_pos(n)[0]);
	maxy = fmax(maxy, ND_pos(n)[1]);
    }

    /* Convert to points
     */
    minx *= POINTS_PER_INCH;
    miny *= POINTS_PER_INCH;
    maxx *= POINTS_PER_INCH;
    maxy *= POINTS_PER_INCH;

    /* Check for rotation
     */
    if ((p = agget(g, "rotate")) && *p != '\0' && (angle = atoi(p)) != 0) {
	fprintf(fp, "306 396 translate\n");
	fprintf(fp, "%d rotate\n", angle);
	fprintf(fp, "-306 -396 translate\n");
    }

    /* If user gives scale factor, use it.
     * Else if figure too large for standard PS page, scale it to fit.
     */
    width = maxx - minx + 20;
    height = maxy - miny + 20;
    if (width > PSWidth) {
	if (height > PSHeight) {
	    scale = fmin(PSWidth / width, PSHeight / height);
	} else
	    scale = PSWidth / width;
    } else if (height > PSHeight) {
	scale = PSHeight / height;
    } else
	scale = 1.0;

    fprintf(fp, "%f %f translate\n",
	    (PSWidth - scale * (minx + maxx)) / 2.0,
	    (PSHeight - scale * (miny + maxy)) / 2.0);
    fprintf(fp, "%f %f scale\n", scale, scale);

/*
    if (Verbose)
      fprintf (stderr, "Region (%f,%f) (%f,%f), scale %f\n", 
        minx, miny, maxx, maxy, scale);
*/

    if (do_arrow) {
	arrow_w = ArrowScale * ARROW_WIDTH / scale;
	arrow_l = ArrowScale * ARROW_LENGTH / scale;
    }

    fprintf(fp, "0.0 setlinewidth\n");
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (IS_PORT(n)) {
	    double r;
	    r = sqrt(ND_pos(n)[0] * ND_pos(n)[0] +
		     ND_pos(n)[1] * ND_pos(n)[1]);
	    fprintf(fp, "0 0 %f inch drawCircle\n", r);
	    break;
	}
    }

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    h = aghead(e);
	    fprintf(fp, "%f inch %f inch moveto %f inch %f inch lineto\n",
		    ND_pos(n)[0], ND_pos(n)[1], ND_pos(h)[0],
		    ND_pos(h)[1]);
	    fprintf(fp, "stroke\n");
	    if (do_arrow) {
		theta =
		    atan2(ND_pos(n)[1] - ND_pos(h)[1],
			  ND_pos(n)[0] - ND_pos(h)[0]);
		fprintf(fp, "%f %f %.2f %.2f %.2f doArrow\n",
			ND_pos(h)[0], ND_pos(h)[1], DEGREES(theta),
			arrow_l, arrow_w);
	    }

	}
    }

#ifdef BOX
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	float wd, ht;

	data = getData(n);
	wd = data->wd;
	ht = data->ht;
	fprintf(fp, "%f %f %f %f doBox\n", wd, ht,
		data->pos.x - wd / 2, data->pos.y - ht / 2);
    }
#else
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	fprintf(fp, "%% %s\n", agnameof(n));
	if (expMode) {
	    double wd, ht;
	    double r;
	    wd = ND_width(n);
	    ht = ND_height(n);
	    r = sqrt(wd * wd / 4 + ht * ht / 4);
	    fprintf(fp, "%f inch %f inch %f inch %f inch doBox\n", wd, ht,
		    ND_pos(n)[0] - wd / 2, ND_pos(n)[1] - ht / 2);
	    fprintf(fp, "%f inch %f inch %f inch drawCircle\n",
		    ND_pos(n)[0], ND_pos(n)[1], r);
	} else {
	    if (IS_PORT(n)) {
		if (!portColor) {
		    fprintf(fp, "0.667 1.000 1.000 sethsbcolor\n");
		    portColor = 1;
		}
	    } else {
		if (portColor) {
		    fprintf(fp, "0.0 0.000 0.000 sethsbcolor\n");
		    portColor = 0;
		}
	    }
	}
	fprintf(fp, "%f inch %f inch %f fillCircle\n", ND_pos(n)[0],
		ND_pos(n)[1], 3 / scale);
    }
#endif

    fprintf(fp, "0.667 1.000 1.000 sethsbcolor\n");
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    h = aghead(e);
	    fprintf(fp, "%f inch %f inch moveto %f inch %f inch lineto\n",
		    ND_pos(n)[0], ND_pos(n)[1], ND_pos(h)[0],
		    ND_pos(h)[1]);
	    fprintf(fp, "stroke\n");
	    if (do_arrow) {
		theta =
		    atan2(ND_pos(n)[1] - ND_pos(h)[1],
			  ND_pos(n)[0] - ND_pos(h)[0]);
		fprintf(fp, "%f %f %.2f %.2f %.2f doArrow\n",
			ND_pos(h)[0], ND_pos(h)[1], DEGREES(theta),
			arrow_l, arrow_w);
	    }

	}
    }

    fprintf(fp, "%s", elog);
}

void outputGraph(Agraph_t * g, FILE * fp, int expMode)
{
    pswrite(g, fp, expMode);
}

#endif				/* DEBUG */
