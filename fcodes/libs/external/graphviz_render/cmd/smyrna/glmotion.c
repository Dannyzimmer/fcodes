/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include "glmotion.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkgl.h>
#include <gdk/gdkcursor.h>
#include "draw.h"
#include <glcomp/glutils.h>
#include "hotkeymap.h"

/*real zoom in out is done here, all other functions send this one what they desire, it is not guranteed,*/
static void graph_zoom(float real_zoom)
{
    float old_zoom;

    if (view->active_camera == -1)
		old_zoom = view->zoom;
    else
		old_zoom = view->cameras[view->active_camera]->r;

    if (real_zoom < view->Topview->fitin_zoom * MAX_ZOOM)
		real_zoom = view->Topview->fitin_zoom * MAX_ZOOM;
    if (real_zoom > view->Topview->fitin_zoom * MIN_ZOOM)
		real_zoom = view->Topview->fitin_zoom * MIN_ZOOM;
    if (view->active_camera == -1)
		view->zoom = real_zoom;
    else
		view->cameras[view->active_camera]->r = real_zoom * -1;
    /*adjust pan values */
    view->panx = old_zoom * view->panx / real_zoom;
    view->pany = old_zoom * view->pany / real_zoom;
}

void glmotion_zoom_inc(int zoomin)
{
    if (zoomin)			/*zooming in , zoom value should be decreased */
	graph_zoom(view->zoom - view->zoom * 0.25f);
    else
	graph_zoom(view->zoom + view->zoom * 0.25f);	/*zoom out */
    glexpose();

}

void glmotion_zoom(void)
{
    float real_zoom;
    if (view->active_camera == -1) {
	real_zoom =
	    view->zoom + view->mouse.dragX / 10 * (view->zoom * -1 / 20);
    } else {
	real_zoom =
	    (view->cameras[view->active_camera]->r +
	     view->mouse.dragX / 10 * (view->cameras[view->active_camera]->r /
				    20)) * -1;
    }
    graph_zoom(real_zoom);

}

void glmotion_pan(ViewInfo * v)
{
    float gldx, gldy;
    if (v->active_camera == -1) {
	gldx = GetOGLDistance((int) v->mouse.dragX) / v->zoom * -1;
	gldy = GetOGLDistance((int) v->mouse.dragY) / v->zoom * -1;
	v->panx = v->panx - gldx;
	v->pany = v->pany + gldy;
    } else {
	gldx =
	    GetOGLDistance((int) v->mouse.dragX) /
	    v->cameras[v->active_camera]->r;
	gldy =
	    GetOGLDistance((int) v->mouse.dragY) /
	    v->cameras[v->active_camera]->r;
	v->cameras[v->active_camera]->x -= gldx;
	v->cameras[v->active_camera]->y -= gldy;
	v->cameras[v->active_camera]->targetx -= gldx;
	v->cameras[v->active_camera]->targety += gldy;
    }
}
