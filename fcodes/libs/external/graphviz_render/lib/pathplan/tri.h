/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif


#include <pathgeom.h>

#ifdef GVDLL
#ifdef PATHPLAN_EXPORTS
#define TRI_API __declspec(dllexport)
#else
#define TRI_API __declspec(dllimport)
#endif
#endif

#ifndef TRI_API
#define TRI_API /* nothing */
#endif

/* Points in polygon must be in CCW order */
    TRI_API int Ptriangulate(Ppoly_t * polygon,
		      void (*fn) (void *closure, Ppoint_t tri[]),
		      void *vc);

#undef TRI_API

#ifdef __cplusplus
}
#endif
