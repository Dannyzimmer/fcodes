/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#endif
#include "viewport.h"
#include "draw.h"
#include <cgraph/alloc.h>
#include <common/color.h>
#include <glade/glade.h>
#include "gui.h"
#include "menucallbacks.h"
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "glcompui.h"
#include "gltemplate.h"
#include <common/colorprocs.h>
#include "topviewsettings.h"
#include "arcball.h"
#include "hotkeymap.h"
#include "topviewfuncs.h"
#include <cgraph/exit.h>
#include <cgraph/strcasecmp.h>


static colorschemaset *create_color_theme(int themeid);

ViewInfo *view;
/* these two global variables should be wrapped in something else */
GtkMessageDialog *Dlg;

static void clear_viewport(ViewInfo *vi) {
    if (vi->graphCount)
	agclose(vi->g[vi->activeGraph]);
}
static void *get_glut_font(int ind)
{

    switch (ind) {
    case 0:
	return GLUT_BITMAP_9_BY_15;
	break;
    case 1:
	return GLUT_BITMAP_8_BY_13;
	break;
    case 2:
	return GLUT_BITMAP_TIMES_ROMAN_10;
	break;
    case 3:
	return GLUT_BITMAP_HELVETICA_10;
	break;
    case 4:
	return GLUT_BITMAP_HELVETICA_12;
	break;
    case 5:
	return GLUT_BITMAP_HELVETICA_18;
	break;
    default:
	return GLUT_BITMAP_TIMES_ROMAN_10;
    }

}

void close_graph(ViewInfo *vi) {
    if (vi->activeGraph < 0)
	return;
    clear_viewport(vi);
}

char *get_attribute_value(char *attr, ViewInfo *vi, Agraph_t *g) {
    char *buf;
    buf = agget(g, attr);
    if (!buf || *buf == '\0')
	buf = agget(vi->systemGraphs.def_attrs, attr);
    return buf;
}

void set_viewport_settings_from_template(ViewInfo *vi, Agraph_t *g) {
    gvcolor_t cl;
    char *buf;
    colorxlate(get_attribute_value("bordercolor", vi, g), &cl,
	       RGBA_DOUBLE);
    vi->borderColor.R = (float)cl.u.RGBA[0];
    vi->borderColor.G = (float)cl.u.RGBA[1];
    vi->borderColor.B = (float)cl.u.RGBA[2];

    vi->borderColor.A =
	(float)atof(get_attribute_value("bordercoloralpha", vi, g));

    vi->bdVisible = atoi(get_attribute_value("bordervisible", vi, g));

    buf = get_attribute_value("gridcolor", vi, g);
    colorxlate(buf, &cl, RGBA_DOUBLE);
    vi->gridColor.R = (float)cl.u.RGBA[0];
    vi->gridColor.G = (float)cl.u.RGBA[1];
    vi->gridColor.B = (float)cl.u.RGBA[2];
    vi->gridColor.A =
	(float) atof(get_attribute_value("gridcoloralpha", vi, g));

    vi->gridSize = (float)atof(buf = get_attribute_value("gridsize", vi, g));

    vi->gridVisible = atoi(get_attribute_value("gridvisible", vi, g));

    //background color , default white
    colorxlate(get_attribute_value("bgcolor", vi, g), &cl, RGBA_DOUBLE);

    vi->bgColor.R = (float)cl.u.RGBA[0];
    vi->bgColor.G = (float)cl.u.RGBA[1];
    vi->bgColor.B = (float)cl.u.RGBA[2];
    vi->bgColor.A = 1.0f;

    //selected nodes are drawn with this color
    colorxlate(get_attribute_value("selectednodecolor", vi, g), &cl,
	       RGBA_DOUBLE);
    vi->selectedNodeColor.R = (float)cl.u.RGBA[0];
    vi->selectedNodeColor.G = (float)cl.u.RGBA[1];
    vi->selectedNodeColor.B = (float)cl.u.RGBA[2];
    vi->selectedNodeColor.A = (float)
	atof(get_attribute_value("selectednodecoloralpha", vi, g));
    //selected edge are drawn with this color
    colorxlate(get_attribute_value("selectededgecolor", vi, g), &cl,
	       RGBA_DOUBLE);
    vi->selectedEdgeColor.R = (float)cl.u.RGBA[0];
    vi->selectedEdgeColor.G = (float)cl.u.RGBA[1];
    vi->selectedEdgeColor.B = (float)cl.u.RGBA[2];
    vi->selectedEdgeColor.A = (float)
	atof(get_attribute_value("selectededgecoloralpha", vi, g));

    colorxlate(get_attribute_value("highlightednodecolor", vi, g), &cl,
	       RGBA_DOUBLE);
    vi->highlightedNodeColor.R = (float)cl.u.RGBA[0];
    vi->highlightedNodeColor.G = (float)cl.u.RGBA[1];
    vi->highlightedNodeColor.B = (float)cl.u.RGBA[2];
    vi->highlightedNodeColor.A = (float)
	atof(get_attribute_value("highlightednodecoloralpha", vi, g));

    buf = agget(g, "highlightededgecolor");
    colorxlate(get_attribute_value("highlightededgecolor", vi, g), &cl,
	       RGBA_DOUBLE);
    vi->highlightedEdgeColor.R = (float)cl.u.RGBA[0];
    vi->highlightedEdgeColor.G = (float)cl.u.RGBA[1];
    vi->highlightedEdgeColor.B = (float)cl.u.RGBA[2];
    vi->highlightedEdgeColor.A = (float)
	atof(get_attribute_value("highlightededgecoloralpha", vi, g));
    vi->defaultnodealpha = (float)
	atof(get_attribute_value("defaultnodealpha", vi, g));

    vi->defaultedgealpha = (float)
	atof(get_attribute_value("defaultedgealpha", vi, g));



    /*default line width */
    vi->LineWidth =
	(float) atof(get_attribute_value("defaultlinewidth", vi, g));

    vi->drawnodes = atoi(get_attribute_value("drawnodes", vi, g));
    vi->drawedges = atoi(get_attribute_value("drawedges", vi, g));
    vi->drawnodelabels=atoi(get_attribute_value("labelshownodes", vi, g));
    vi->drawedgelabels=atoi(get_attribute_value("labelshowedges", vi, g));
    vi->nodeScale=atof(get_attribute_value("nodesize", vi, g))*.30;

    vi->glutfont =
	get_glut_font(atoi(get_attribute_value("labelglutfont", vi, g)));
    colorxlate(get_attribute_value("nodelabelcolor", vi, g), &cl,
	       RGBA_DOUBLE);
    vi->nodelabelcolor.R = (float)cl.u.RGBA[0];
    vi->nodelabelcolor.G = (float)cl.u.RGBA[1];
    vi->nodelabelcolor.B = (float)cl.u.RGBA[2];
    vi->nodelabelcolor.A =
	(float) atof(get_attribute_value("defaultnodealpha", vi, g));
    colorxlate(get_attribute_value("edgelabelcolor", vi, g), &cl,
	       RGBA_DOUBLE);
    vi->edgelabelcolor.R = (float)cl.u.RGBA[0];
    vi->edgelabelcolor.G = (float)cl.u.RGBA[1];
    vi->edgelabelcolor.B = (float)cl.u.RGBA[2];
    vi->edgelabelcolor.A =
	(float) atof(get_attribute_value("defaultedgealpha", vi, g));
    vi->labelwithdegree = atoi(get_attribute_value("labelwithdegree", vi, g));
    vi->labelnumberofnodes =
	atof(get_attribute_value("labelnumberofnodes", vi, g));
    vi->labelshownodes = atoi(get_attribute_value("labelshownodes", vi, g));
    vi->labelshowedges = atoi(get_attribute_value("labelshowedges", vi, g));
    vi->colschms =
	create_color_theme(atoi
			   (get_attribute_value("colortheme", vi, g)));

    if (vi->graphCount > 0)
	glClearColor(vi->bgColor.R, vi->bgColor.G, vi->bgColor.B, vi->bgColor.A);	//background color
}

static gboolean gl_main_expose(gpointer data)
{
    (void)data;

    if (view->activeGraph >= 0) {
	if (view->Topview->fisheyeParams.animate == 1)
	    expose_event(view->drawing_area, NULL, NULL);
	return 1;
    }
    return 1;
}

static void get_data_dir(void)
{
    free(view->template_file);
    view->template_file = gv_strdup(smyrnaPath("template.dot"));
}

void init_viewport(ViewInfo *vi) {
    FILE *input_file = NULL;
    FILE *input_file2 = NULL;
    static char* path;
    get_data_dir();
    input_file = fopen(vi->template_file, "rb");
    if (!input_file) {
	fprintf(stderr,
		"default attributes template graph file \"%s\" not found\n",
		vi->template_file);
	graphviz_exit(-1);
    } 
    vi->systemGraphs.def_attrs = agread(input_file, NULL);
    fclose (input_file);

    if (!vi->systemGraphs.def_attrs) {
	fprintf(stderr,
		"could not load default attributes template graph file \"%s\"\n",
		vi->template_file);
	graphviz_exit(-1);
    }
    if (!path)
	path = smyrnaPath("attr_widgets.dot");
    input_file2 = fopen(path, "rb");
    if (!input_file2) {
	fprintf(stderr,	"default attributes template graph file \"%s\" not found\n",smyrnaPath("attr_widgets.dot"));
	graphviz_exit(-1);

    }
    vi->systemGraphs.attrs_widgets = agread(input_file2, NULL);
    fclose (input_file2);
    if (!vi->systemGraphs.attrs_widgets) {
	fprintf(stderr,"could not load default attribute widgets graph file \"%s\"\n",smyrnaPath("attr_widgets.dot"));
	graphviz_exit(-1);
    }
    //init graphs
    vi->g = NULL;		//no graph, gl screen should check it
    vi->graphCount = 0;	//and disable interactivity if count is zero

    vi->bdxLeft = 0;
    vi->bdxRight = 500;
    vi->bdyBottom = 0;
    vi->bdyTop = 500;

    vi->borderColor.R = 1;
    vi->borderColor.G = 0;
    vi->borderColor.B = 0;
    vi->borderColor.A = 1;

    vi->bdVisible = 1;	//show borders red

    vi->gridSize = 10;
    vi->gridColor.R = 0.5;
    vi->gridColor.G = 0.5;
    vi->gridColor.B = 0.5;
    vi->gridColor.A = 1;
    vi->gridVisible = 0;	//show grids in light gray

    //mouse mode=pan
    //pen color
    vi->penColor.R = 0;
    vi->penColor.G = 0;
    vi->penColor.B = 0;
    vi->penColor.A = 1;

    vi->fillColor.R = 1;
    vi->fillColor.G = 0;
    vi->fillColor.B = 0;
    vi->fillColor.A = 1;
    //background color , default white
    vi->bgColor.R = 1;
    vi->bgColor.G = 1;
    vi->bgColor.B = 1;
    vi->bgColor.A = 1;

    //selected objets are drawn with this color
    vi->selectedNodeColor.R = 1;
    vi->selectedNodeColor.G = 0;
    vi->selectedNodeColor.B = 0;
    vi->selectedNodeColor.A = 1;

    //default line width;
    vi->LineWidth = 1;

    //default view settings, camera is not active
    vi->panx = 0;
    vi->pany = 0;
    vi->panz = 0;

    vi->zoom = -20;

    vi->mouse.down = 0;
    vi->activeGraph = -1;
    vi->Topview = gv_alloc(sizeof(topview));
    vi->Topview->fisheyeParams.fs = 0;
    vi->Topview->xDot=NULL;

    /* init topfish parameters */
    vi->Topview->fisheyeParams.level.num_fine_nodes = 10;
    vi->Topview->fisheyeParams.level.coarsening_rate = 2.5;
    vi->Topview->fisheyeParams.hier.dist2_limit = 1;
    vi->Topview->fisheyeParams.repos.rescale = Polar;
    vi->Topview->fisheyeParams.repos.width = (int)(vi->bdxRight - vi->bdxLeft);
    vi->Topview->fisheyeParams.repos.height = (int)(vi->bdyTop - vi->bdyBottom);
    vi->Topview->fisheyeParams.repos.margin = 0;
    vi->Topview->fisheyeParams.repos.graphSize = 100;
    vi->Topview->fisheyeParams.repos.distortion = 1.0;
    /*create timer */
    vi->timer = g_timer_new();
    vi->timer2 = g_timer_new();
    vi->timer3 = g_timer_new();

    g_timer_stop(vi->timer);
    vi->active_frame = 0;
    vi->total_frames = 1500;
    /*add a call back to the main() */
    g_timeout_add_full(G_PRIORITY_DEFAULT, 100u, gl_main_expose, NULL, NULL);
    vi->cameras = NULL;
    vi->camera_count = 0;
    vi->active_camera = -1;
    set_viewport_settings_from_template(vi, vi->systemGraphs.def_attrs);
    vi->Topview->Graphdata.GraphFileName = NULL;
    vi->colschms = NULL;
    vi->arcball = gv_alloc(sizeof(ArcBall_t));
    vi->keymap.down=0;
    load_mouse_actions(vi);
    vi->refresh.color=1;
    vi->refresh.pos=1;
    vi->refresh.selection=1;
    if(vi->guiMode!=GUI_FULLSCREEN)
	vi->guiMode=GUI_WINDOWED;

    /*create glcomp menu system */
    vi->widgets = glcreate_gl_topview_menu();
}


/* update_graph_params:
 * adds gledit params
 * assumes custom_graph_data has been attached to the graph.
 */
static void update_graph_params(Agraph_t * graph)
{
    agattr(graph, AGRAPH, "GraphFileName",
	   view->Topview->Graphdata.GraphFileName);
}

static Agraph_t *loadGraph(char *filename)
{
    Agraph_t *g;
    FILE *input_file;
    if (!(input_file = fopen(filename, "r"))) {
	g_print("Cannot open %s\n", filename);
	return 0;
    }
    g = agread(input_file, NULL);
    fclose (input_file);
    if (!g) {
	g_print("Cannot read graph in  %s\n", filename);
	return 0;
    }

    /* If no position info, run layout with -Txdot
     */
    if (!agattr(g, AGNODE, "pos", NULL)) {
	g_print("There is no position info in graph %s in %s\n", agnameof(g), filename);
	agclose (g);
	return 0;
    }
    view->Topview->Graphdata.GraphFileName = gv_strdup(filename);
    return g;
}

/* add_graph_to_viewport_from_file:
 * returns 1 if successful else 0
 */
int add_graph_to_viewport_from_file(char *fileName)
{
    Agraph_t *graph = loadGraph(fileName);

    return add_graph_to_viewport(graph, fileName);
}

/* updateRecord:
 * Update fields which may be added dynamically.
 */
void updateRecord (Agraph_t* g)
{
    GN_size(g) = agattr (g, AGNODE, "size", 0);
    GN_visible(g) = agattr (g, AGNODE, "visible", 0);
    GN_selected(g) = agattr (g, AGNODE, "selected", 0);
    GN_labelattribute(g) = agattr (g, AGNODE, "nodelabelattribute", 0);

    GE_pos(g)=agattr(g,AGEDGE,"pos",0);
    GE_visible(g) = agattr (g, AGEDGE, "visible", 0);
    GE_selected(g) = agattr (g, AGEDGE, "selected", 0);
    GE_labelattribute(g) = agattr (g, AGEDGE, "edgelabelattribute", 0);
}

/* graphRecord:
 * add graphRec to graph if necessary.
 * update fields of graphRec.
 * We assume the graph has attributes nodelabelattribute, edgelabelattribute,
 * nodelabelcolor and edgelabelcolor from template.dot.
 * We assume nodes have pos attributes. 
 * Only size, visible, selected and edge pos may or may not be defined.
 */
static void
graphRecord (Agraph_t* g)
{
    agbindrec(g, "graphRec", sizeof(graphRec), true);

    GG_nodelabelcolor(g) = agattr (g, AGRAPH, "nodelabelcolor", 0);
    GG_edgelabelcolor(g) = agattr (g, AGRAPH, "edgelabelcolor", 0);
    GG_labelattribute(g) = agattr (g, AGRAPH, "nodelabelattribute", 0);
    GG_elabelattribute(g) = agattr (g, AGRAPH, "edgelabelattribute", 0);

    GN_pos(g) = agattr (g, AGNODE, "pos", 0);


    updateRecord (g);
}

void refreshViewport(void)
{
    Agraph_t *graph = view->g[view->activeGraph];
    view->refresh.color=1;
    view->refresh.pos=1;
    view->refresh.selection=1;
    load_settings_from_graph();

    if(view->guiMode!=GUI_FULLSCREEN)
	update_graph_from_settings(graph);
    set_viewport_settings_from_template(view, graph);
    graphRecord(graph);
    initSmGraph(graph,view->Topview);

    expose_event(view->drawing_area, NULL, NULL);
}

static void activate(int id)
{
    view->activeGraph = id;
    refreshViewport();
}

int add_graph_to_viewport(Agraph_t * graph, char *id)
{
    if (graph) {
	view->g = gv_recalloc(view->g, view->graphCount, view->graphCount + 1,
	                      sizeof(Agraph_t*));
	view->graphCount = view->graphCount + 1;
	view->g[view->graphCount - 1] = graph;

	gtk_combo_box_append_text(view->graphComboBox, id);
	gtk_combo_box_set_active(view->graphComboBox,view->graphCount-1);
	activate(view->graphCount - 1);
	return 1;
    } else {
	return 0;
    }

}
void switch_graph(int graphId)
{
    if (graphId >= view->graphCount || graphId < 0)
	return;			/*wrong entry */
    else
	activate(graphId);
}

/* save_graph_with_file_name:
 * saves graph with file name; if file name is NULL save as is
 */
int save_graph_with_file_name(Agraph_t * graph, char *fileName)
{
    int ret;
    FILE *output_file;
    update_graph_params(graph);
    if (fileName)
	output_file = fopen(fileName, "w");
    else if (view->Topview->Graphdata.GraphFileName)
	output_file = fopen(view->Topview->Graphdata.GraphFileName, "w");
    else {
	g_print("there is no file name to save! Programmer error\n");
	return 0;
    }
    if (output_file == NULL) {
	g_print("Cannot create file \n");
	return 0;
    }

    ret = agwrite(graph, output_file);
    fclose (output_file);
    if (ret) {
	g_print("%s successfully saved \n", fileName);
	return 1;
    }
    return 0;
}

/* save_graph:
 * save without prompt
 */
int save_graph(void)
{
    //check if there is an active graph
    if (view->activeGraph > -1) 
    {
	//check if active graph has a file name
	if (view->Topview->Graphdata.GraphFileName) {
	    return save_graph_with_file_name(view->g[view->activeGraph],
					     view->Topview->Graphdata.
					     GraphFileName);
	} else
	    return save_as_graph();
    }
    return 1;

}

/* save_as_graph:
 * save with prompt
 */
int save_as_graph(void)
{
    //check if there is an active graph
    if (view->activeGraph > -1) {
	GtkWidget *dialog = gtk_file_chooser_dialog_new("Save File",
					     NULL,
					     GTK_FILE_CHOOSER_ACTION_SAVE,
					     GTK_STOCK_CANCEL,
					     GTK_RESPONSE_CANCEL,
					     GTK_STOCK_SAVE,
					     GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER
						       (dialog), TRUE);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
	    char *filename =
		gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	    save_graph_with_file_name(view->g[view->activeGraph],
				      filename);
	    g_free(filename);
	    gtk_widget_destroy(dialog);

	    return 1;
	} else {
	    gtk_widget_destroy(dialog);
	    return 0;
	}
    }
    return 0;
}

void glexpose(void)
{
    expose_event(view->drawing_area, NULL, NULL);
}

static float interpol(float minv, float maxv, float minc, float maxc, float x)
{
    return (x - minv) * (maxc - minc) / (maxv - minv) + minc;
}

void getcolorfromschema(colorschemaset * sc, float l, float maxl,
			glCompColor * c)
{
    int ind;
    float percl = l / maxl;

    if (sc->smooth) {
	/* For smooth schemas, s[0].perc = 0, so we start with ind=1 */
	for (ind = 1; ind < sc->schemacount-1; ind++) {
	    if (percl < sc->s[ind].perc)
		break;
	}
	c->R =
	    interpol(sc->s[ind - 1].perc, sc->s[ind].perc,
		     sc->s[ind - 1].c.R, sc->s[ind].c.R, percl);
	c->G =
	    interpol(sc->s[ind - 1].perc, sc->s[ind].perc,
		     sc->s[ind - 1].c.G, sc->s[ind].c.G, percl);
	c->B =
	    interpol(sc->s[ind - 1].perc, sc->s[ind].perc,
		     sc->s[ind - 1].c.B, sc->s[ind].c.B, percl);
    }
    else {
	for (ind = 0; ind < sc->schemacount-1; ind++) {
	    if (percl < sc->s[ind].perc)
		break;
	}
	c->R = sc->s[ind].c.R;
	c->G = sc->s[ind].c.G;
	c->B = sc->s[ind].c.B;
    }

    c->A = 1;
}

/* set_color_theme_color:
 * Convert colors as strings to RGB
 */
static void set_color_theme_color(colorschemaset * sc, char **colorstr)
{
    int ind;
    int colorcnt = sc->schemacount;
    gvcolor_t cl;
    float av_perc;

    sc->smooth = 1;
    av_perc = 1.0 / (float) (colorcnt-1);
    for (ind = 0; ind < colorcnt; ind++) {
        colorxlate(colorstr[ind], &cl, RGBA_DOUBLE);
        sc->s[ind].c.R = cl.u.RGBA[0];
        sc->s[ind].c.G = cl.u.RGBA[1];
        sc->s[ind].c.B = cl.u.RGBA[2];
        sc->s[ind].c.A = cl.u.RGBA[3];
        sc->s[ind].perc = ind * av_perc;
    }
}

static void clear_color_theme(colorschemaset * cs)
{
    free(cs->s);
    free(cs);
}

static char *deep_blue[] = {
    "#C8CBED", "#9297D3", "#0000FF", "#2C2E41"
};
static char *pastel[] = {
    "#EBBE29", "#D58C4A", "#74AE09", "#893C49"
};
static char *magma[] = {
    "#E0061E", "#F0F143", "#95192B", "#EB712F"
};
static char *rain_forest[] = {
    "#1E6A10", "#2ABE0E", "#AEDD39", "#5EE88B"
};
#define CSZ(x) (sizeof(x)/sizeof(char*))
typedef struct {
    int cnt;
    char **colors;
} colordata;
static colordata palette[] = {
    {CSZ(deep_blue), deep_blue},
    {CSZ(pastel), pastel},
    {CSZ(magma), magma},
    {CSZ(rain_forest), rain_forest},
};
#define NUM_SCHEMES (sizeof(palette)/sizeof(colordata))

static colorschemaset *create_color_theme(int themeid)
{
    if (themeid < 0 || (int)NUM_SCHEMES <= themeid) {
	fprintf (stderr, "colorschemaset: illegal themeid %d\n", themeid);
	return view->colschms;
    }

    colorschemaset *s = gv_alloc(sizeof(colorschemaset));
    if (view->colschms)
	clear_color_theme(view->colschms);

    s->schemacount = palette[themeid].cnt;
    s->s = gv_calloc(s->schemacount, sizeof(colorschema));
    set_color_theme_color(s, palette[themeid].colors);

    return s;
}

