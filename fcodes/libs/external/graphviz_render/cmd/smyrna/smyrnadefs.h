/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#pragma once

#ifdef _WIN32
#ifndef NO_WIN_HEADER
#include "windows.h"
#endif
#endif

#include <stdbool.h>
#include <stddef.h>
#include <xdot/xdot.h>
#include <gtk/gtk.h>
#include <glcomp/opengl.h>
#include <gtk/gtkgl.h>
#include <cgraph/cgraph.h>
#include <cgraph/list.h>
#include <glcomp/glcompset.h>
#include "hier.h"
#include <glcomp/glutils.h>

#ifdef	_WIN32			//this is needed on _WIN32 to get libglade see the callback
#define _BB  __declspec(dllexport)
#else
#define _BB
#endif

typedef struct _ArcBall_t ArcBall_t;

#define GL_VIEWPORT_FACTOR						100
//mouse modes
#define MM_PAN					0
#define MM_ZOOM					1
#define MM_ROTATE				2
#define MM_SINGLE_SELECT		3
#define MM_RECTANGULAR_SELECT	4
#define MM_RECTANGULAR_X_SELECT	5
#define MM_MOVE					10
#define MM_MAGNIFIER			20
#define MM_FISHEYE_MAGNIFIER	21
#define MM_FISHEYE_PICK		22  /*fisheye select foci point*/
#define MM_POLYGON_SELECT   30

#define MAX_ZOOM	500.0f
#define MIN_ZOOM	0.005f

#define DEG2RAD  G_PI/180

#define MAX_FILTERED_ATTR_COUNT 50

typedef enum {attr_alpha,attr_float,attr_int,attr_bool} attr_data_type;
typedef enum {smyrna_all,smyrna_2D,smyrna_3D,smyrna_fisheye,smyrna_all_but_fisheye} smyrna_view_mode;


typedef struct{
    int keyVal;
    int down;
}keymap_t;



typedef struct {
	size_t index;
	char* name;
	char* value;
	char* defValG;
	char* defValN;
	char* defValE;
	attr_data_type type;
	int objType[3];
	GtkWidget* widget;
	int propagate;
}attr_t;

DEFINE_LIST(attrs, attr_t*)

typedef struct
{
	attrs_t attributes;
	GtkLabel* fLabels[MAX_FILTERED_ATTR_COUNT];
	bool with_widgets;
}attr_list;

    typedef struct 
    {
	xdot_op op;
	void *obj;
	glCompFont* font;
	int size;
	int layer;
	glCompImage* img;
    } sdot_op;	
	
    typedef struct {
	float perc;
	glCompColor c;

    } colorschema;

    typedef struct {
	int schemacount;       /* number of colors */
	int smooth;            /* if true, interpolate */
	colorschema *s;
    } colorschemaset;

    typedef enum {
	GVK_DOT,
	GVK_NEATO,
	GVK_TWOPI,
	GVK_CIRCO,
	GVK_FDP,
    } gvk_layout;

    typedef struct
    {
	unsigned node_id;
	unsigned edge_id;
	unsigned selnode_id;
	unsigned seledge_id;
	unsigned nodelabel_id;
	unsigned edgelabel_id;
    }topviewcache;

    typedef struct {
	int color;
	int pos;
	int selection;
    }refresh_filter;


    typedef struct 
    {
	int index;
	int action;
	int hotkey;
	glMouseButtonType type;
	int drag;
	smyrna_view_mode mode;
    }mouse_action_t;

    typedef struct _viewport_camera {
	float x;
	float y;
	float z;

	float targetx;
	float targety;
	float targetz;

	float r;
    } viewport_camera;

    typedef struct _graph_data {
	Agrec_t h;
	char *GraphFileName;
    } graph_data;



    typedef struct{
        Agrec_t h;
        glCompPoint A;
	float size;
	int selected;
	int visible;
	int printLabel;
	int TVref;
    }nodeRec;
#define NREC(n) ((nodeRec*)(aggetrec(n,"nodeRec",0)))
#define ND_visible(n) (NREC(n)->visible)
#define ND_selected(n) (NREC(n)->selected)
#define ND_printLabel(n) (NREC(n)->printLabel)
#define ND_A(n) (NREC(n)->A)
#define ND_size(n) (NREC(n)->size)
#define ND_TVref(n) (NREC(n)->TVref)

    typedef struct{
	Agrec_t h;
	glCompPoint posTail;
	glCompPoint posHead;
	int selected;
    }edgeRec;
#define EREC(e) ((edgeRec*)(aggetrec(e,"edgeRec",0)))
#define ED_selected(e) (EREC(e)->selected)
#define ED_posTail(e) (EREC(e)->posTail)
#define ED_posHead(e) (EREC(e)->posHead)

    typedef struct{
	Agrec_t h;
	Agsym_t* N_pos;
	Agsym_t* N_size;
	Agsym_t* N_visible;
	Agsym_t* N_selected;
	Agsym_t* G_nodelabelcolor;
	Agsym_t* GN_labelattribute;
	Agsym_t* N_labelattribute;
	Agsym_t* E_visible;
	Agsym_t* E_selected;
	Agsym_t* E_pos;
	Agsym_t* G_edgelabelcolor;
	Agsym_t* E_labelattribute;
	Agsym_t* GE_labelattribute;
    } graphRec;
#define GREC(g) ((graphRec*)(AGDATA(g)))
#define GN_pos(g) (GREC(g)->N_pos)
#define GN_size(g) (GREC(g)->N_size)
#define GN_visible(g) (GREC(g)->N_visible)
#define GN_selected(g) (GREC(g)->N_selected)
#define GG_nodelabelcolor(g) (GREC(g)->G_nodelabelcolor)
#define GN_labelattribute(g) (GREC(g)->N_labelattribute)
#define GG_labelattribute(g) (GREC(g)->GN_labelattribute)
#define GE_pos(g) (GREC(g)->E_pos)
#define GE_visible(g) (GREC(g)->E_visible)
#define GE_selected(g) (GREC(g)->E_selected)
#define GG_edgelabelcolor(g) (GREC(g)->G_edgelabelcolor)
#define GE_labelattribute(g) (GREC(g)->E_labelattribute)
#define GG_elabelattribute(g) (GREC(g)->GE_labelattribute)

#define GUI_WINDOWED   0
#define GUI_FULLSCREEN    1

    typedef struct _selection {
	glCompPoly selPoly;
	int selectNodes;
	int selectEdges;
    } selection;

    typedef struct {
	int Nodecount;
	struct {
	    int active;	//1 draw hierarchy 0 draw regular topview
	    reposition_t repos;
	    levelparms_t level;
	    hierparms_t hier;
	    Hierarchy *h;
	    int animate;
	    glCompColor srcColor;	//fine node colors of topfisheye
	    glCompColor tarColor;	//supernode colors of fisheye
	    focus_t *fs;
    	} fisheyeParams;

	graph_data Graphdata;
	float maxedgelen;
	float minedgelen;
	float fitin_zoom;
	xdot* xDot;
	double global_z;
	attr_list* attributes;/*attribute list*/

	topviewcache cache;
	selection sel;
	
    } topview;

    typedef struct{
    Agraph_t *def_attrs;
    Agraph_t *attrs_widgets;
    }systemgraphs;


    typedef struct _ViewInfo {
	systemgraphs systemGraphs;
	/*view variables */
	float panx;
	float pany;
	float panz;
	float zoom;

	/*clipping coordinates, to avoid unnecesarry rendering */
	float clipX1, clipX2, clipY1, clipY2;

	/*background color */
	glCompColor bgColor;
	/*default pen color */
	glCompColor penColor;
	/*default fill color */
	glCompColor fillColor;
	/*highlighted Node Color */
	glCompColor highlightedNodeColor;
	/*highlighted Edge Color */
	glCompColor highlightedEdgeColor;
	/*grid color */
	glCompColor gridColor;	//grid color
	/*border color */
	glCompColor borderColor;
	/*selected node color */
	glCompColor selectedNodeColor;
	/*selected edge color */
	glCompColor selectedEdgeColor;
	/*default node alpha */
	float defaultnodealpha;
	/*default edge alpha */
	float defaultedgealpha;

	/*default line width */
	float LineWidth;

	/*grid is drawn if this variable is 1 */
	int gridVisible;
	/*grid cell size in gl coords system */
	float gridSize;		//grid cell size

	/*draws a border in border colors if it is 1 */
	int bdVisible;		//if borders are visible (boundries of the drawing,
	/*border coordinates, needs to be calculated for each graph */

	float bdxLeft, bdyTop;
	float bdxRight, bdyBottom;

	/*screen window size in 2d */
	int w, h;
	/*graph pointer to hold loaded graphs */
	Agraph_t **g;
	/*number of graphs loaded */
	int graphCount;
	/*active graph */
	int activeGraph;

	/*stores the info about status of mouse,pressed? what button ? where? */
//	mouse_attr mouse;
	glCompMouse mouse;

	/*selection object,refer to smyrnadefs.h for more info */
//	selection Selection;

	viewport_camera **cameras;
	int camera_count;	//number of cameras
	int active_camera;

	/*open gl canvas, used to be a globa variable before looks better wrapped in viewinfo */
	GtkWidget *drawing_area;

	/*Topview data structure, refer topview.h for more info */
	topview *Topview;
	/*timer for animations */
	GTimer *timer;
	/*this timer is session timer and always active */
	GTimer *timer2;
	/*general purpose timer */
	GTimer *timer3;
	int active_frame;
	int total_frames;
	/*lately added */
	int drawnodes;
	int drawedges;
	int drawnodelabels;
	int drawedgelabels;

	/*labelling properties */
	void *glutfont;
	glCompColor nodelabelcolor;
	glCompColor edgelabelcolor;
	int labelwithdegree;
	int labelnumberofnodes;
	int labelshownodes;
	int labelshowedges;

	glCompSet *widgets;	//for novice user open gl menu
	char *initFileName;	//file name from command line
	int initFile;
	int drawSplines;
	colorschemaset *colschms;
	char *template_file;
	GtkComboBox *graphComboBox;	/*pointer to graph combo box at top right */
	ArcBall_t *arcball;
	keymap_t keymap;
	mouse_action_t* mouse_actions;	/*customizable moouse interraction list*/
	size_t mouse_action_count;
	refresh_filter refresh;
	float nodeScale;
	int guiMode;
	char* optArg;

    } ViewInfo;
/*rotation steps*/

    extern ViewInfo *view;
    extern GtkMessageDialog *Dlg;
    extern char *smyrnaPath(char *suffix);

    extern void glexpose(void);
