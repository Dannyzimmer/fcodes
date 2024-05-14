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
#include "hotkeymap.h"

static int get_mouse_mode(const char *s)
{
    if (strcmp(s, "MM_PAN") == 0)
	return MM_PAN;
    if (strcmp(s, "MM_ZOOM") == 0)
	return MM_ZOOM;
    if (strcmp(s, "MM_ROTATE") == 0)
	return MM_ROTATE;
    if (strcmp(s, "MM_SINGLE_SELECT") == 0)
	return MM_SINGLE_SELECT;
    if (strcmp(s, "MM_RECTANGULAR_SELECT") == 0)
	return MM_RECTANGULAR_SELECT;
    if (strcmp(s, "MM_RECTANGULAR_X_SELECT") == 0)
	return MM_RECTANGULAR_X_SELECT;

    if (strcmp(s, "MM_POLYGON_SELECT") == 0)
	return MM_POLYGON_SELECT;

    if (strcmp(s, "MM_MOVE") == 0)
	return MM_MOVE;
    if (strcmp(s, "MM_MOVE") == 0)
	return MM_MOVE;
    if (strcmp(s, "MM_MAGNIFIER") == 0)
	return MM_MAGNIFIER;
    if (strcmp(s, "MM_FISHEYE_MAGNIFIER") == 0)
	return MM_FISHEYE_MAGNIFIER;
    if (strcmp(s, "MM_FISHEYE_PICK") == 0)
	return MM_FISHEYE_PICK;

    return -1;
}

static int get_button(const char *s)
{
    if(view->guiMode != GUI_FULLSCREEN)
    {
	if (strcmp(s, "B_LSHIFT") == 0)
	    return B_LSHIFT;
	if (strcmp(s, "B_RSHIFT") == 0)
	    return B_RSHIFT;
	if (strcmp(s, "B_LCTRL") == 0)
	    return B_LCTRL;
	if (strcmp(s, "B_RCTRL") == 0)
	    return B_RCTRL;
	if (strcmp(s, "0") == 0)
	    return 0;
    }
    else
    {
		int mod = glutGetModifiers();
		if (mod == GLUT_ACTIVE_ALT)

	if (strcmp(s, "B_LSHIFT") == 0)
	    return GLUT_ACTIVE_SHIFT;
	if (strcmp(s, "B_RSHIFT") == 0)
	    return GLUT_ACTIVE_SHIFT;
	if (strcmp(s, "B_LCTRL") == 0)
	    return GLUT_ACTIVE_CTRL;
	if (strcmp(s, "B_RCTRL") == 0)
	    return GLUT_ACTIVE_CTRL;
	if (strcmp(s, "0") == 0)
	    return 0;
    }
    return 0;
}

static int get_view_mode(const char *s)
{
    if (strcmp(s, "ALL") == 0)
	return smyrna_all;
    if (strcmp(s, "2D") == 0)
	return smyrna_2D;
    if (strcmp(s, "3D") == 0)
	return smyrna_3D;
    if (strcmp(s, "FISHEYE") == 0)
	return smyrna_fisheye;
    if (strcmp(s, "NO_FISHEYE") == 0)
	return smyrna_all_but_fisheye;
    return -1;
}

static int get_mouse_button(const char *s)
{

    if (strcmp(s, "LEFT") == 0)
	return glMouseLeftButton;
    if (strcmp(s, "RIGHT") == 0)
	return glMouseRightButton;
    if (strcmp(s, "MIDDLE") == 0)
	return glMouseMiddleButton;
    return -1;
}

static int get_drag(const char *s)
{
    if (s[0] == '1')
	return 1;
    return 0;
}

void load_mouse_actions(ViewInfo * v)
{
    // file parsing is temporarily not available
    int i = 0;
    FILE *file;
    char linebuf[BUFSIZ];
    char *a;
    char *action_file = smyrnaPath("mouse_actions.txt");
    file = fopen(action_file, "r");
    if (file != NULL) {
	int ind = 0;
	while (fgets(linebuf, BUFSIZ, file) != NULL) {
	    int idx = 0;
	    a = strtok(linebuf, ",");

	    if (linebuf[0] == '#' || linebuf[0] == ' ' || strlen(linebuf) == 0)
		continue;

	    v->mouse_actions = gv_recalloc(v->mouse_actions, v->mouse_action_count,
	                                   v->mouse_action_count + 1,
	                                   sizeof(mouse_action_t));
	    v->mouse_action_count++;
	    v->mouse_actions[ind].action = get_mouse_mode(a);
	    v->mouse_actions[ind].index = i;

	    while ((a = strtok(NULL, ","))) {
		//#Action(0),hotkey(1),view_mode(2),mouse_button(3),drag(4)
		switch (idx) {
		case 0:
		    v->mouse_actions[ind].hotkey = get_button(a);
		    break;
		case 1:
		    v->mouse_actions[ind].mode = get_view_mode(a);
		    break;
		case 2:
		    v->mouse_actions[ind].type = get_mouse_button(a);
		    break;
		case 3:
		    v->mouse_actions[ind].drag = get_drag(a);
		    break;

		}
		idx++;
	    }
	    ind++;
	}
	fclose(file);
    }
    free(action_file);
}

int get_mode(ViewInfo * v)
{
    glMouseButtonType curMouseType = v->mouse.t;
    int curDragging = ((v->mouse.dragX != 0) || (v->mouse.dragY != 0));
    smyrna_view_mode view_mode;
    view_mode = smyrna_2D;
    if (v->active_camera >= 0)
	view_mode = smyrna_3D;
    if (v->Topview->fisheyeParams.active)
	view_mode = smyrna_fisheye;


    for (size_t ind = 0; ind < v->mouse_action_count; ind++) {

	if ((v->mouse_actions[ind].hotkey == v->keymap.keyVal)
	    && (v->mouse_actions[ind].type == curMouseType)
	    && (v->mouse_actions[ind].drag == curDragging)
	    &&
	    ((v->mouse_actions[ind].mode == view_mode)
	     || (v->mouse_actions[ind].mode == smyrna_all)
	     || ((v->mouse_actions[ind].mode == smyrna_all_but_fisheye)
		 && (view_mode != smyrna_fisheye))
	    )) {
	    return v->mouse_actions[ind].action;

	}
    }
    return -1;
}
