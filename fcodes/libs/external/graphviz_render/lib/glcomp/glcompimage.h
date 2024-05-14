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

    extern glCompImage *glCompImageNewFile(GLfloat x, GLfloat y, char *imgfile);
    extern glCompImage *glCompImageNew(glCompObj * par, GLfloat x,
				       GLfloat y);
    extern void glCompImageDelete(glCompImage * p);
    extern int glCompImageLoad(glCompImage * i, unsigned char *data,
			       int width, int height,int is2D);
    extern int glCompImageLoadPng(glCompImage * i, char *pngFile,int is2D);
    extern void glCompImageDraw(void *obj);

#ifdef __cplusplus
}
#endif
