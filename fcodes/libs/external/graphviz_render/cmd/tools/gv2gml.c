/**
 * @file
 * @brief DOT-GML converter
 */

/**********************************************************
*      This software is part of the graphviz package      *
*                  https://graphviz.org                   *
*                                                         *
*            Copyright (c) 1994-2004 AT&T Corp.           *
*                and is licensed under the                *
*            Common Public License, Version 1.0           *
*                      by AT&T Corp.                      *
*                                                         *
*        Information and Software Systems Research        *
*              AT&T Research, Florham Park NJ             *
**********************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <getopt.h>

#include <cgraph/cgraph.h>
#include <cgraph/exit.h>
#include <cgraph/strview.h>
#include <cgraph/tokenize.h>
#include <cgraph/unreachable.h>
#include <common/types.h>
#include <common/utils.h>
#include <ctype.h>
#include <ingraphs/ingraphs.h>
#include "openFile.h"

static FILE *outFile;
static char *CmdName;
static char **Files;
static uint64_t id;

#define streq(s,t) (!strcmp(s,t))

#define POS_SET (1<<0)
#define W_SET   (1<<1)
#define H_SET   (1<<2)
#define INVIS   (1<<3)
#define FILL    (1<<4)
#define LINE    (1<<5)
#define DASH    (1<<6)
#define DOT     (1<<7)
#define BOLD    (1<<8)

typedef struct {
    unsigned int flags;
/* graphics */
    double x;
    double y;
    double w;
    double h;
    char* type;    /* shape */
    char* image;
    char* fill;    /* fillcolor */
    char* outline; /* pencolor */
    char* width;   /* penwidth */
    char* outlineStyle;
/* label graphics */
    char* fontColor;
    char* fontSize;
    char* fontName;
} node_attrs;

typedef struct {
    unsigned int flags;
/* graphics */
    char* width;   /* penwidth */
    char* fill;    /* pencolor */
    char* arrow;   /* dir */
    char* arrowhead;   /* arrowhead */
    char* arrowtail;   /* arrowtail */
    char* pos;
/* label graphics */
    char* fontColor;
    char* fontSize;
    char* fontName;
} edge_attrs;

typedef struct {
    Agrec_t h;
    uint64_t id;
} Local_Agnodeinfo_t;

#define ID(n)  (((Local_Agnodeinfo_t*)(n->base.data))->id)

static void indent(int ix) {
    while (ix--)
	fprintf (outFile, "  ");
}

/* isNumber:
 * Return true if input string is number
 */
static int
isNumber (char* s)
{
    char* ep = s;
    strtod(s, &ep);
    if (s != ep) {
	while (*ep && isspace((int)*ep)) ep++;
	if (*ep) return 0;
	else return 1;
    }
    else
	return 0;
}

/* parseStyle:
 * Simple implementation for parsing style attribute
 */
static int 
parseStyle (char* s)
{
    int flags = 0;
    char* sep = " \t,";

    for (tok_t t = tok(s, sep); !tok_end(&t); tok_next(&t)) {
	strview_t ip = tok_get(&t);
	if (strview_str_eq(ip, "invis")) flags |= INVIS;
	else if (strview_str_eq(ip, "filled")) flags |= FILL;
	else if (strview_str_eq(ip, "dashed")) flags |= DASH;
	else if (strview_str_eq(ip, "dotted")) flags |= DOT;
	else if (strview_str_eq(ip, "solid")) flags |= LINE;
	else if (strview_str_eq(ip, "bold")) flags |= BOLD;
    }  

    return flags;
}

static void emitInt(char *name, int value, int ix) {
    indent(ix);
    fprintf (outFile, "%s %d\n", name, value);
}

static void emitReal(char *name, double value, int ix) {
    indent(ix);
    fprintf (outFile, "%s %g\n", name, value);
}

static void emitPoint(double x, double y, int ix) {
    indent(ix);
    fprintf (outFile, "point [ x %g y %g ]\n", x, y);
}

static char*
skipWS (char* s)
{
    while (isspace((int)*s)) s++;
    return s;
}

/* Return NULL if unsuccessful 
 */
static char*
readPoint (char* s, double* xp, double* yp)
{
    char* endp;

    s = skipWS(s);
    *xp = strtod (s, &endp);
    if (s == endp) return NULL;
    endp++;  /* skip comma */ 
    s = endp;
    *yp = strtod (s, &endp);
    if (s == endp) return NULL;
    else return endp;
}

static char*
arrowEnd (char* s0, char* pfx, int* fp, double* xp, double* yp)
{
    char* s = skipWS(s0);

    if (strncmp(s,pfx,2)) return s;
    s += 2;  /* skip prefix */
    s = readPoint (s, xp, yp);
    if (s == NULL) {
	fprintf (stderr, "Illegal spline end: %s\n", s0);
	graphviz_exit(1);
    }
    *fp = 1;
    return s;
}

static void emitSpline(char *s, int ix) {
    double sx, sy, ex, ey;
    int sarrow = 0;
    int earrow = 0;

    s = arrowEnd (s, "e,", &earrow, &ex, &ey); 
    s = arrowEnd (s, "s,", &sarrow, &sx, &sy); 
    indent(ix);
    fprintf (outFile, "Line [\n");
    if (sarrow)
	emitPoint(sx, sy, ix+1);
    while ((s = readPoint (s, &sx, &sy)))
	emitPoint(sx, sy, ix+1);
    if (earrow)
	emitPoint(ex, ey, ix+1);
    indent(ix);
    fprintf (outFile, "]\n");

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

static void emitAttr(char *name, char *value, int ix) {
    indent(ix);
    if (isNumber (value))
	fprintf (outFile, "%s %s\n", name, value);
    else {
	fprintf(outFile, "%s \"", name);
	xml_puts(outFile, value);
	fputs("\"\n", outFile);
    }
}

/* node attributes:
 * label
 * graphics
 * LabelGraphics
 */
static void  emitNodeAttrs(Agraph_t *G, Agnode_t *np, int ix) {
    Agsym_t*  s;
    char* v;
    node_attrs attrs;
    int doGraphics = 0;
    int doLabelGraphics = 0;
    char* label = 0;
    int style;
    double x, y;

    /* First, process the attributes, saving the graphics attributes */
    memset(&attrs,0, sizeof(attrs));
    for (s = agnxtattr (G, AGNODE, NULL); s; s = agnxtattr (G, AGNODE, s)) {
	if (streq(s->name, "style")) { /* hasFill outlineStyle invis */
	    if (*(v = agxget (np, s))) {
		style = parseStyle (v);
		if (style & INVIS)
		    attrs.flags |= INVIS;
		if (style & FILL)
		    attrs.flags |= FILL;
		if (style & LINE)
		    attrs.outlineStyle = "line";
		if (style & DASH)
		    attrs.outlineStyle = "dashed";
		if (style & DOT)
		    attrs.outlineStyle = "dotted";
		doGraphics = 1;
	    }
	}
	else if (streq(s->name, "label")) {
	    v = agxget (np, s);
	    if (streq("\\N", v)) {
		label = agnameof(np);
		emitAttr(s->name, label, ix);
		doLabelGraphics = 1;
	    }
	    else if (*v) {
		label = v;
		emitAttr(s->name, label, ix);
		doLabelGraphics = 1;
	    }
	}
	else if (streq(s->name, "penwidth")) {
	    if (*(v = agxget (np, s))) {
		attrs.width = v;
		doGraphics = 1;
	    }
	}
	else if (streq(s->name, "width")) {
	    v = agxget (np, s);
	    if (*v) {
		attrs.w = 72.0*atof (v);
		attrs.flags |= W_SET;
		doGraphics = 1;
	    }
	}
	else if (streq(s->name, "height")) {
	    v = agxget (np, s);
	    if (*v) {
		attrs.h = 72.0*atof (v);
		attrs.flags |= H_SET;
		doGraphics = 1;
	    }
	}
	else if (streq(s->name, "pos")) {
	    v = agxget (np, s);
	    if (sscanf (v, "%lf,%lf", &x, &y) == 2) {
		doGraphics = 1;
		attrs.x = x;
		attrs.y = y;
		attrs.flags |= POS_SET;
	    }
	}
	else if (streq(s->name, "shape")) { /* type */
	    if (*(v = agxget (np, s))) {
		attrs.type = v;
		doGraphics = 1;
	    }
	}
	else if (streq(s->name, "color")) {
	    if (*(v = agxget (np, s))) {
		attrs.fill = v;
		attrs.outline = v;
		doGraphics = 1;
	    }
	}
	else if (streq(s->name, "fillcolor")) {
	    if (*(v = agxget (np, s))) {
		attrs.fill = v;
		doGraphics = 1;
	    }
	}
	else if (streq(s->name, "pencolor")) {
	    if (*(v = agxget (np, s))) {
		attrs.outline = v;
		doGraphics = 1;
	    }
	}
	else if (streq(s->name, "fontname")) { /* fontName */
	    if (*(v = agxget (np, s))) {
		attrs.fontName = v;
		doLabelGraphics = 1;
	    }
	}
	else if (streq(s->name, "fontsize")) { /* fontSize */
	    if (*(v = agxget (np, s))) {
		attrs.fontSize = v;
		doLabelGraphics = 1;
	    }
	}
	else if (streq(s->name, "fontcolor")) { /* fontColor */
	    if (*(v = agxget (np, s))) {
		attrs.fontColor = v;
		doLabelGraphics = 1;
	    }
	}
	else {
	    v = agxget (np, s);
	    emitAttr(s->name, v, ix);
	}
    }

    /* Then, print them, if any */
    if (doGraphics) {
	fprintf (outFile, "    graphics [\n");
	if (attrs.flags & POS_SET) {
	    emitReal("x", attrs.x, ix+1);
	    emitReal("y", attrs.y, ix+1);
	}
	if (attrs.flags & W_SET) {
	    emitReal("w", attrs.w, ix+1);
	}
	if (attrs.flags & H_SET) {
	    emitReal("H", attrs.h, ix+1);
	}
	if (attrs.flags & INVIS) {
	    emitInt("visible", 0, ix+1);
	}
	if (attrs.flags & FILL) {
	    emitInt("hasFill", 1, ix+1);
	}
	if (attrs.type) {
	    emitAttr("type", attrs.type, ix+1);
	}
	if (attrs.image) {
	    emitAttr("image", attrs.image, ix+1);
	}
	if (attrs.fill) {
	    emitAttr("fill", attrs.fill, ix+1);
	}
	if (attrs.outline) {
	    emitAttr("outline", attrs.outline, ix+1);
	}
	if (attrs.width) {
	    emitAttr("width", attrs.width, ix+1);
	}
	if (attrs.outlineStyle) {
	    emitAttr("outlineStyle", attrs.outlineStyle, ix+1);
	}
	fprintf (outFile, "    ]\n");
    }

    if (doLabelGraphics) {
	fprintf (outFile, "    LabelGraphics [\n");
	if (label) emitAttr("text", label, ix+1);
	if (attrs.fontColor) {
	    emitAttr("fontColor", attrs.fontColor, ix+1);
	}
	if (attrs.fontSize) {
	    emitAttr("fontSize", attrs.fontSize, ix+1);
	}
	if (attrs.fontName) {
	    emitAttr("fontName", attrs.fontName, ix+1);
	}
	fprintf (outFile, "    ]\n");
    }
}

/* emitNode:
 * set node id
 * label, width height, x, y, type fillcolor
 */
static void emitNode(Agraph_t *G, Agnode_t *n) {
    agbindrec(n, "nodeinfo", sizeof(Local_Agnodeinfo_t), true);
    fprintf(outFile, "  node [\n    id %" PRIu64 "\n    name \"%s\"\n", id,
            agnameof(n));
    ID(n) = id++;
    emitNodeAttrs(G, n, 2);
    fprintf (outFile, "  ]\n");

}

/* edge attributes:
 * label
 * graphics
 * LabelGraphics
 */
static void emitEdgeAttrs(Agraph_t *G, Agedge_t *ep, int ix) {
    Agsym_t*  s;
    char* v;
    edge_attrs attrs;
    int doGraphics = 0;
    int doLabelGraphics = 0;
    char* label = 0;
    int style;

    /* First, process the attributes, saving the graphics attributes */
    memset(&attrs,0, sizeof(attrs));
    for (s = agnxtattr (G, AGEDGE, NULL); s; s = agnxtattr (G, AGEDGE, s)) {
	if (streq(s->name, "style")) { /* hasFill outlineStyle invis */
	    if (*(v = agxget (ep, s))) {
		style = parseStyle (v);
		if (style & INVIS)
		    attrs.flags |= INVIS;
		if (style & LINE)
		    attrs.flags |= LINE;
		if (style & DASH)
		    attrs.flags |= DASH;
		if (style & DOT)
		    attrs.flags |= DOT;
		if (style & BOLD)
		    attrs.width = "2";
		doGraphics = 1;
	    }
	}
	else if (streq(s->name, "label")) {
	    if (*(v = agxget (ep, s))) {
		label = v;
		emitAttr(s->name, label, ix);
		doLabelGraphics = 1;
	    }
	}
	else if (streq(s->name, "penwidth")) {
	    if (*(v = agxget (ep, s))) {
		attrs.width = v;
		doGraphics = 1;
	    }
	}
	else if (streq(s->name, "pos")) {
	    if (*(v = agxget (ep, s))) {
		doGraphics = 1;
		attrs.pos = v;
	    }
	}
	else if (streq(s->name, "dir")) {
	    if (*(v = agxget (ep, s))) {
		doGraphics = 1;
		attrs.arrow = v;
	    }
	}
	else if (streq(s->name, "color")) {
	    if (*(v = agxget (ep, s))) {
		attrs.fill = v;
		doGraphics = 1;
	    }
	}
	else if (streq(s->name, "pencolor")) {
	    if (*(v = agxget (ep, s))) {
		attrs.fill = v;
		doGraphics = 1;
	    }
	}
	else if (streq(s->name, "arrowhead")) {
	    if (*(v = agxget (ep, s))) {
		attrs.arrowhead = v;
		doGraphics = 1;
	    }
	}
	else if (streq(s->name, "arrowtail")) {
	    if (*(v = agxget (ep, s))) {
		attrs.arrowtail = v;
		doGraphics = 1;
	    }
	}
	else if (streq(s->name, "fontname")) { /* fontName */
	    if (*(v = agxget (ep, s))) {
		attrs.fontName = v;
		doLabelGraphics = 1;
	    }
	}
	else if (streq(s->name, "fontsize")) { /* fontSize */
	    if (*(v = agxget (ep, s))) {
		attrs.fontSize = v;
		doLabelGraphics = 1;
	    }
	}
	else if (streq(s->name, "fontcolor")) { /* fontColor */
	    if (*(v = agxget (ep, s))) {
		attrs.fontColor = v;
		doLabelGraphics = 1;
	    }
	}
	else {
	    v = agxget (ep, s);
	    emitAttr(s->name, v, ix);
	}
    }

    /* Then, print them, if any */
    if (doGraphics) {
	fprintf (outFile, "    graphics [\n");
	if (attrs.pos) {
	    emitSpline(attrs.pos, ix+1);
	}
	if (attrs.flags & INVIS) {
	    emitInt("visible", 0, ix+1);
	}
	if (attrs.fill) {
	    emitAttr("fill", attrs.fill, ix+1);
	}
	if (attrs.width) {
	    emitAttr("width", attrs.width, ix+1);
	}
	if (attrs.arrowhead) {
	    emitAttr("targetArrow", attrs.arrowhead, ix+1);
	}
	if (attrs.arrowtail) {
	    emitAttr("sourceArrow", attrs.arrowtail, ix+1);
	}
	if (attrs.flags & DASH) {
	    emitAttr("style", "dashed", ix+1);
	}
	else if (attrs.flags & DOT) {
	    emitAttr("style", "dotted", ix+1);
	}
	else if (attrs.flags & LINE) {
	    emitAttr("style", "line", ix+1);
	}
	if (attrs.arrow) {
	    if (streq(attrs.arrow,"forward"))
		emitAttr("arrow", "first", ix+1);
	    else if (streq(attrs.arrow,"back"))
		emitAttr("arrow", "last", ix+1);
	    else if (streq(attrs.arrow,"both"))
		emitAttr("arrow", "both", ix+1);
	    else if (streq(attrs.arrow,"none"))
		emitAttr("arrow", "none", ix+1);
	}
	fprintf (outFile, "    ]\n");
    }

    if (doLabelGraphics) {
	fprintf (outFile, "    LabelGraphics [\n");
	if (label) emitAttr("text", label, ix+1);
	if (attrs.fontColor) {
	    emitAttr("fontColor", attrs.fontColor, ix+1);
	}
	if (attrs.fontSize) {
	    emitAttr("fontSize", attrs.fontSize, ix+1);
	}
	if (attrs.fontName) {
	    emitAttr("fontName", attrs.fontName, ix+1);
	}
	fprintf (outFile, "    ]\n");
    }
}

static void emitEdge(Agraph_t *G, Agedge_t *e) {
    fprintf(outFile, "  edge [\n    id %" PRIu64 "\n", (uint64_t)AGSEQ(e));
    fprintf(outFile, "    source %" PRIu64 "\n", ID(agtail(e)));
    fprintf(outFile, "    target %" PRIu64 "\n", ID(aghead(e)));
    emitEdgeAttrs(G, e, 2);
    fprintf (outFile, "  ]\n");
}

static void emitGraphAttrs(Agraph_t *G) {
    Agsym_t*  s;
    char* v;

    for (s = agnxtattr (G, AGRAPH, NULL); s; s = agnxtattr (G, AGRAPH, s)) {
	if (*(v = agxget (G, s))) {
	    emitAttr(s->name, v, 1);
	}
    }
}

static void gv_to_gml(Agraph_t *G) {
    Agnode_t* n;
    Agedge_t* e;

    fprintf (outFile, "graph [\n  version 2\n");
    if (agisdirected(G))
	fprintf (outFile, "  directed 1\n");
    else
	fprintf (outFile, "  directed 0\n");
	
    emitGraphAttrs(G);

    /* FIX: Not sure how to handle default attributes or subgraphs */

    for (n = agfstnode(G); n; n = agnxtnode (G, n)) {
	emitNode(G, n);
    } 
    
    for (n = agfstnode(G); n; n = agnxtnode (G, n)) {
	for (e = agfstout(G, n); e; e = agnxtout (G, e)) {
	    emitEdge(G, e);
	}
    }
    fprintf (outFile, "]\n");
}

static char *useString = "Usage: %s [-?] <files>\n\
  -o<file>  : output to <file> (stdout)\n\
  -? - print usage\n\
If no files are specified, stdin is used\n";

static void usage(int v)
{
    printf(useString, CmdName);
    graphviz_exit(v);
}

static char *cmdName(char *cmd)
{
    char *sp;

    sp = strrchr(cmd, '/');
    if (sp)
        sp++;
    else
        sp = cmd;
    return sp;
}

static void initargs(int argc, char **argv)
{
    int c;

    CmdName = cmdName(argv[0]);
    opterr = 0;
    while ((c = getopt(argc, argv, ":o:")) != -1) {
	switch (c) {
	case 'o':
	    if (outFile != NULL)
		fclose(outFile);
	    outFile = openFile(CmdName, optarg, "w");
	    break;
	case ':':
	    fprintf(stderr, "%s: option -%c missing parameter\n", CmdName, optopt);
	    usage(1);
	    break;
	case '?':
	    if (optopt == '?')
		usage(0);
	    else {
		fprintf(stderr, "%s: option -%c unrecognized\n", CmdName,
			optopt);
		usage(1);
	    }
	    break;
	default:
	    UNREACHABLE();
	}
    }

    argv += optind;
    argc -= optind;

    if (argc)
	Files = argv;
    if (!outFile)
	outFile = stdout;
}

int main(int argc, char **argv)
{
    Agraph_t *G;
    Agraph_t *prev = 0;
    int rv;
    ingraph_state ig;

    rv = 0;
    initargs(argc, argv);
    newIngraph(&ig, Files);

    while ((G = nextGraph(&ig))) {
	if (prev) {
	    id = 0;
	    agclose(prev);
	}
	prev = G;
	gv_to_gml(G);
	fflush(outFile);
    }
    graphviz_exit(rv);
}
