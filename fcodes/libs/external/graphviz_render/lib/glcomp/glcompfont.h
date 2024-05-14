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

#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <glcomp/glpangofont.h>
typedef float GLfloat;
#endif
#include <glcomp/opengl.h>
#include <glcomp/glcompdefs.h>
#include <GL/glut.h>

#ifdef __cplusplus
extern "C" {
#endif

    void glprintfglut(void *font, GLfloat xpos, GLfloat ypos, GLfloat zpos, char *bf);

    glCompFont *glNewFont(glCompSet * s, char *text, glCompColor * c,
                          char *fontdesc, int fs,int is2D);
    glCompFont *glNewFontFromParent(glCompObj * o, char *text);
    void glDeleteFont(glCompFont * f);
    void glCompDrawText(glCompFont * f,GLfloat x,GLfloat y);
    void glCompRenderText(glCompFont * f, glCompObj * parentObj);
    void glCompDrawText3D(glCompFont *f, GLfloat x, GLfloat y, double z,
                          GLfloat w, GLfloat h);

#ifdef __cplusplus
}
#endif
