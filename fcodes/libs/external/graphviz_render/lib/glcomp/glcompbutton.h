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

#include <glcomp/glcompdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

    extern glCompButton *glCompButtonNew(glCompObj * par, GLfloat x,
					 GLfloat y, GLfloat w, GLfloat h,
					 char *caption);
    extern void glCompButtonDraw(glCompButton * p);
    extern int glCompButtonAddPngGlyph(glCompButton * b, char *fileName);
    extern void glCompButtonClick(glCompObj * o, GLfloat x, GLfloat y,
				  glMouseButtonType t);
    extern void glCompButtonDoubleClick(glCompObj * o, GLfloat x,
					GLfloat y, glMouseButtonType t);
    extern void glCompButtonMouseDown(glCompObj * o, GLfloat x, GLfloat y,
				      glMouseButtonType t);
    extern void glCompButtonMouseIn(glCompObj * o, GLfloat x, GLfloat y);
    extern void glCompButtonMouseOut(glCompObj * o, GLfloat x, GLfloat y);
    extern void glCompButtonMouseOver(glCompObj * o, GLfloat x, GLfloat y);
    extern void glCompButtonMouseUp(glCompObj * o, GLfloat x, GLfloat y,
				    glMouseButtonType t);
    extern void glCompButtonHide(glCompButton * p);
    extern void glCompButtonShow(glCompButton * p);

#ifdef __cplusplus
}
#endif
