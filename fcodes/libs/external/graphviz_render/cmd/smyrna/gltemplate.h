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

/*
	this code is used to set up a OpenGL window and set
	some basic features (panning zooming and rotating)
	Viewport.h provides a higher level control such as drawing primitives
*/

#ifdef _WIN32
#include <windows.h>
#endif
#include <glcomp/opengl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkgl.h>
#include <gdk/gdkcursor.h>

    gboolean expose_event(GtkWidget * widget, GdkEventExpose * event,
			  gpointer data);
    extern GdkGLConfig *configure_gl(void);
    void create_window(GdkGLConfig * glconfig, GtkWidget * vbox);
