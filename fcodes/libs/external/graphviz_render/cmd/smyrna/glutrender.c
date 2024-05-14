/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <cgraph/exit.h>

#include "glutrender.h"
#include "viewport.h"
#include "arcball.h"
#include "appmouse.h"
#include "glexpose.h"
#include "glcompui.h"

    /*call backs */

static float begin_x = 0.0;
static float begin_y = 0.0;
static float dx = 0.0;
static float dy = 0.0;

/*mouse mode mapping funvtion from glut to glcomp*/
static glMouseButtonType getGlCompMouseType(int n)
{
    switch (n) {
    case GLUT_LEFT_BUTTON:
	return glMouseLeftButton;
    case GLUT_RIGHT_BUTTON:
	return glMouseMiddleButton;
    case GLUT_MIDDLE_BUTTON:
	return glMouseRightButton;

    default:
	return glMouseLeftButton;
    }
}


static void cb_reshape(int width, int height)
{
    int vPort[4];
    float aspect;
    view->w = width;
    view->h = height;
    if (view->widgets)
	glcompsetUpdateBorder(view->widgets, view->w, view->h);
    glViewport(0, 0, view->w, view->h);
    /* get current viewport */
    glGetIntegerv(GL_VIEWPORT, vPort);
    /* setup various opengl things that we need */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    init_arcBall(view->arcball, (GLfloat) view->w, (GLfloat) view->h);
    if (view->w > view->h) {
	aspect = (float) view->w / (float) view->h;
	glOrtho(-aspect * GL_VIEWPORT_FACTOR, aspect * GL_VIEWPORT_FACTOR,
		GL_VIEWPORT_FACTOR * -1, GL_VIEWPORT_FACTOR, -1500, 1500);
    } else {
	aspect = (float) view->h / (float) view->w;
	glOrtho(GL_VIEWPORT_FACTOR * -1, GL_VIEWPORT_FACTOR,
		-aspect * GL_VIEWPORT_FACTOR, aspect * GL_VIEWPORT_FACTOR,
		-1500, 1500);
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	/*** OpenGL END ***/

}

static void cb_display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glexpose_main(view);	//draw all stuff
    glutSwapBuffers();
    if (view->initFile) {
	view->initFile = 0;
	if (view->activeGraph == 0)
	    close_graph(view);
	add_graph_to_viewport_from_file(view->initFileName);
    }

}

static void cb_mouseclick(int button, int state, int x, int y)
{
    if (view->g == 0) return;
    begin_x = (float) x;
    begin_y = (float) y;
    if(state==GLUT_DOWN)
    {
	view->widgets->common.functions.mousedown((glCompObj*)view->widgets,(GLfloat) x,(GLfloat)y,getGlCompMouseType(button));
	if (button ==  GLUT_LEFT_BUTTON)
	    appmouse_left_click_down(view,x,y);
        if (button ==  GLUT_RIGHT_BUTTON)
	    appmouse_right_click_down(view,x,y);
	if (button ==  GLUT_MIDDLE_BUTTON)
	    appmouse_middle_click_down(view,x,y);
    }
    else
    {
	view->arcball->isDragging = 0;
	view->widgets->common.functions.mouseup((glCompObj*)view->widgets,(GLfloat) x,(GLfloat) y,getGlCompMouseType(button));

	if (button ==  GLUT_LEFT_BUTTON)
	    appmouse_left_click_up(view,x,y);
	if (button ==  GLUT_LEFT_BUTTON)
	    appmouse_right_click_up(view,x,y);
	if (button ==  GLUT_MIDDLE_BUTTON)
	    appmouse_middle_click_up(view,x,y);
	dx = 0.0;
	dy = 0.0;
    }
    cb_display();

}

// mouse moving with a button clicked (dragging)
static void cb_drag(int X, int Y)
{

    float x = (float) X;
    float y = (float) Y;

    if (view->widgets)
	view->widgets->common.functions.mouseover((glCompObj*)view->widgets, (GLfloat) x,(GLfloat) y);

    dx = x - begin_x;
    dy = y - begin_y;
    view->mouse.dragX = dx;
    view->mouse.dragY = dy;
    appmouse_move(view,x,y);

    if((view->mouse.t==glMouseLeftButton) && (view->mouse.down)  )
	appmouse_left_drag(view,x,y);
    if((view->mouse.t==glMouseRightButton) && (view->mouse.down))
	appmouse_right_drag(view,x,y);
    if((view->mouse.t==glMouseMiddleButton) && (view->mouse.down))
	appmouse_middle_drag(view,x,y);
    begin_x = x;
    begin_y = y;
    cb_display();

}

static void cb_keyboard(unsigned char key, int x, int y)
{
    (void)x;
    (void)y;

    if (key==27)    /*ESC*/
	graphviz_exit(1);
    if(key=='3')
	switch2D3D(NULL, 0, 0,glMouseLeftButton);
    if(key=='c')
        menu_click_center(NULL, 0, 0,glMouseLeftButton);

    if(key=='+')
        menu_click_zoom_plus(NULL, 0, 0,glMouseLeftButton);
    if(key=='-')
        menu_click_zoom_minus(NULL, 0, 0,glMouseLeftButton);
    if(key=='p')
        menu_click_pan(NULL, 0, 0,glMouseLeftButton);
    

    appmouse_key_press(view,key);
}

static void cb_keyboard_up(unsigned char key, int x, int y)
{
    (void)key;
    (void)x;
    (void)y;

    appmouse_key_release(view);
}

static void cb_special_key(int key, int x, int y)
{
    (void)x;
    (void)y;

    if(key==GLUT_KEY_F1)
    {
	printf("Currently help is not available\n");
    }
    appmouse_key_press(view,key);

}

static void cb_special_key_up(int key, int x, int y)
{
    (void)x;
    (void)y;

    if(key==GLUT_KEY_F1)
    {
	printf("Currently help is not available\n");
    }
    appmouse_key_release(view);

}

static void cb_game_mode(char* optArg)
{
    
    glutGameModeString(optArg);
    if (glutGameModeGet(GLUT_GAME_MODE_POSSIBLE)) 
    {
    	glutEnterGameMode();
    }
    else 
    {
	printf("smyrna cannot initialize requested screen resolution and rate!\n");
	graphviz_exit(-1);

    }
}

int cb_glutinit(int w, int h, int *argcp, char *argv[], char *optArg) {
    /*
    w,h: width and height of the window in pixels
    argcp argv: main function's parameters, required for glutinit
    */



    glutInit(argcp,argv); //this is required by some OS.

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	// The Type Of Depth Testing To Do
        glDisable(GL_DEPTH);
	glClearDepth(1.0f);		// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To 
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations


    cb_game_mode(optArg);

    /*register callbacks here*/
    glutDisplayFunc(cb_display);
    glutReshapeFunc(cb_reshape);
    glutKeyboardFunc(cb_keyboard);
    glutKeyboardUpFunc( cb_keyboard_up);
    glutMouseFunc(cb_mouseclick);
    glutMotionFunc(cb_drag);
    glutPassiveMotionFunc(NULL);
    glutVisibilityFunc(NULL);
    glutEntryFunc(NULL);
    glutSpecialFunc(cb_special_key);
    glutSpecialUpFunc(cb_special_key_up);

    //Attach extra call backs if needed in the future
/*  glutOverlayDisplayFunc
    glutSpaceballMotionFunc
    glutSpaceballRotateFunc
    glutSpaceballButtonFunc
    glutButtonBoxFunc
    glutDialsFunc
    glutTabletMotionFunc
    glutTabletButtonFunc
    glutMenuStatusFunc
    glutIdleFunc
    glutTimerFunc 
*/


    //pass control to glut
    cb_reshape(w,h);
    glutMainLoop ();


    return 0; //we should never reach here

}
