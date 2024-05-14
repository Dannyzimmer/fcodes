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
#include <cgraph/cgraph.h>

extern int l_int(void *obj, Agsym_t * attr, int def);
extern float l_float(void *obj, Agsym_t * attr, float def);
extern int getAttrBool(Agraph_t* g,void* obj,char* attr_name,int def);
extern int getAttrInt(Agraph_t* g,void* obj,char* attr_name,int def);
extern float getAttrFloat(Agraph_t* g,void* obj,char* attr_name,float def);
extern char* getAttrStr(Agraph_t* g,void* obj,char* attr_name,char* def);
extern void setColor(glCompColor* c,GLfloat R,GLfloat G,GLfloat B,GLfloat A);
extern void getcolorfromschema(colorschemaset * sc, float l, float maxl,glCompColor * c);
extern glCompPoint getPointFromStr(const char *str);
extern int point_in_polygon(glCompPoly* selPoly,glCompPoint p);
