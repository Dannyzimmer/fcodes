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

#include "gui.h"
#include "tvnodes.h"

    _BB void on_TVNodes_close (GtkWidget * widget, gpointer user_data);
    _BB void btnTVOK_clicked_cb(GtkWidget * widget, gpointer user_data);
    _BB void btnTVShowAll_clicked_cb(GtkWidget * widget,
				     gpointer user_data);
    _BB void btnTVHideAll_clicked_cb(GtkWidget * widget,
				     gpointer user_data);

    _BB void btnTVSaveAs_clicked_cb(GtkWidget * widget,
				    gpointer user_data);
    _BB void btnTVSaveWith_clicked_cb(GtkWidget * widget, gpointer user_data);
