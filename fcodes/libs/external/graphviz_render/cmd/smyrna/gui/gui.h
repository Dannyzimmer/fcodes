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
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkgl.h>
#include <glade/glade.h>
#include "callbacks.h"
#include <cgraph/cgraph.h>

    extern GladeXML *xml;	//global libglade vars
    extern GtkWidget *gladewidget;

    void load_graph_properties(void);	//load from graph to gui

//GTK helper functions
//void Color_Widget_bg (int r, int g, int b, GtkWidget *widget);        //change background color 
    void Color_Widget_bg(char *colorstring, GtkWidget * widget);
/*generic warning pop up*/
    void show_gui_warning(char *str);
/** generic open file dialog
 *
 * \param [out] filename Selected filename on success. Caller should \p g_free
 *   this.
 * \return Non-zero on success.
 */
    int openfiledlg(char **filename);
/** generic save file dialog
 *
 * \param [out] filename Selected filename on success. Caller should \p g_free
 *   this.
 * \return Non-zero on success.
 */
    int savefiledlg(char **filename);
    void append_textview(GtkTextView * textv, const char *s, size_t bytes);
