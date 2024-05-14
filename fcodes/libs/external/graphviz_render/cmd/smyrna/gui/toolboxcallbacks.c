/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include "toolboxcallbacks.h"
#include "viewport.h"

#include "gltemplate.h"
#include <glcomp/glutils.h>
#include "glmotion.h"

void btnToolZoomOut_clicked(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    glmotion_zoom_inc(0);
}

void btnToolZoomFit_clicked(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    float z = view->active_camera >= 0 ? view->cameras[view->active_camera]->r
                                       : -view->zoom;

    float GDX = view->bdxRight / z - view->bdxLeft / z;
    float SDX = view->clipX2 - view->clipX1;
    float GDY = view->bdyTop / z - view->bdyBottom / z;
    float SDY = view->clipY2 - view->clipY1;

    if (SDX / GDX <= SDY / GDY) {
        if (view->active_camera >= 0) {
            view->cameras[view->active_camera]->r /= SDX / GDX;
        } else {
            view->zoom /= SDX / GDX;
        }
    } else {
        if (view->active_camera >= 0) {
            view->cameras[view->active_camera]->r /= SDY / GDY;
        } else {
            view->zoom /= SDY / GDY;
        }

    }
    btnToolFit_clicked(NULL, NULL);
}

void btnToolFit_clicked(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    float scx, scy, gcx, gcy, z;

    (view->active_camera >= 0)
	? (z = view->cameras[view->active_camera]->r) : (z =
							 view->zoom * -1);

    gcx = view->bdxLeft / z + (view->bdxRight / z - view->bdxLeft / z) / 2.0f;
    scx = view->clipX1 + (view->clipX2 - view->clipX1) / 2.0f;
    gcy = view->bdyBottom / z + (view->bdyTop / z - view->bdyBottom / z) / 2.0f;
    scy = view->clipY1 + (view->clipY2 - view->clipY1) / 2.0f;

    if (view->active_camera >= 0) {
	view->cameras[view->active_camera]->targetx += (gcx - scx);
	view->cameras[view->active_camera]->targety += (gcx - scy);
    } else {
	view->panx += gcx - scx;
	view->pany += gcy - scy;
    }
    view->Topview->fitin_zoom = view->zoom;

    glexpose();
}

void on_btnActivateGraph_clicked(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    int graphId;
    graphId = gtk_combo_box_get_active(view->graphComboBox);
    /* fprintf (stderr, "switch to graph %d\n",graphId); */
    switch_graph(graphId);
}
