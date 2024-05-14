/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <cgraph/alloc.h>
#include "viewportcamera.h"
#include "gui.h"
#include <math.h>
#include <glcomp/glcompbutton.h>
#include <glcomp/glcomplabel.h>
#include <glcomp/glcomppanel.h>


static viewport_camera *new_viewport_camera(void) {
    return gv_alloc(sizeof(viewport_camera));
}

static viewport_camera *add_camera_to_viewport(ViewInfo *vi) {
    vi->cameras = gv_recalloc(vi->cameras, vi->camera_count,
                              vi->camera_count + 1,
                              sizeof(viewport_camera*));
    vi->camera_count++;
    vi->cameras[vi->camera_count - 1] = new_viewport_camera();
    vi->active_camera = vi->camera_count - 1;
    return vi->cameras[vi->camera_count - 1];
}

void menu_click_add_camera(void)
{
    viewport_camera *c;
    /*add test cameras */
    c = add_camera_to_viewport(view);
    c->targetx = view->panx;
    c->targety = view->pany;
    c->targetz = view->panz;
    c->x = view->panx;
    c->y = view->pany;
    c->z = view->zoom;

    c->r = view->zoom * -1;
}
