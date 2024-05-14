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

#include "smyrnadefs.h"

    typedef struct {
	GLUtesselator *tobj;
	GLenum windingRule;
    }tessPoly;
    extern void drawTessPolygon(sdot_op* p);
    extern int testDraw(void);
