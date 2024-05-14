/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include "topviewfuncs.h"
#include "topviewsettings.h"
#include "gui.h"
#include <cgraph/startswith.h>
#include <common/colorprocs.h>
#include "viewport.h"

void color_change_request(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    view->refresh.color=1;
}
void size_change_request(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;
}

void on_settingsOKBtn_clicked(GtkWidget * widget, gpointer user_data)
{
    on_settingsApplyBtn_clicked(widget, user_data);
    gtk_widget_hide(glade_xml_get_widget(xml, "dlgSettings"));
}

void on_settingsApplyBtn_clicked(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    update_graph_from_settings(view->g[view->activeGraph]);
    set_viewport_settings_from_template(view, view->g[view->activeGraph]);
    updateSmGraph(view->g[view->activeGraph],view->Topview);
}
void on_dlgSettings_close (GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    gtk_widget_hide(glade_xml_get_widget(xml, "dlgSettings"));
}

void on_settingsCancelBtn_clicked(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    gtk_widget_hide(glade_xml_get_widget(xml, "dlgSettings"));
}
static void copy_attr(Agraph_t *destG, char *attrib, Agraph_t *g)
{
    agattr(g, AGRAPH, attrib, agget(destG, attrib));
}


static void set_color_button_widget(char *attrib, char *widget_name) {
    GdkColor color;
    gvcolor_t cl;

    char *buf;
    buf = agget(view->g[view->activeGraph], attrib);
    if ((!buf) || (strcmp(buf, "") == 0))
    {
	buf = agget(view->systemGraphs.def_attrs, attrib);
	copy_attr(view->systemGraphs.def_attrs, attrib, view->g[view->activeGraph]);
    }
    if (buf) {
	colorxlate(buf, &cl, RGBA_DOUBLE);
	color.red = (guint16)(cl.u.RGBA[0] * 65535.0);
	color.green = (guint16)(cl.u.RGBA[1] * 65535.0);
	color.blue = (guint16)(cl.u.RGBA[2] * 65535.0);
	gtk_color_button_set_color((GtkColorButton *)
				   glade_xml_get_widget(xml, widget_name),
				   &color);

    }
}

static void get_color_button_widget_to_attribute(char *attrib,
						char *widget_name,
						Agraph_t * g)
{
    GdkColor color;
    char buf[256];

    gtk_color_button_get_color((GtkColorButton *)
			       glade_xml_get_widget(xml, widget_name),
			       &color);
    snprintf(buf, sizeof(buf), "#%02x%02x%02x",
	    (int) ((float) color.red / 65535.0 * 255.0),
	    (int) ((float) color.green / 65535.0 * 255.0),
	    (int) ((float) color.blue / 65535.0 * 255.0));
    agattr(g, AGRAPH, attrib, buf);
}

static void get_text_widget_to_attribute(char *attrib, char *widget_name,
					Agraph_t * g)
{
    if (strlen(attrib) > 512)
	return;
    agattr(g, AGRAPH, attrib, gtk_entry_get_text((GtkEntry*)
			       glade_xml_get_widget(xml, widget_name)));
}

static void set_text_widget(char *attrib, char *widget_name) {
    char *buf;

    buf = agget(view->g[view->activeGraph], attrib);
    if ((!buf) || (strcmp(buf, "") == 0))
    {
	buf = agget(view->systemGraphs.def_attrs, attrib);
	copy_attr(view->systemGraphs.def_attrs, attrib, view->g[view->activeGraph]);
    }

    if (buf) {
	gtk_entry_set_text((GtkEntry *)
			   glade_xml_get_widget(xml, widget_name), buf);
    }
}

static void set_checkbox_widget(char *attrib, char *widget_name) {
    char *buf;
    int value;

    buf = agget(view->g[view->activeGraph], attrib);
    if ((!buf) || (strcmp(buf, "") == 0))
    {
	buf = agget(view->systemGraphs.def_attrs, attrib);
	copy_attr(view->systemGraphs.def_attrs, attrib, view->g[view->activeGraph]);
    }


    if (buf) {
	value = atoi(buf);
	gtk_toggle_button_set_active((GtkToggleButton *)
				     glade_xml_get_widget(xml,
							  widget_name),
				     value);
    }
}

static void get_checkbox_widget_to_attribute(char *attrib, char *widget_name,
					    Agraph_t * g)
{
    int value;
    char buf[100];

    value = (int) gtk_toggle_button_get_active((GtkToggleButton *)
					       glade_xml_get_widget(xml,
								    widget_name));
    snprintf(buf, sizeof(buf), "%d", value);
    agattr(g, AGRAPH, attrib, buf);
}

static void set_spinbtn_widget(char *attrib, char *widget_name) {
    char *buf;
    float value;

    buf = agget(view->g[view->activeGraph], attrib);
    if ((!buf) || (strcmp(buf, "") == 0))
    {
	buf = agget(view->systemGraphs.def_attrs, attrib);
	copy_attr(view->systemGraphs.def_attrs, attrib, view->g[view->activeGraph]);
    }
    if (buf) {
	value = (float) atof(buf);
	gtk_spin_button_set_value((GtkSpinButton *)
				  glade_xml_get_widget(xml, widget_name),
				  value);
    }
}

static void get_spinbtn_widget_to_attribute(char *attrib,
					   char *widget_name, Agraph_t * g)
{
    float value;
    char buf[25];

    value = (float) gtk_spin_button_get_value((GtkSpinButton *)
					      glade_xml_get_widget(xml,
								   widget_name));
    snprintf(buf, sizeof(buf), "%f", value);
    agattr(g, AGRAPH, attrib, buf);
}
static void get_scalebtn_widget_to_attribute(char *attrib, char *widget_name,
					    Agraph_t * g)
{
    float value;
    char buf[25];

    value = (float) gtk_range_get_value((GtkRange *)
					glade_xml_get_widget(xml,
							     widget_name));
    snprintf(buf, sizeof(buf), "%f", value);
    agattr(g, AGRAPH, attrib, buf);
}

static void set_scalebtn_widget_to_attribute(char *attrib, char *widget_name) {
    char *buf;
    float value;
    buf = agget(view->g[view->activeGraph], attrib);

    if ((!buf) || (strcmp(buf, "") == 0))
    {
	buf = agget(view->systemGraphs.def_attrs, attrib);
	copy_attr(view->systemGraphs.def_attrs, attrib, view->g[view->activeGraph]);

    }
    if (buf) {
	value = (float) atof(buf);
	gtk_range_set_value((GtkRange *)
			    glade_xml_get_widget(xml, widget_name), value);
    }
}

static void set_combobox_widget(char *attrib, char *widget_name) {
    char *buf;
    int value;
    buf = agget(view->g[view->activeGraph], attrib);
    if ((!buf) || (strcmp(buf, "") == 0))
    {
	buf = agget(view->systemGraphs.def_attrs, attrib);
	copy_attr(view->systemGraphs.def_attrs, attrib, view->g[view->activeGraph]);
    }
    if (buf) {
	value = atoi(buf);
	gtk_combo_box_set_active((GtkComboBox *)
				 glade_xml_get_widget(xml, widget_name), value);
    }
}

static void get_combobox_widget_to_attribute(char *attrib, char *widget_name,
					    Agraph_t * g)
{
    char buf[25];
    int value;

    value = (int)
	gtk_combo_box_get_active((GtkComboBox *)
				 glade_xml_get_widget(xml, widget_name));

    snprintf(buf, sizeof(buf), "%d", value);
    agattr(g, AGRAPH, attrib, buf);
}

void load_settings_from_graph(void) {
  Agsym_t *sym = NULL;
  while ((sym = agnxtattr(view->systemGraphs.attrs_widgets, AGRAPH, sym))) {
    if (startswith(sym->name, "color_button"))
      set_color_button_widget(
          sym->name + strlen("color_button="),
          agget(view->systemGraphs.attrs_widgets, sym->name));
    if (startswith(sym->name, "check_box"))
      set_checkbox_widget(sym->name + strlen("check_box="),
                          agget(view->systemGraphs.attrs_widgets, sym->name));
    if (startswith(sym->name, "text_box"))
      set_text_widget(sym->name + strlen("text_box="),
                      agget(view->systemGraphs.attrs_widgets, sym->name));
    if (startswith(sym->name, "combobox"))
      set_combobox_widget(sym->name + strlen("combobox="),
                          agget(view->systemGraphs.attrs_widgets, sym->name));
    if (startswith(sym->name, "spin_button"))
      set_spinbtn_widget(sym->name + strlen("spin_button="),
                         agget(view->systemGraphs.attrs_widgets, sym->name));
    if (startswith(sym->name, "scale_button"))
      set_scalebtn_widget_to_attribute(
          sym->name + strlen("scale_button="),
          agget(view->systemGraphs.attrs_widgets, sym->name));
  }
}

void update_graph_from_settings(Agraph_t *g) {
  Agsym_t *sym = NULL;
  while ((sym = agnxtattr(view->systemGraphs.attrs_widgets, AGRAPH, sym))) {
    if (startswith(sym->name, "color_button"))
      get_color_button_widget_to_attribute(
          sym->name + strlen("color_button="),
          agget(view->systemGraphs.attrs_widgets, sym->name), g);
    if (startswith(sym->name, "check_box"))
      get_checkbox_widget_to_attribute(
          sym->name + strlen("check_box="),
          agget(view->systemGraphs.attrs_widgets, sym->name), g);
    if (startswith(sym->name, "text_box"))
      get_text_widget_to_attribute(
          sym->name + strlen("text_box="),
          agget(view->systemGraphs.attrs_widgets, sym->name), g);
    if (startswith(sym->name, "combobox"))
      get_combobox_widget_to_attribute(
          sym->name + strlen("combobox="),
          agget(view->systemGraphs.attrs_widgets, sym->name), g);
    if (startswith(sym->name, "spin_button"))
      get_spinbtn_widget_to_attribute(
          sym->name + strlen("spin_button="),
          agget(view->systemGraphs.attrs_widgets, sym->name), g);
    if (startswith(sym->name, "scale_button"))
      get_scalebtn_widget_to_attribute(
          sym->name + strlen("scale_button="),
          agget(view->systemGraphs.attrs_widgets, sym->name), g);
  }
}

void show_settings_form(void) {

    if (view->activeGraph >= 0) {
	load_settings_from_graph();
	gtk_widget_hide(glade_xml_get_widget(xml, "dlgSettings"));
	gtk_widget_show(glade_xml_get_widget(xml, "dlgSettings"));
	gtk_window_set_keep_above((GtkWindow *)
				  glade_xml_get_widget(xml, "dlgSettings"),
				  1);
    } else {
	void *dlg = gtk_message_dialog_new(NULL,
							  GTK_DIALOG_MODAL,
							  GTK_MESSAGE_QUESTION,
							  GTK_BUTTONS_OK,
							  "No active graph");
	gtk_dialog_run(dlg);
	gtk_widget_hide(dlg);
    }
}
