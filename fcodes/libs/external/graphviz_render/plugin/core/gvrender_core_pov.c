/**********************************************************
*            Copyright (c) 2011 Andy Jeutter              *
*            AKA HallerHarry at gmx.de                    *
*            All rights reserved.                         *
**********************************************************/

/*************************************************************************
 * This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#define _GNU_SOURCE
#include "config.h"
#include <cgraph/agxbuf.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <common/macros.h>
#include <common/const.h>

#include <gvc/gvplugin_render.h>
#include <gvc/gvplugin_device.h>
#include <gvc/gvio.h>
#include <gvc/gvcint.h>

#define POV_VERSION \
    "#version 3.6;\n"

#define POV_GLOBALS \
    "global_settings { assumed_gamma 1.0 }\n"

#define POV_DEFAULT \
    "#default { finish { ambient 0.1 diffuse 0.9 } }\n"

#define POV_INCLUDE \
    "#include \"colors.inc\"\n"\
    "#include \"textures.inc\"\n"\
    "#include \"shapes.inc\"\n"

#define POV_LIGHT \
    "light_source { <1500,3000,-2500> color White }\n"

#define POV_CAMERA \
    "camera { location <%.3f , %.3f , -500.000>\n"\
    "         look_at  <%.3f , %.3f , 0.000>\n"\
    "         right x * image_width / image_height\n"\
    "         angle %.3f\n"\
    "}\n"

#define POV_SKY_AND_GND \
    "//sky\n"\
    "plane { <0, 1, 0>, 1 hollow\n"\
    "    texture {\n"\
    "        pigment { bozo turbulence 0.95\n"\
    "            color_map {\n"\
    "                [0.00 rgb <0.05, 0.20, 0.50>]\n"\
    "                [0.50 rgb <0.05, 0.20, 0.50>]\n"\
    "                [0.75 rgb <1.00, 1.00, 1.00>]\n"\
    "                [0.75 rgb <0.25, 0.25, 0.25>]\n"\
    "                [1.00 rgb <0.50, 0.50, 0.50>]\n"\
    "            }\n"\
    "            scale <1.00, 1.00, 1.50> * 2.50\n"\
    "            translate <0.00, 0.00, 0.00>\n"\
    "        }\n"\
    "        finish { ambient 1 diffuse 0 }\n"\
    "    }\n"\
    "    scale 10000\n"\
    "}\n"\
    "//mist\n"\
    "fog { fog_type 2\n"\
    "    distance 50\n"\
    "    color rgb <1.00, 1.00, 1.00> * 0.75\n"\
    "    fog_offset 0.10\n"\
    "    fog_alt 1.50\n"\
    "    turbulence 1.75\n"\
    "}\n"\
    "//gnd\n"\
    "plane { <0.00, 1.00, 0.00>, 0\n"\
    "    texture {\n"\
    "        pigment{ color rgb <0.25, 0.45, 0.00> }\n"\
    "        normal { bumps 0.75 scale 0.01 }\n"\
    "        finish { phong 0.10 }\n"\
    "    }\n"\
    "}\n"

#define POV_BOX \
    "box { <%.3f, %.3f, %.3f>, <%.3f, %.3f, %.3f>\n"

#define POV_SCALE1 \
    "scale %.3f\n"

#define POV_SCALE3 \
    "scale    "POV_VECTOR3"\n"

#define POV_ROTATE \
    "rotate   "POV_VECTOR3"\n"

#define POV_TRANSLATE \
    "translate<%9.3f, %9.3f, %d.000>\n"

#define END \
    "}\n"

#define POV_TORUS \
    "torus { %.3f, %.3f\n"

#define POV_SPHERE_SWEEP \
    "sphere_sweep {\n"\
    "    %s\n"\
    "    %d,\n"

#define POV_SPHERE \
    "sphere {"POV_VECTOR3", 1.0\n"	// center, radius

#define POV_TEXT \
    "text {\n"\
    "    ttf \"%s\",\n"\
    "    \"%s\", %.3f, %.3f\n"

#define POV_DECLARE \
    "#declare %s = %s;\n"

#define POV_OBJECT \
    "object { %s }\n"

#define POV_VERBATIM \
    "%s\n"

#define POV_DEBUG \
    "#debug %s\n"

#define POV_POLYGON \
    "polygon { %d,\n"

#define POV_VECTOR3 \
    "<%9.3f, %9.3f, %9.3f>"

#define POV_PIGMENT_COLOR \
    "pigment { color %s }\n"

#define POV_COLOR_NAME \
    "%s transmit %.3f"

#define POV_COLOR_RGB \
    "rgb"POV_VECTOR3" transmit %.3f"

//colors are taken from /usr/share/povray-3.6/include/colors.inc
//list must be LANG_C sorted (all lower case)
#define POV_COLORS \
"aquamarine",\
"bakerschoc",\
"black",\
"blue",\
"blueviolet",\
"brass",\
"brightgold",\
"bronze",\
"bronze2",\
"brown",\
"cadetblue",\
"clear",\
"coolcopper",\
"copper",\
"coral",\
"cornflowerblue",\
"cyan",\
"darkbrown",\
"darkgreen",\
"darkolivegreen",\
"darkorchid",\
"darkpurple",\
"darkslateblue",\
"darkslategray",\
"darkslategrey",\
"darktan",\
"darkturquoise",\
"darkwood",\
"dkgreencopper",\
"dustyrose",\
"feldspar",\
"firebrick",\
"flesh",\
"forestgreen",\
"gold",\
"goldenrod",\
"gray05",\
"gray10",\
"gray15",\
"gray20",\
"gray25",\
"gray30",\
"gray35",\
"gray40",\
"gray45",\
"gray50",\
"gray55",\
"gray60",\
"gray65",\
"gray70",\
"gray75",\
"gray80",\
"gray85",\
"gray90",\
"gray95",\
"green",\
"greencopper",\
"greenyellow",\
"huntersgreen",\
"indianred",\
"khaki",\
"lightblue",\
"light_purple",\
"lightsteelblue",\
"lightwood",\
"limegreen",\
"magenta",\
"mandarinorange",\
"maroon",\
"mediumaquamarine",\
"mediumblue",\
"mediumforestgreen",\
"mediumgoldenrod",\
"mediumorchid",\
"mediumseagreen",\
"mediumslateblue",\
"mediumspringgreen",\
"mediumturquoise",\
"mediumvioletred",\
"mediumwood",\
"med_purple",\
"mica",\
"midnightblue",\
"navy",\
"navyblue",\
"neonblue",\
"neonpink",\
"newmidnightblue",\
"newtan",\
"oldgold",\
"orange",\
"orangered",\
"orchid",\
"palegreen",\
"pink",\
"plum",\
"quartz",\
"red",\
"richblue",\
"salmon",\
"scarlet",\
"seagreen",\
"semiSweetChoc",\
"sienna",\
"silver",\
"skyblue",\
"slateblue",\
"spicypink",\
"springgreen",\
"steelblue",\
"summersky",\
"tan",\
"thistle",\
"turquoise",\
"verydarkbrown",\
"very_light_purple",\
"violet",\
"violetred",\
"wheat",\
"white",\
"yellow",\
"yellowgreen"

#define GV_OBJ_EXT(type, obj, name) \
	do { \
		char debug_str[256]; \
		gvprintf(job, POV_DECLARE, type, obj); \
		gvprintf(job, POV_OBJECT, type); \
		gvprintf(job, POV_DECLARE, "Min", "min_extent("type")"); \
		gvprintf(job, POV_DECLARE, "Max", "max_extent("type")"); \
		snprintf(debug_str, 256,  \
			"concat(\"Dim = \" , vstr(3, Max - Min, \", \", 0, 3)," \
			" \" "type": %s\", \"\\n\")", name); \
		gvprintf(job, POV_DEBUG, debug_str); \
	} while (0)

#define DPI 72.0
#define RENDERER_COLOR_TYPE RGBA_BYTE
typedef enum { FORMAT_POV, } format_type;

//#define DEBUG

//TODO: check why this dot file does not work (90 rotated)
//   /usr/share/graphviz/graphs/directed/NaN.gv
//TODO: add Texttures
//TODO: check how we can receive attributes from dot file
//   if we can't receive attributes set defaults in pov include file
//   - put #include "graph-scheme-fancy.inc" in pov file
//   - run povray with +L`pwd`
//   - put e.g. #declare mycolor = Gold; in graph-scheme-fancy.inc
//   - use textures and color: pigment { color mycolor transmit 0.000 }
//TODO: idea, put the whole graph in a declare= and then
//   print it with something along the line:
//   object{ graph translate{page->translation, ...} rotate{page->rotation, ...} }

static char *pov_knowncolors[] = { POV_COLORS };

static int layerz = 0;
static int z = 0;

static char *pov_color_as_str(GVJ_t * job, gvcolor_t color, float transparency)
{
	agxbuf c = {0};
	switch (color.type) {
	case COLOR_STRING:
#ifdef DEBUG
		gvprintf(job, "// type = %d, color = %s\n", color.type, color.u.string);
#else
		(void)job;
#endif
		if (!strcmp(color.u.string, "red"))
			agxbprint(&c, POV_COLOR_NAME, "Red", transparency);
		else if (!strcmp(color.u.string, "green"))
			agxbprint(&c, POV_COLOR_NAME, "Green", transparency);
		else if (!strcmp(color.u.string, "blue"))
			agxbprint(&c, POV_COLOR_NAME, "Blue", transparency);
		else
			agxbprint(&c, POV_COLOR_NAME, color.u.string, transparency);
		break;
	case RENDERER_COLOR_TYPE:
#ifdef DEBUG
		gvprintf(job, "// type = %d, color = %d, %d, %d\n",
			 color.type, color.u.rgba[0], color.u.rgba[1],
			 color.u.rgba[2]);
#endif
		agxbprint(&c, POV_COLOR_RGB,
		       color.u.rgba[0] / 256.0, color.u.rgba[1] / 256.0,
		       color.u.rgba[2] / 256.0, transparency);
		break;
	default:
		fprintf(stderr,
			"oops, internal error: unhandled color type=%d %s\n",
			color.type, color.u.string);
		assert(0);	//oops, wrong type set in gvrender_features_t?
	}
	agxbuf pov = {0};
	agxbprint(&pov, POV_PIGMENT_COLOR, agxbuse(&c));
	agxbfree(&c);
	return agxbdisown(&pov);
}

static void pov_comment(GVJ_t * job, char *str)
{
	gvprintf(job, "//*** comment: %s\n", str);
}

static void pov_begin_job(GVJ_t * job)
{
	gvputs(job, POV_VERSION);
	gvputs(job, POV_GLOBALS);
	gvputs(job, POV_DEFAULT);
	gvputs(job, POV_INCLUDE);
	gvprintf(job, POV_DECLARE, "black", "Black");
	gvprintf(job, POV_DECLARE, "white", "White");
}

static void pov_begin_graph(GVJ_t * job)
{
	gvprintf(job, "//*** begin_graph %s\n", agnameof(job->obj->u.g));
#ifdef DEBUG
	gvprintf(job, "// graph_index = %d, pages = %d, layer = %d/%d\n",
		 job->graph_index, job->numPages, job->layerNum,
		 job->numLayers);
	gvprintf(job, "// pagesArraySize.x,y = %d,%d\n", job->pagesArraySize.x,
		 job->pagesArraySize.y);
	gvprintf(job, "// pagesArrayFirst.x,y = %d,%d\n",
		 job->pagesArrayFirst.x, job->pagesArrayFirst.y);
	gvprintf(job, "// pagesArrayElem.x,y = %d,%d\n", job->pagesArrayElem.x,
		 job->pagesArrayElem.y);
	gvprintf(job, "// bb.LL,UR = %.3f,%.3f, %.3f,%.3f\n", job->bb.LL.x,
		 job->bb.LL.y, job->bb.UR.x, job->bb.UR.y);
	gvprintf(job, "// pageBox in graph LL,UR = %.3f,%.3f, %.3f,%.3f\n",
		 job->pageBox.LL.x, job->pageBox.LL.y, job->pageBox.UR.x,
		 job->pageBox.UR.y);
	gvprintf(job, "// pageSize.x,y = %.3f,%.3f\n", job->pageSize.x,
		 job->pageSize.y);
	gvprintf(job, "// focus.x,y = %.3f,%.3f\n", job->focus.x, job->focus.y);
	gvprintf(job, "// zoom = %.3f, rotation = %d\n", job->zoom,
		 (float)job->rotation);
	gvprintf(job, "// view port.x,y = %.3f,%.3f\n", job->view.x,
		 job->view.y);
	gvprintf(job, "// canvasBox LL,UR = %.3f,%.3f, %.3f,%.3f\n",
		 job->canvasBox.LL.x, job->canvasBox.LL.y, job->canvasBox.UR.x,
		 job->canvasBox.UR.y);
	gvprintf(job, "// pageBoundingBox LL,UR = %d,%d, %d,%d\n",
		 job->pageBoundingBox.LL.x, job->pageBoundingBox.LL.y,
		 job->pageBoundingBox.UR.x, job->pageBoundingBox.UR.y);
	gvprintf(job, "// boundingBox (all pages) LL,UR = %d,%d, %d,%d\n",
		 job->boundingBox.LL.x, job->boundingBox.LL.y,
		 job->boundingBox.UR.x, job->boundingBox.UR.y);
	gvprintf(job, "// scale.x,y = %.3f,%.3f\n", job->scale.x, job->scale.y);
	gvprintf(job, "// translation.x,y = %.3f,%.3f\n", job->translation.x,
		 job->translation.y);
	gvprintf(job, "// devscale.x,y = %.3f,%.3f\n", job->devscale.x,
		 job->devscale.y);
	gvprintf(job, "// verbose = %d\n", job->common->verbose);
	gvprintf(job, "// cmd = %s\n", job->common->cmdname);
	gvprintf(job, "// info = %s, %s, %s\n", job->common->info[0],
		 job->common->info[1], job->common->info[2]);
#endif

	//setup scene
	double x = job->view.x / 2.0 * job->scale.x;
	double y = job->view.y / 2.0 * job->scale.y;
	double d = 500;
	double px = atan(x / d) * 180.0 / M_PI * 2.0;
	double py = atan(y / d) * 180.0 / M_PI * 2.0;
	gvprintf(job, POV_CAMERA, x, y, x, y, fmax(px, py) * 1.2);
	gvputs(job, POV_SKY_AND_GND);
	gvputs(job, POV_LIGHT);
}

static void pov_end_graph(GVJ_t * job)
{
	gvputs(job, "//*** end_graph\n");
}

static void pov_begin_layer(GVJ_t * job, char *layername, int layerNum, int numLayers)
{
	gvprintf(job, "//*** begin_layer: %s, %d/%d\n", layername, layerNum,
		 numLayers);
	layerz = layerNum * -10;
}

static void pov_end_layer(GVJ_t * job)
{
	gvputs(job, "//*** end_layer\n");
}

static void pov_begin_page(GVJ_t * job)
{
	gvputs(job, "//*** begin_page\n");
}

static void pov_end_page(GVJ_t * job)
{
	gvputs(job, "//*** end_page\n");
}

static void pov_begin_cluster(GVJ_t * job)
{
	gvputs(job, "//*** begin_cluster\n");
	layerz -= 2;
}

static void pov_end_cluster(GVJ_t * job)
{
	gvputs(job, "//*** end_cluster\n");
}

static void pov_begin_node(GVJ_t * job)
{
	gvprintf(job, "//*** begin_node: %s\n", agnameof(job->obj->u.n));
}

static void pov_end_node(GVJ_t * job)
{
	gvputs(job, "//*** end_node\n");
}

static void pov_begin_edge(GVJ_t * job)
{
	gvputs(job, "//*** begin_edge\n");
	layerz -= 5;
#ifdef DEBUG
	gvprintf(job, "// layerz = %d.000\n", layerz);
#endif
}

static void pov_end_edge(GVJ_t * job)
{
	gvputs(job, "//*** end_edge\n");
	layerz += 5;
#ifdef DEBUG
	gvprintf(job, "// layerz = %d.000\n", layerz);
#endif
}

static void pov_textspan(GVJ_t * job, pointf c, textspan_t * span)
{
	gvprintf(job, "//*** textspan: %s, fontsize = %.3f, fontname = %s\n",
		 span->str, span->font->size, span->font->name);
	z = layerz - 9;

#ifdef DEBUG
	if (span->font->postscript_alias)
		gvputs(job, "// Warning: postscript_alias not handled!\n");
#endif

	//handle text justification
	switch (span->just) {
	case 'l':		//left justified
		break;
	case 'r':		//right justified
		c.x = c.x - span->size.x;
		break;
	default:
	case 'n':		//centered
		c.x = c.x - span->size.x / 2.0;
		break;
	}

	double x = (c.x + job->translation.x) * job->scale.x;
	double y = (c.y + job->translation.y) * job->scale.y;

	char *p = pov_color_as_str(job, job->obj->pencolor, 0.0);

	//pov bundled fonts: timrom.ttf, cyrvetic.ttf
	agxbuf pov = {0};
	agxbprint(&pov, POV_TEXT "        no_shadow\n",
		span->font->name, span->str, 0.25, 0.0); // font, text, depth (0.5 ... 2.0), offset
	agxbprint(&pov, "    " POV_SCALE1, span->font->size * job->scale.x);
	agxbprint(&pov, "    " POV_ROTATE, 0.0, 0.0, (float)job->rotation);
	agxbprint(&pov, "    " POV_TRANSLATE, x, y, z);
	agxbprint(&pov, "    %s" END, p);

#ifdef DEBUG
	GV_OBJ_EXT("Text", agxbuse(&pov), span->str);
	gvprintf(job, "sphere{<0, 0, 0>, 2\ntranslate<%f, %f, %d>\n"
		 "pigment{color Red}\nno_shadow\n}\n", x, y, z - 1);
#else
	gvputs(job, agxbuse(&pov));
#endif

	agxbfree(&pov);
	free(p);
}

static void pov_ellipse(GVJ_t * job, pointf * A, int filled)
{
	gvputs(job, "//*** ellipse\n");
	z = layerz - 6;

	// A[0] center, A[1] corner of ellipse
	float cx = (A[0].x + job->translation.x) * job->scale.x;
	float cy = (A[0].y + job->translation.y) * job->scale.y;
	float rx = (A[1].x - A[0].x) * job->scale.x;
	float ry = (A[1].y - A[0].y) * job->scale.y;
	float w = job->obj->penwidth / (rx + ry) / 2.0 * 5;

	//draw rim (torus)
	char *p = pov_color_as_str(job, job->obj->pencolor, 0.0);

	agxbuf pov = {0};
	agxbprint(&pov, POV_TORUS, 1.0, w); // radius, size of ring
	agxbprint(&pov, "    " POV_SCALE3, rx, (rx + ry) / 4.0, ry);
	agxbprint(&pov, "    " POV_ROTATE, 90.0, 0.0, (float)job->rotation);
	agxbprint(&pov, "    " POV_TRANSLATE, cx, cy, z);
	agxbprint(&pov, "    %s" END, p);

#ifdef DEBUG
	GV_OBJ_EXT("Torus", agxbuse(&pov), "");
	gvprintf(job, "sphere{<0, 0, 0>, 2\ntranslate<%f, %f, %d>\n"
		 "pigment{color Green}\nno_shadow\n}\n", cx, cy, z - 1);
#else
	gvputs(job, agxbuse(&pov));
#endif

	free(p);

	//backgroud of ellipse if filled
	if (filled) {
		p = pov_color_as_str(job, job->obj->fillcolor, 0.0);

		gvprintf(job, POV_SPHERE, 0.0, 0.0, 0.0);
		gvprintf(job, "    " POV_SCALE3, rx, ry, 1.0);
		gvprintf(job, "    " POV_ROTATE, 0.0, 0.0, (float)job->rotation);
		gvprintf(job, "    " POV_TRANSLATE, cx, cy, z);
		gvprintf(job, "    %s" END, p);

		free(p);
	}
	agxbfree(&pov);
}

static void pov_bezier(GVJ_t *job, pointf *A, int n, int filled) {
	(void)filled;

	gvputs(job, "//*** bezier\n");
	z = layerz - 4;

	char *p = pov_color_as_str(job, job->obj->fillcolor, 0.0);

	agxbuf pov = {0};
	agxbprint(&pov, POV_SPHERE_SWEEP, "b_spline", n + 2);

	for (int i = 0; i < n; i++) {
		agxbprint(&pov, "    " POV_VECTOR3 ", %.3f\n", A[i].x + job->translation.x,
		          A[i].y + job->translation.y, 0.0, job->obj->penwidth); // z coordinate, thickness

		//TODO: we currently just use the start and end points of the curve as
		//control points but we should use center of nodes
		if (i == 0 || i == n - 1) {
			agxbprint(&pov, "    " POV_VECTOR3 ", %.3f\n", A[i].x + job->translation.x,
			          A[i].y + job->translation.y, 0.0, job->obj->penwidth); // z coordinate, thickness
		}
#ifdef DEBUG
		gvprintf(job, "sphere{<0, 0, 0>, 2\ntranslate<%f, %f, %d>\n"
			 "pigment{color Yellow}\nno_shadow\n}\n",
			 (A[i].x + job->translation.x) * job->scale.x,
			 (A[i].y + job->translation.y) * job->scale.y, z - 2);
#endif
	}
	// catenate pov & end str
	gvprintf(job, "%s        tolerance 0.01\n", agxbuse(&pov));
	gvprintf(job, "    " POV_SCALE3, job->scale.x, job->scale.y, 1.0);
	gvprintf(job, "    " POV_ROTATE, 0.0, 0.0, (float)job->rotation);
	gvprintf(job, "    " POV_TRANSLATE, 0.0, 0.0, z - 2);
	gvprintf(job, "    %s" END, p);

	free(p);
	agxbfree(&pov);
}

static void pov_polygon(GVJ_t * job, pointf * A, int n, int filled)
{
	gvputs(job, "//*** polygon\n");
	z = layerz - 2;

	char *p = pov_color_as_str(job, job->obj->pencolor, 0.0);

	gvprintf(job, POV_SPHERE_SWEEP, "linear_spline", n + 1);

	for (int i = 0; i < n; i++) {
		gvprintf(job, "    " POV_VECTOR3 ", %.3f\n", A[i].x + job->translation.x,
		          A[i].y + job->translation.y, 0.0, job->obj->penwidth); // z coordinate, thickness
	}

	//close polygon, add starting point as final point^
	gvprintf(job, "    " POV_VECTOR3 ", %.3f\n", A[0].x + job->translation.x,
	          A[0].y + job->translation.y, 0.0, job->obj->penwidth); // z coordinate, thickness

	gvputs(job, "    tolerance 0.1\n");
	gvprintf(job, "    " POV_SCALE3, job->scale.x, job->scale.y, 1.0);
	gvprintf(job, "    " POV_ROTATE, 0.0, 0.0, (float)job->rotation);
	gvprintf(job, "    " POV_TRANSLATE, 0.0, 0.0, z - 2);
	gvprintf(job, "    %s" END, p);

	free(p);

	//create fill background
	if (filled) {
		p = pov_color_as_str(job, job->obj->fillcolor, 0.25);

		gvprintf(job, POV_POLYGON, n);

		for (int i = 0; i < n; i++) {
			//create on z = 0 plane, then translate to real z pos
			gvprintf(job, "\n    " POV_VECTOR3,
			       A[i].x + job->translation.x,
			       A[i].y + job->translation.y, 0.0);
		}
		gvputs(job, "\n    ");
		gvprintf(job, "    " POV_SCALE3, job->scale.x, job->scale.y, 1.0);
		gvprintf(job, "    " POV_ROTATE, 0.0, 0.0, (float)job->rotation);
		gvprintf(job, "    " POV_TRANSLATE, 0.0, 0.0, z - 2);
		gvprintf(job, "    %s" END, p);

		free(p);
	}
}

static void pov_polyline(GVJ_t * job, pointf * A, int n)
{
	gvputs(job, "//*** polyline\n");
	z = layerz - 6;

	char *p = pov_color_as_str(job, job->obj->pencolor, 0.0);

	gvprintf(job, POV_SPHERE_SWEEP, "linear_spline", n);

	for (int i = 0; i < n; i++) {
		gvprintf(job, "    " POV_VECTOR3 ", %.3f\n", A[i].x + job->translation.x,
		          A[i].y + job->translation.y, 0.0, job->obj->penwidth); // z coordinate, thickness
	}

	gvputs(job, "    tolerance 0.01\n");
	gvprintf(job, "    " POV_SCALE3, job->scale.x, job->scale.y, 1.0);
	gvprintf(job, "    " POV_ROTATE, 0.0, 0.0, (float)job->rotation);
	gvprintf(job, "    " POV_TRANSLATE, 0.0, 0.0, z);
	gvprintf(job, "    %s" END, p);

	free(p);
}

gvrender_engine_t pov_engine = {
	pov_begin_job,
	0,			/* pov_end_job */
	pov_begin_graph,
	pov_end_graph,
	pov_begin_layer,
	pov_end_layer,
	pov_begin_page,
	pov_end_page,
	pov_begin_cluster,
	pov_end_cluster,
	0,			/* pov_begin_nodes */
	0,			/* pov_end_nodes */
	0,			/* pov_begin_edges */
	0,			/* pov_end_edges */
	pov_begin_node,
	pov_end_node,
	pov_begin_edge,
	pov_end_edge,
	0,			/* pov_begin_anchor */
	0,			/* pov_end_anchor */
	0,			/* pov_begin_label */
	0,			/* pov_end_label */
	pov_textspan,
	0,			/* pov_resolve_color */
	pov_ellipse,
	pov_polygon,
	pov_bezier,
	pov_polyline,
	pov_comment,
	0,			/* pov_library_shape */
};

gvrender_features_t render_features_pov = {
	/* flags */
	GVDEVICE_DOES_LAYERS
	    | GVRENDER_DOES_MAP_RECTANGLE
	    | GVRENDER_DOES_MAP_CIRCLE
	    | GVRENDER_DOES_MAP_POLYGON
	    | GVRENDER_DOES_MAP_ELLIPSE
	    | GVRENDER_DOES_MAP_BSPLINE
	    | GVRENDER_NO_WHITE_BG
	    | GVRENDER_DOES_TRANSFORM
	    | GVRENDER_DOES_Z | GVRENDER_DOES_MAP_BSPLINE,
	4.0,			/* default pad - graph units */
	pov_knowncolors,	/* knowncolors */
	sizeof(pov_knowncolors) / sizeof(char *),	/* strings in knowncolors */
	RENDERER_COLOR_TYPE	/* set renderer color type */
};

gvdevice_features_t device_features_pov = {
	GVDEVICE_DOES_TRUECOLOR,	/* flags */
	{0.0, 0.0},		/* default margin - points */
	{0.0, 0.0},		/* default page width, height - points */
	{DPI, DPI},		/* default dpi */
};

gvplugin_installed_t gvrender_pov_types[] = {
	{FORMAT_POV, "pov", 1, &pov_engine, &render_features_pov},
	{0, NULL, 0, NULL, NULL}
};

gvplugin_installed_t gvdevice_pov_types[] = {
	{FORMAT_POV, "pov:pov", 1, NULL, &device_features_pov},
	{0, NULL, 0, NULL, NULL}
};

