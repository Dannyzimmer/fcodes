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

#include "draw.h"
#include <glcomp/opengl.h>

extern void pick_objects_rect(Agraph_t* g) ;
extern void deselect_all(Agraph_t* g);
extern void add_selpoly(Agraph_t* g,glCompPoly* selPoly,glCompPoint pt);
extern void clear_selpoly(glCompPoly* sp);
