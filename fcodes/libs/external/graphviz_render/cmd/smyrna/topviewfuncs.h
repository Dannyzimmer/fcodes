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

extern void pick_object_xyz(Agraph_t* g,topview* t,GLfloat x,GLfloat y,GLfloat z) ;
extern void initSmGraph(Agraph_t * g,topview* rv);
extern void updateSmGraph(Agraph_t * g,topview* t);
extern void renderSmGraph(topview* t);
extern void cacheSelectedEdges(Agraph_t * g,topview* t);
extern void cacheSelectedNodes(Agraph_t * g,topview* t);
