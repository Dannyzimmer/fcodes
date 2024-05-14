/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include "polytess.h"
#include <cgraph/alloc.h>
#include <stddef.h>
#include <xdot/xdot.h>
tessPoly TP;

#ifndef _WIN32
#define CALLBACK 
#endif
static void CALLBACK combineCallback(GLdouble coords[3], GLdouble *vertex_data[4],GLfloat weight[4], GLdouble **dataOut)
{
    (void)vertex_data;
    (void)weight;

    int i;
    GLdouble *vertex = gv_calloc(6, sizeof(GLdouble));
    vertex[0] = coords[0];
    vertex[1] = coords[1];
    vertex[2] = coords[2];
    for (i = 3; i < 6; i++)
    {
	vertex[i] = 0;

    }
    *dataOut = vertex;
}

static void CALLBACK vertexCallback(GLvoid *vertex)
{
    GLdouble *ptr = vertex;
    glVertex3dv(ptr);

}

// OpenGL’s `gluTessCallback` function has a prototype indicating it takes a
// `void(*)(void)`. But its documentation describes passing in various function
// pointers with differing calling conventions. To use this API while also
// pacifying all the various build environments, we need this rather silly
// wrapper.
#ifdef _MSC_VER
// MSVC is of the (correct) opinion that casting between function pointers of
// incompatible calling conventions is unacceptable behavior…
#define MAKE_GLU_CALLBACK(f) f
#else
// …nevertheless other compilers insist we cast or they believe we have made a
// typo
#define MAKE_GLU_CALLBACK(f) ((void (*)(void))(f))
#endif

static GLUtesselator* Init(void)
{
    // Create a new tessellation object 
    GLUtesselator* tobj = gluNewTess(); 
    // Set callback functions
    gluTessCallback(tobj, GLU_TESS_VERTEX, MAKE_GLU_CALLBACK(vertexCallback));
    gluTessCallback(tobj, GLU_TESS_BEGIN, MAKE_GLU_CALLBACK(glBegin));
    gluTessCallback(tobj, GLU_TESS_END, MAKE_GLU_CALLBACK(glEnd));
    gluTessCallback(tobj, GLU_TESS_COMBINE, MAKE_GLU_CALLBACK(combineCallback));
    return tobj;
}

static void Set_Winding_Rule(GLUtesselator *tobj, GLenum winding_rule)
{
// Set the winding rule
    gluTessProperty(tobj, GLU_TESS_WINDING_RULE, winding_rule); 
}

static void Render_Contour2(GLUtesselator *tobj, sdot_op* p)
{
    GLdouble *d = gv_calloc(p->op.u.polygon.cnt * 3, sizeof(GLdouble));
    for (size_t x = 0; x < p->op.u.polygon.cnt; x++)
    {
        d[x * 3] = p->op.u.polygon.pts[x].x;
        d[x * 3 + 1] = p->op.u.polygon.pts[x].y;
        d[x * 3 + 2] = p->op.u.polygon.pts[x].z + view->Topview->global_z;
    }
    for (size_t x = 0; x < p->op.u.polygon.cnt; x++) //loop through the vertices
    {
        gluTessVertex(tobj, &d[x * 3], &d[x * 3]); //store the vertex
    }
}

static void Begin_Polygon(GLUtesselator *tobj)
{
    gluTessBeginPolygon(tobj, NULL);
}
static void End_Polygon(GLUtesselator *tobj)
{
    gluTessEndPolygon(tobj);
}
static void Begin_Contour(GLUtesselator *tobj)
{
    gluTessBeginContour(tobj);
}
static void End_Contour(GLUtesselator *tobj)
{
    gluTessEndContour(tobj);
}

void drawTessPolygon(sdot_op* p)
{
    if (!TP.tobj)
    {
	TP.tobj=Init();
	TP.windingRule=GLU_TESS_WINDING_ODD;
    }
    Set_Winding_Rule(TP.tobj,TP.windingRule);
    Begin_Polygon(TP.tobj); 
    Begin_Contour(TP.tobj);
    Render_Contour2(TP.tobj,p); 
    End_Contour(TP.tobj);
    End_Polygon(TP.tobj);
}
