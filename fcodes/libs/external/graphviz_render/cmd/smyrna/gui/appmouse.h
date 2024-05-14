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

extern void appmouse_left_click_down(ViewInfo* v,int x,int y);
extern void appmouse_left_click_up(ViewInfo* v,int x,int y);
extern void appmouse_left_drag(ViewInfo* v,int x,int y);

extern void appmouse_right_click_down(ViewInfo* v,int x,int y);
extern void appmouse_right_click_up(ViewInfo* v,int x,int y);
extern void appmouse_right_drag(ViewInfo* v,int x,int y);

extern void appmouse_middle_click_down(ViewInfo* v,int x,int y);
extern void appmouse_middle_click_up(ViewInfo* v,int x,int y);
extern void appmouse_middle_drag(ViewInfo* v,int x,int y);
extern void appmouse_move(ViewInfo* v,int x,int y);
extern void appmouse_key_release(ViewInfo* v);
extern void appmouse_key_press(ViewInfo* v,int key);
