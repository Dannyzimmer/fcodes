/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include "datalistcallbacks.h"
#include "tvnodes.h"

void btnTVOK_clicked_cb(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    gtk_widget_hide(glade_xml_get_widget(xml, "frmTVNodes"));
}

void on_TVNodes_close (GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    gtk_widget_hide(glade_xml_get_widget(xml, "frmTVNodes"));
}

void btnTVShowAll_clicked_cb(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    tv_show_all();
}

void btnTVHideAll_clicked_cb(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    tv_hide_all();
}

void btnTVSaveAs_clicked_cb(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    tv_save_as(0);
}

void btnTVSaveWith_clicked_cb(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    tv_save_as(1);
}
