/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <gvc/gvplugin.h>

extern gvplugin_installed_t gvrender_pango_types[];
extern gvplugin_installed_t gvtextlayout_pango_types[];
extern gvplugin_installed_t gvloadimage_pango_types[];
extern gvplugin_installed_t gvdevice_pango_types[];

static gvplugin_api_t apis[] = {
    {API_render, gvrender_pango_types},
    {API_textlayout, gvtextlayout_pango_types},
    {API_loadimage, gvloadimage_pango_types},
    {API_device, gvdevice_pango_types},
    {(api_t)0, 0},
};

#ifdef GVDLL
#define GVPLUGIN_PANGO_API __declspec(dllexport)
#else
#define GVPLUGIN_PANGO_API
#endif

GVPLUGIN_PANGO_API gvplugin_library_t gvplugin_pango_LTX_library = { "cairo", apis };
