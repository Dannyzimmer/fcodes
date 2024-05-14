/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include "appmouse.h"
#include "topfisheyeview.h"
#include "arcball.h"
#include "glmotion.h"
#include "hotkeymap.h"

#include "selectionfuncs.h"
#include "topviewfuncs.h"

static int lastAction;

static void apply_actions(ViewInfo* v,int x,int y)
{
    int a;
    gdouble seconds;
    a=get_mode(v);
    if((a==MM_PAN) && (view->guiMode==GUI_FULLSCREEN) &&((v->active_camera >= 0)))
	a=MM_ROTATE;	
    switch (a) {
    case MM_ROTATE :
	seconds = g_timer_elapsed(view->timer3, NULL);
	if (seconds > 0.1) {
	    g_timer_stop(view->timer3);
		view->arcball->MousePt.s.X = (GLfloat) x;
		view->arcball->MousePt.s.Y = (GLfloat) y;
		if (!view->arcball->isDragging) {
		    arcmouseClick();
		    view->arcball->isDragging = 1;
		}
		else 
		    arcmouseDrag();
	    g_timer_start(view->timer3);
	}
	return;
    case MM_PAN :
	glmotion_pan(v);
	break;
    case MM_MOVE :
        /* move_TVnodes(); */
	break;
    case MM_RECTANGULAR_SELECT :
	if (!view->mouse.down) {
	    pick_objects_rect(view->g[view->activeGraph]) ;
	}
	break;
    case MM_POLYGON_SELECT :
	add_selpoly(view->g[view->activeGraph],&view->Topview->sel.selPoly,view->mouse.GLfinalPos);
	break;
    case MM_SINGLE_SELECT :
	pick_object_xyz(view->g[view->activeGraph],view->Topview,view->mouse.GLfinalPos.x,view->mouse.GLfinalPos.y,view->mouse.GLfinalPos.z);
	break;
    case MM_FISHEYE_PICK :
	if (view->activeGraph >= 0) {
	    if (view->Topview->fisheyeParams.active) 
		changetopfishfocus(view->Topview, &view->mouse.GLpos.x, &view->mouse.GLpos.y, 1);

	}
	break;
    }
    lastAction=a;
}

static void appmouse_down(ViewInfo* v,int x,int y)
{
    view->mouse.dragX = 0;
    view->mouse.dragY = 0;
    v->mouse.down=1;
    v->mouse.pos.x=x;
    v->mouse.pos.y=y;
    
    to3D(x,y,&v->mouse.GLinitPos.x,&v->mouse.GLinitPos.y,&v->mouse.GLinitPos.z);
    to3D( x,y, &v->mouse.GLpos.x,&v->mouse.GLpos.y,&v->mouse.GLpos.z);
}
static void appmouse_up(ViewInfo* v,int x,int y)
{
    v->mouse.down=0;
    to3D(x,y, &v->mouse.GLfinalPos.x,&v->mouse.GLfinalPos.y,&v->mouse.GLfinalPos.z);
    apply_actions(v,x,y);
    view->mouse.dragX = 0;
    view->mouse.dragY = 0;
    view->arcball->isDragging=0;


}
static void appmouse_drag(ViewInfo* v,int x,int y)
{
    v->mouse.pos.x=x;
    v->mouse.pos.y=y;
    to3D( x,y, &v->mouse.GLpos.x,&v->mouse.GLpos.y,&v->mouse.GLpos.z);
    apply_actions(v,x,y);
}

void appmouse_left_click_down(ViewInfo* v,int x,int y)
{
       v->mouse.t=glMouseLeftButton;
       appmouse_down(v,x,y);


}
void appmouse_left_click_up(ViewInfo* v,int x,int y)
{
    appmouse_up(v,x,y);
}
void appmouse_left_drag(ViewInfo* v,int x,int y)
{
    appmouse_drag(v,x,y);



}
void appmouse_right_click_down(ViewInfo* v,int x,int y)
{
    v->mouse.t=glMouseRightButton;
    appmouse_down(v,x,y);


}
void appmouse_right_click_up(ViewInfo* v,int x,int y)
{
    appmouse_up(v,x,y);
 
}
void appmouse_right_drag(ViewInfo* v,int x,int y)
{
        
    appmouse_drag(v,x,y);

}


void appmouse_middle_click_down(ViewInfo* v,int x,int y)
{
    v->mouse.t=glMouseMiddleButton;
    appmouse_down(v,x,y);


}
void appmouse_middle_click_up(ViewInfo* v,int x,int y)
{
    appmouse_up(v,x,y);
 
}
void appmouse_middle_drag(ViewInfo* v,int x,int y)
{
        
    appmouse_drag(v,x,y);

}
void appmouse_move(ViewInfo* v,int x,int y)
{
    to3D( x,y, &v->mouse.GLpos.x,&v->mouse.GLpos.y,&v->mouse.GLpos.z);
}
void appmouse_key_release(ViewInfo* v)
{
    if(lastAction==MM_POLYGON_SELECT)
    {
	clear_selpoly(&view->Topview->sel.selPoly);	
	glexpose();
    }
    v->keymap.down=0;
    v->keymap.keyVal=0;
}
void appmouse_key_press(ViewInfo* v,int key)
{
    v->keymap.down=1;
    v->keymap.keyVal=key;
}


