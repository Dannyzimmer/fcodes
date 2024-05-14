/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include "config.h"

#include <gvc/gvplugin.h>

#if defined(GVDLL)
#define IMPORT	__declspec(dllimport)
#else
#define IMPORT /* nothing */
#endif

extern "C" {

IMPORT extern gvplugin_library_t gvplugin_dot_layout_LTX_library;
IMPORT extern gvplugin_library_t gvplugin_neato_layout_LTX_library;
#ifdef HAVE_LIBGD
IMPORT extern gvplugin_library_t gvplugin_gd_LTX_library;
#endif
#ifdef HAVE_PANGOCAIRO
IMPORT extern gvplugin_library_t gvplugin_pango_LTX_library;
#ifdef HAVE_WEBP
IMPORT extern gvplugin_library_t gvplugin_webp_LTX_library;
#endif
#endif
IMPORT extern gvplugin_library_t gvplugin_core_LTX_library;


lt_symlist_t lt_preloaded_symbols[] = {
	{ "gvplugin_dot_layout_LTX_library", &gvplugin_dot_layout_LTX_library },
	{ "gvplugin_neato_layout_LTX_library", &gvplugin_neato_layout_LTX_library },
#ifdef HAVE_PANGOCAIRO
	{ "gvplugin_pango_LTX_library", &gvplugin_pango_LTX_library },
#ifdef HAVE_WEBP
	{ "gvplugin_webp_LTX_library", &gvplugin_webp_LTX_library },
#endif
#endif
#ifdef HAVE_LIBGD
	{ "gvplugin_gd_LTX_library", &gvplugin_gd_LTX_library },
#endif
	{ "gvplugin_core_LTX_library", &gvplugin_core_LTX_library },
	{ 0, 0 }
};

}
