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

#define B_LSHIFT	    65505
#define B_RSHIFT	    65506
#define B_LCTRL		    65507
#define B_RCTRL		    65508

extern void load_mouse_actions (ViewInfo* v);

extern int get_mode(ViewInfo* v);
