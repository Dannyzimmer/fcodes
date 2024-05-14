/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include "gui.h"
#include "viewport.h"
#include "gltemplate.h"
#include <cgraph/exit.h>
#include <glcomp/glutils.h>
#include "glexpose.h"
#include "glmotion.h"

#include <glcomp/glcompset.h>
#include "viewportcamera.h"
#include "gui/menucallbacks.h"
#include "arcball.h"
#include "appmouse.h"
static float begin_x = 0.0;
static float begin_y = 0.0;
static float dx = 0.0;
static float dy = 0.0;

/*mouse mode mapping funvtion from gtk to glcomp*/
static glMouseButtonType getGlCompMouseType(int n)
{
    switch (n) {
    case 1:
	return glMouseLeftButton;
    case 2:
	return glMouseMiddleButton;
    case 3:
	return glMouseRightButton;

    default:
	return glMouseLeftButton;
    }
}

/*
	initialize the gl , run only once!!
	params:gtk opgn gl canvas and optional data pointer
	return value:none
*/
static void realize(GtkWidget * widget, gpointer data)
{
    (void)data;

    GdkGLContext *glcontext = gtk_widget_get_gl_context(widget);
    GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(widget);

    /*smyrna does not use any ligthting affects but can be turned on for more effects in the future */
    GLfloat ambient[] = { 0.0, 0.0, 0.0, 1.0 };
    GLfloat diffuse[] = { 0.5, 0.5, 0.5, 1.0 };
    GLfloat position[] = { 0.0, 3.0, 3.0, 0.0 };

    GLfloat lmodel_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat local_view[] = { 0.0 };

	/*** OpenGL BEGIN ***/
    if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext))
	return;

    glClearColor(view->bgColor.R, view->bgColor.G, view->bgColor.B, view->bgColor.A);	//background color
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT);


    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, local_view);

    glFrontFace(GL_CW);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gdk_gl_drawable_gl_end(gldrawable);
  /*** OpenGL END ***/
}

/*
	set up GL for window size changes, run this when necesary (when window size or monitor resolution is changed)
	params:gtk opgn gl canvas , GdkEventConfigure object to retrieve window dimensions and custom data
	return value:true or false, fails (false) if cannot init gl
*/
static gboolean configure_event(GtkWidget * widget,
				GdkEventConfigure * event, gpointer data)
{
    (void)event;
    (void)data;

    int vPort[4];
    float aspect;
    GdkGLContext *glcontext = gtk_widget_get_gl_context(widget);
    GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(widget);
    view->w = widget->allocation.width;
    view->h = widget->allocation.height;
    if (view->widgets)
	glcompsetUpdateBorder(view->widgets, view->w, view->h);


	/*** OpenGL BEGIN ***/
    if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext))
	return FALSE;
    glViewport(0, 0, view->w, view->h);
    /* get current viewport */
    glGetIntegerv(GL_VIEWPORT, vPort);
    /* setup various opengl things that we need */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (widget->allocation.width > 1)
	init_arcBall(view->arcball, (GLfloat) view->w, (GLfloat) view->h);

    if (view->w > view->h) {
	aspect = (float) view->w / (float) view->h;
	glOrtho(-aspect * GL_VIEWPORT_FACTOR, aspect * GL_VIEWPORT_FACTOR,
		GL_VIEWPORT_FACTOR * -1, GL_VIEWPORT_FACTOR, -1500, 1500);
    } else {
	aspect = (float) view->h / (float) view->w;
	glOrtho(GL_VIEWPORT_FACTOR * -1, GL_VIEWPORT_FACTOR,
		-aspect * GL_VIEWPORT_FACTOR, aspect * GL_VIEWPORT_FACTOR,
		-1500, 1500);
    }

    glMatrixMode(GL_MODELVIEW);
    gdk_gl_drawable_gl_end(gldrawable);
	/*** OpenGL END ***/


    return TRUE;
}

/*
	expose function.real drawing takes place here, 
	params:gtk opgn gl canvas , GdkEventExpose object and custom data
	return value:true or false, fails (false) if cannot init gl
*/
gboolean expose_event(GtkWidget * widget, GdkEventExpose * event,
		      gpointer data)
{
    (void)event;
    (void)data;

    GdkGLContext *glcontext = gtk_widget_get_gl_context(widget);
    GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable(widget);

	/*** OpenGL BEGIN ***/
    if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext))
	return FALSE;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glexpose_main(view);	//draw all stuff
    /* Swap buffers */
    if (gdk_gl_drawable_is_double_buffered(gldrawable))
	gdk_gl_drawable_swap_buffers(gldrawable);
    else
	glFlush();
    gdk_gl_drawable_gl_end(gldrawable);
  /*** OpenGL END ***/
    if (view->initFile) {
	view->initFile = 0;
	if (view->activeGraph == 0)
	    close_graph(view);
	add_graph_to_viewport_from_file(view->initFileName);
    }
    return TRUE;
}

/*
	when a mouse button is clicked this function is called
	params:gtk opgn gl canvas , GdkEventButton object and custom data
	return value:true or false, fails (false) if cannot init gl
*/
static gboolean button_press_event(GtkWidget * widget,
				   GdkEventButton * event, gpointer data)
{
    (void)widget;
    (void)data;

    if (view->g == 0) return FALSE;

    begin_x = (float) event->x;
    begin_y = (float) event->y;
    view->widgets->common.functions.mousedown((glCompObj*)view->widgets,(GLfloat) event->x,(GLfloat) event->y,getGlCompMouseType(event->button));
    if (event->button == 1)	//left click
	appmouse_left_click_down(view,(int) event->x,(int) event->y);

    if (event->button == 3)	//right click
	appmouse_right_click_down(view,(int) event->x,(int) event->y);
    if (event->button == 2)	//middle click
	appmouse_middle_click_down(view,(int) event->x,(int) event->y);

    expose_event(view->drawing_area, NULL, NULL);

    return FALSE;
}

/*
	when a mouse button is released(always after click) this function is called
	params:gtk opgn gl canvas , GdkEventButton object and custom data
	return value:true or false, fails (false) if cannot init gl
*/
static gboolean button_release_event(GtkWidget * widget,
				     GdkEventButton * event, gpointer data)
{
    (void)widget;
    (void)data;

    if (view->widgets == 0) return FALSE;
    view->arcball->isDragging = 0;
    view->widgets->common.functions.mouseup((glCompObj*)view->widgets,(GLfloat) event->x,(GLfloat) event->y,getGlCompMouseType(event->button));


    if (event->button == 1)	//left click release
	appmouse_left_click_up(view,(int) event->x,(int) event->y);
    if (event->button == 3)	//right click
	appmouse_right_click_up(view,(int) event->x,(int) event->y);
    if (event->button == 2)	//right click
	appmouse_middle_click_up(view,(int) event->x,(int) event->y);

    expose_event(view->drawing_area, NULL, NULL);
    dx = 0.0;
    dy = 0.0;

    return FALSE;
}
static gboolean key_press_event(GtkWidget * widget, GdkEventKey * event, gpointer data)
{
    (void)widget;
    (void)data;

    appmouse_key_press(view,event->keyval);
    return FALSE;
}
static gboolean key_release_event(GtkWidget * widget, GdkEventKey * event, gpointer data)
{
    (void)widget;
    (void)event;
    (void)data;

    appmouse_key_release(view);
    return FALSE;
}


static gboolean
scroll_event(GtkWidget * widget, GdkEventScroll * event, gpointer data)
{
    (void)widget;
    (void)data;

    gdouble seconds;

    seconds = g_timer_elapsed(view->timer2, NULL);
    if (seconds > 0.005) {
	g_timer_stop(view->timer2);
	if (event->direction == 0)
	    view->mouse.dragX = -30;
	if (event->direction == 1)
	    view->mouse.dragX = +30;
	glmotion_zoom();
	glexpose();
	g_timer_start(view->timer2);

    }
    return TRUE;
}

/*
	when  mouse is moved over glcanvas this function is called
	params:gtk opgn gl canvas , GdkEventMotion object and custom data
	return value:always TRUE !!!
*/
static gboolean motion_notify_event(GtkWidget * widget,
				    GdkEventMotion * event, gpointer data)
{
    (void)data;

    float x = (float) event->x;
    float y = (float) event->y;

    bool redraw = false;
    if (view->widgets)
	view->widgets->common.functions.mouseover((glCompObj*)view->widgets, (GLfloat) x,(GLfloat) y);

    dx = x - begin_x;
    dy = y - begin_y;
    view->mouse.dragX = dx;
    view->mouse.dragY = dy;
    appmouse_move(view,(int)event->x,(int)event->y);

    if((view->mouse.t==glMouseLeftButton) && (view->mouse.down)  )
    {
	appmouse_left_drag(view,(int)event->x,(int)event->y);
	redraw = true;

    }
    if((view->mouse.t==glMouseRightButton) && (view->mouse.down))
    {
	appmouse_right_drag(view,(int)event->x,(int)event->y);
	redraw = true;
    }
    if((view->mouse.t==glMouseMiddleButton) && (view->mouse.down))
    {
	appmouse_middle_drag(view,(int)event->x,(int)event->y);
	redraw = true;
    }
    if(view->Topview->sel.selPoly.cnt > 0)
	redraw = true;

    begin_x = x;
    begin_y = y;


    if (redraw)
	gdk_window_invalidate_rect(widget->window, &widget->allocation,FALSE);
    return TRUE;
}

/*
	configures various opengl settings
	params:none
	return value:GdkGLConfig object
*/
GdkGLConfig *configure_gl(void)
{
    GdkGLConfig *glconfig;
    /* Try double-buffered visual */
    glconfig = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGB |
					 GDK_GL_MODE_DEPTH |
					 GDK_GL_MODE_DOUBLE);
    if (glconfig == NULL) {
	g_print("\n*** Cannot find the double-buffered visual.\n");
	g_print("\n*** Trying single-buffered visual.\n");

	/* Try single-buffered visual */
	glconfig = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGB |
					     GDK_GL_MODE_DEPTH);
	if (glconfig == NULL) {
	    g_print("*** No appropriate OpenGL-capable visual found.\n");
	    graphviz_exit(1);
	}
    }

    return glconfig;
}

/*
	run this function only once to create and customize gtk based opengl canvas
	all signal, socket connections are done here for opengl interractions
	params:gl config object, gtk container widget for opengl canvas
	return value:none
*/
void create_window(GdkGLConfig * glconfig, GtkWidget * vbox)
{
    gint major, minor;

    /*
     * Query OpenGL extension version.
     */

    gdk_gl_query_version(&major, &minor);

    /* Try double-buffered visual */

    /* Drawing area for drawing OpenGL scene. */
    view->drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(view->drawing_area, 300, 300);
    /* Set OpenGL-capability to the widget. */
    gtk_widget_set_gl_capability(view->drawing_area,
				 glconfig, NULL, TRUE, GDK_GL_RGBA_TYPE);

    gtk_widget_add_events(view->drawing_area,
			  GDK_BUTTON_MOTION_MASK |
			  GDK_POINTER_MOTION_MASK |
			  GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS |
			  GDK_BUTTON_RELEASE_MASK |
			  GDK_SCROLL | GDK_VISIBILITY_NOTIFY_MASK |  GDK_KEY_PRESS_MASK	| GDK_KEY_RELEASE_MASK	);

    g_signal_connect_after(G_OBJECT(view->drawing_area), "realize",
			   G_CALLBACK(realize), NULL);
    g_signal_connect(G_OBJECT(view->drawing_area), "configure_event",
		     G_CALLBACK(configure_event), NULL);
    g_signal_connect(G_OBJECT(view->drawing_area), "expose_event",
		     G_CALLBACK(expose_event), NULL);

    g_signal_connect(G_OBJECT(view->drawing_area), "button_press_event",
		     G_CALLBACK(button_press_event), NULL);

    g_signal_connect(G_OBJECT(view->drawing_area), "button_release_event",G_CALLBACK(button_release_event), NULL);
    g_signal_connect(G_OBJECT(view->drawing_area), "key_release_event", G_CALLBACK(key_release_event), NULL);
    g_signal_connect(G_OBJECT(view->drawing_area), "key_press_event", G_CALLBACK(key_press_event), NULL);
    g_signal_connect(G_OBJECT(view->drawing_area), "scroll_event",
		     G_CALLBACK(scroll_event), NULL);

    g_signal_connect(G_OBJECT(view->drawing_area), "motion_notify_event",
		     G_CALLBACK(motion_notify_event), NULL);

    gtk_box_pack_start(GTK_BOX(vbox), view->drawing_area, TRUE, TRUE, 0);

    gtk_widget_show(view->drawing_area);



    gtk_widget_add_events(glade_xml_get_widget(xml, "frmMain"),
			  GDK_BUTTON_MOTION_MASK |
			  GDK_POINTER_MOTION_MASK |
			  GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS |
			  GDK_BUTTON_RELEASE_MASK |
			  GDK_SCROLL | GDK_VISIBILITY_NOTIFY_MASK |  GDK_KEY_PRESS_MASK	| GDK_KEY_RELEASE_MASK	);


    g_signal_connect(G_OBJECT(glade_xml_get_widget(xml, "frmMain")), "key_release_event", G_CALLBACK(key_release_event), NULL);
    g_signal_connect(G_OBJECT(glade_xml_get_widget(xml, "frmMain")), "key_press_event", G_CALLBACK(key_press_event), NULL);
}
