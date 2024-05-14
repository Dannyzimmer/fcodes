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
#include <glcomp/glcomppanel.h>
#include <glcomp/glcompfont.h>
#include <glcomp/glcompset.h>
#include <glcomp/glcomptexture.h>
#include <glcomp/glutils.h>

glCompPanel *glCompPanelNew(glCompObj * parentObj, GLfloat x, GLfloat y,
			    GLfloat w, GLfloat h)
{
    glCompPanel *p = gv_alloc(sizeof(glCompPanel));
    glCompInitCommon((glCompObj *) p, parentObj, x, y);

    p->shadowcolor.R = GLCOMPSET_PANEL_SHADOW_COLOR_R;
    p->shadowcolor.G = GLCOMPSET_PANEL_SHADOW_COLOR_G;
    p->shadowcolor.B = GLCOMPSET_PANEL_SHADOW_COLOR_B;
    p->shadowcolor.A = GLCOMPSET_PANEL_SHADOW_COLOR_A;
    p->shadowwidth = GLCOMPSET_PANEL_SHADOW_WIDTH;
    p->common.borderWidth = GLCOMPSET_PANEL_BORDERWIDTH;


    p->common.width = w;
    p->common.height = h;

    p->common.font = glNewFontFromParent((glCompObj *) p, NULL);
    p->text = NULL;
    p->common.functions.draw = (glcompdrawfunc_t)glCompPanelDraw;
    p->image = NULL;
    return p;
}

int glCompPanelDraw(glCompObj * o)
{
    glCompPanel *p;
    glCompCommon ref;
    glCompRect r;
    p = (glCompPanel *) o;
    ref = p->common;
    glCompCalcWidget((glCompCommon *) p->common.parent, &p->common, &ref);
    p->objType = glPanelObj;

    if (!p->common.visible)
	return 0;
    /*draw shadow */
    glColor4f(p->shadowcolor.R, p->shadowcolor.G, p->shadowcolor.B,
              p->shadowcolor.A);
    r.h = p->shadowwidth;
    r.w = ref.width;
    r.pos.x = ref.pos.x + p->shadowwidth;
    r.pos.y = ref.pos.y - p->shadowwidth;
    r.pos.z = -0.001f;
    glCompDrawRectangle(&r);
    r.h = ref.height;
    r.w = p->shadowwidth;
    r.pos.x = ref.pos.x + ref.width;
    r.pos.y = ref.pos.y - p->shadowwidth;
    r.pos.z = -0.001f;
    glCompDrawRectangle(&r);
    /*draw panel */
    glCompDrawRectPrism(&ref.pos, ref.width, ref.height,
			p->common.borderWidth, 0.01f, &ref.color, 1);
    /*draw image if there is */
    if (p->image) {
	p->image->common.callbacks.draw(p->image);
    }
    return 1;
}
