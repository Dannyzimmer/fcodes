/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <glcomp/glutils.h>
#include <stdlib.h>
#include <string.h>
#include <glcomp/glcompdefs.h>

/*transforms 2d windows location to 3d gl coords but depth is calculated unlike the previous function*/
int GetOGLPosRef(int x, int y, float *X, float *Y) {
    GLdouble wwinX;
    GLdouble wwinY;
    GLdouble wwinZ;
    GLdouble posX, posY, posZ;

    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLfloat winX, winY;
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    //draw a point  to a not important location to get window coordinates
    glColor4f(0.0f, 0.0f, 0.0f, 0.001f);

    glBegin(GL_POINTS);
    glVertex3f(-100.0f, -100.0f, 0.0f);
    glEnd();
    gluProject(-100.0, -100.0, 0.00, modelview, projection, viewport,
	       &wwinX, &wwinY, &wwinZ);
    winX = (float) x;
    winY = (float) viewport[3] - (float) y;
    gluUnProject(winX, winY, wwinZ, modelview, projection, viewport, &posX,
		 &posY, &posZ);

    *X = (float) posX;
    *Y = (float) posY;
    (void)posZ; // ignore Z that the caller does not need

    return 1;
}

float GetOGLDistance(int l)
{
    int x, y;
    GLdouble wwinX;
    GLdouble wwinY;
    GLdouble wwinZ;
    GLdouble posX, posY, posZ;
    GLdouble posXX, posYY, posZZ;

    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLfloat winX, winY;

    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    //draw a point  to a not important location to get window coordinates
    glColor4f(0.0f, 0.0f, 0.0f, 0.001f);

    glBegin(GL_POINTS);
    glVertex3f(10.0f, 10.0f, 1.0f);
    glEnd();
    gluProject(10.0, 10.0, 1.00, modelview, projection, viewport, &wwinX,
	       &wwinY, &wwinZ);
    x = 50;
    y = 50;
    winX = (float) x;
    winY = (float) viewport[3] - (float) y;
    gluUnProject(winX, winY, wwinZ, modelview, projection, viewport, &posX,
		 &posY, &posZ);
    x = x + l;
    y = 50;
    winX = (float) x;
    winY = (float) viewport[3] - (float) y;
    gluUnProject(winX, winY, wwinZ, modelview, projection, viewport,
		 &posXX, &posYY, &posZZ);
    return ((float) (posXX - posX));
}

/*
	functions def: returns opengl coordinates of firt hit object by using screen coordinates
	x,y; 2D screen coordiantes (usually received from mouse events
	X,Y,Z; pointers to coordinates values to be calculated
	return value: no return value


*/

void to3D(int x, int y, GLfloat * X, GLfloat * Y, GLfloat * Z)
{
    int const WIDTH = 20;

    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLfloat winX, winY;
    GLfloat winZ[400];
    GLdouble posX, posY, posZ;
    int idx;

    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    winX = (float) x;
    winY = (float) viewport[3] - (float) y;

    glReadPixels(x - WIDTH / 2, (int) winY - WIDTH / 2, WIDTH, WIDTH,
		 GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);
    float comp = -9999999;
    for (idx = 0; idx < WIDTH * WIDTH; idx++) {
	if (winZ[idx] > comp && winZ[idx] < 1)
	    comp = winZ[idx];
    }

    gluUnProject(winX, winY, comp, modelview, projection, viewport, &posX,
		 &posY, &posZ);

    *X = (GLfloat) posX;
    *Y = (GLfloat) posY;
    *Z = (GLfloat) posZ;
    return;
}

#include <math.h>

static glCompPoint sub(glCompPoint p, glCompPoint q)
{
    p.x -= q.x;
    p.y -= q.y;
    p.z -= q.z;
    return p;
}

static double dot(glCompPoint p, glCompPoint q)
{
    return p.x * q.x + p.y * q.y + p.z * q.z;
}

static double len(glCompPoint p)
{
    return sqrt(dot(p, p));
}

static glCompPoint blend(glCompPoint p, glCompPoint q, float m)
{
    glCompPoint r;

    r.x = p.x + m * (q.x - p.x);
    r.y = p.y + m * (q.y - p.y);
    r.z = p.z + m * (q.z - p.z);
    return r;
}

static double dist(glCompPoint p, glCompPoint q)
{
    return len(sub(p, q));
}

/*
 * Given a line segment determined by two points a and b, and a 3rd point p,
 * return the distance between the point and the segment.
 * If the perpendicular from p to the line a-b is outside of the segment,
 * return the distance to the closer of a or b.
 */
double point_to_lineseg_dist(glCompPoint p, glCompPoint a, glCompPoint b)
{
    float U;
    glCompPoint q;
    glCompPoint ba = sub(b, a);
    glCompPoint pa = sub(p, a);

    U = (float) (dot(pa, ba) / dot(ba, ba));

    if (U > 1)
	q = b;
    else if (U < 0)
	q = a;
    else
	q = blend(a, b, U);

    return dist(p, q);
}

void glCompCalcWidget(glCompCommon * parent, glCompCommon * child,
		      glCompCommon * ref)
{
    /*check alignments first , alignments overrides anchors */
    GLfloat borderWidth;
    ref->height = child->height;
    ref->width = child->width;
    if(!parent)
    {
	child->refPos.x = child->pos.x;
	child->refPos.y = child->pos.y;
	return;
    }
    borderWidth = parent->borderWidth;
    if (child->align != glAlignNone)	//if alignment, make sure width and height is no greater than parent
    {
	if (child->width > parent->width)
	    ref->width = parent->width - 2.0f *borderWidth;
	if (child->height > parent->height)
	    ref->height = parent->height - 2.0f *borderWidth;;

    }

    ref->pos.x = parent->refPos.x + ref->pos.x + borderWidth;
    ref->pos.y = parent->refPos.y + ref->pos.y + borderWidth;


    switch (child->align) {
    case glAlignLeft:
	ref->pos.x = parent->refPos.x + borderWidth;
	ref->pos.y = parent->refPos.y + borderWidth;
	ref->height = parent->height - 2.0f * borderWidth;
	break;
    case glAlignRight:
	ref->pos.x =
	    parent->refPos.x + parent->width - child->width - borderWidth;
	ref->pos.y = parent->refPos.y + borderWidth;
	ref->height = parent->height - 2.0f * borderWidth;
	break;

    case glAlignTop:
	ref->pos.y =
	    parent->refPos.y + parent->height - child->height -
	    borderWidth;
	ref->pos.x = parent->refPos.x;
	ref->width = parent->width - 2.0f * borderWidth;
	break;

    case glAlignBottom:
	ref->pos.y = parent->refPos.y + borderWidth;
	ref->pos.x = parent->refPos.x + borderWidth;
	ref->width = parent->width - 2.0f * borderWidth;
	break;
    case glAlignParent:
	ref->pos.y = parent->refPos.y + borderWidth;
	ref->pos.x = parent->refPos.x + borderWidth;;
	ref->width = parent->width - 2.0f * borderWidth;;
	ref->height = parent->height - 2.0f * borderWidth;
	break;
    case glAlignCenter:
    case glAlignNone:
	break;
    }
    if (child->align == glAlignNone)	// No alignment , chekc anchors
    {
	ref->pos.x = parent->refPos.x + child->pos.x + borderWidth;
	ref->pos.y = parent->refPos.y + child->pos.y + borderWidth;

	if (child->anchor.leftAnchor)
	    ref->pos.x =
		parent->refPos.x + child->anchor.left + borderWidth;
	if (child->anchor.bottomAnchor)
	    ref->pos.y =
		parent->refPos.y + child->anchor.bottom + borderWidth;

	if (child->anchor.topAnchor)
	    ref->height =
		parent->refPos.y + parent->height - ref->pos.y -
		child->anchor.top - borderWidth;
	if (child->anchor.rightAnchor)
	    ref->width =
		parent->refPos.x + parent->width - ref->pos.x -
		child->anchor.right - borderWidth;
    }
    child->refPos.x = ref->pos.x;
    child->refPos.y = ref->pos.y;
    child->width = ref->width;
    child->height = ref->height;
}

static void glCompQuadVertex(glCompPoint * p0, glCompPoint * p1,
			     glCompPoint * p2, glCompPoint * p3)
{
    glVertex3f(p0->x, p0->y, p0->z);
    glVertex3f(p1->x, p1->y, p1->z);
    glVertex3f(p2->x, p2->y, p2->z);
    glVertex3f(p3->x, p3->y, p3->z);
}

void glCompSetColor(glCompColor * c)
{
    glColor4f(c->R, c->G, c->B, c->A);
}

void glCompDrawRectangle(glCompRect * r)
{
    glBegin(GL_QUADS);
    glVertex3f(r->pos.x, r->pos.y, r->pos.z);
    glVertex3f(r->pos.x + r->w, r->pos.y, r->pos.z);
    glVertex3f(r->pos.x + r->w, r->pos.y + r->h, r->pos.z);
    glVertex3f(r->pos.x, r->pos.y + r->h, r->pos.z);
    glEnd();
}
void glCompDrawRectPrism(glCompPoint * p, GLfloat w, GLfloat h, GLfloat b,
			 GLfloat d, glCompColor * c, int bumped)
{
    GLfloat color_fac;
    glCompPoint A, B, C, D, E, F, G, H;
    GLfloat dim = 1.00;
    if (!bumped) {
	color_fac = 1.3f;
	b = b - 2;
	dim = 0.5;
    } else
	color_fac = 1.0f / 1.3f;


    A.x = p->x;
    A.y = p->y;
    A.z = p->z;
    B.x = p->x + w;
    B.y = p->y;
    B.z = p->z;
    C.x = p->x + w;
    C.y = p->y + h;
    C.z = p->z;
    D.x = p->x;
    D.y = p->y + h;
    D.z = p->z;
    G.x = p->x + b;
    G.y = p->y + b;
    G.z = p->z + d;
    H.x = p->x + w - b;
    H.y = p->y + b;
    H.z = p->z + d;
    E.x = p->x + b;
    E.y = p->y + h - b;
    E.z = p->z + d;
    F.x = p->x + w - b;
    F.y = p->y + h - b;
    F.z = p->z + d;
    glBegin(GL_QUADS);
    glColor4f(c->R * dim, c->G * dim, c->B * dim, c->A);
    glCompQuadVertex(&G, &H, &F, &E);

    glColor4f(c->R * color_fac * dim, c->G * color_fac * dim,
	      c->B * color_fac * dim, c->A);
    glCompQuadVertex(&A, &B, &H, &G);
    glCompQuadVertex(&B, &H, &F, &C);

    glColor4f(c->R / color_fac * dim, c->G / color_fac * dim,
	      c->B / color_fac * dim, c->A);
    glCompQuadVertex(&A, &G, &E, &D);
    glCompQuadVertex(&E, &F, &C, &D);
    glEnd();

}

GLfloat distBetweenPts(glCompPoint A,glCompPoint B,float R)
{
    GLfloat rv=0;	
    rv=(A.x-B.x)*(A.x-B.x) + (A.y-B.y)*(A.y-B.y) +(A.z-B.z)*(A.z-B.z);
    rv=sqrtf(rv);
    if (rv <=R)
	return 0;
    return rv;
}

int is_point_in_rectangle(float X, float Y, float RX, float RY, float RW,float RH)
{
    if (X >= RX && X <= RX + RW && Y >= RY && Y <= RY + RH)
	return 1;
    else
	return 0;
}

/**
 * @dir lib/glcomp
 * @brief OpenGL GUI for cmd/smyrna
 */
