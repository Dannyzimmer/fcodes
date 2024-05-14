/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include "menucallbacks.h"
#include "viewport.h"
#include "tvnodes.h"

#include "gvprpipe.h"
#include "topviewsettings.h"
#include "gltemplate.h"
#include <common/const.h>
#include <cgraph/agxbuf.h>
#include <assert.h>
#include <ctype.h>
#include <glib.h>
#include <stdlib.h>
#include  "frmobjectui.h"

void mAttributesSlot(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    showAttrsWidget();
}

void mOpenSlot(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    GtkWidget *dialog;
    GtkFileFilter *filter;

    filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.gv");
    gtk_file_filter_add_pattern(filter, "*.dot");
    dialog = gtk_file_chooser_dialog_new("Open File",
					 NULL,
					 GTK_FILE_CHOOSER_ACTION_OPEN,
					 GTK_STOCK_CANCEL,
					 GTK_RESPONSE_CANCEL,
					 GTK_STOCK_OPEN,
					 GTK_RESPONSE_ACCEPT, NULL);

    gtk_file_chooser_set_filter((GtkFileChooser *) dialog, filter);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
	char *filename;
	filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

	add_graph_to_viewport_from_file(filename);
	g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

void mSaveSlot(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    save_graph();		//save without prompt
}

void mSaveAsSlot(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    save_as_graph();		//save with prompt
}

void mCloseSlot(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    if (view->activeGraph == 0)
	close_graph(view);
}

void mOptionsSlot(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;
}

void mQuitSlot(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    close_graph(view);
    gtk_main_quit();
}

//edit
void mCutSlot(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;
}

void mCopySlot(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;
}

void mPasteSlot(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;
}

void mDeleteSlot(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;
}
void mTopviewSettingsSlot(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;
    show_settings_form();
}

//view
void mShowToolBoxSlot(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    if (!gtk_widget_set_gl_capability
	(glade_xml_get_widget(xml, "glfixed"), configure_gl(),
	 gtk_widget_get_gl_context(view->drawing_area), 0, 0))
	printf("glwidget creation failed \n");
}

void mShowConsoleSlot(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    static int state = 0;  // off by default

    if (state) {
	gtk_widget_hide (glade_xml_get_widget(xml, "vbox13"));
	gtk_widget_show (glade_xml_get_widget(xml, "show_console1"));
	gtk_widget_hide (glade_xml_get_widget(xml, "hide_console1")); 
	state = 0;
    }
    else {
	gtk_widget_show (glade_xml_get_widget(xml, "vbox13"));
	gtk_widget_hide (glade_xml_get_widget(xml, "show_console1"));
	gtk_widget_show (glade_xml_get_widget(xml, "hide_console1")); 
	state = 1;
    }
}

//Graph
void mNodeListSlot(GtkWidget * widget, gpointer user_data)
{
	(void)widget;
	(void)user_data;

	gtk_widget_show(glade_xml_get_widget(xml, "frmTVNodes"));
	setup_tree (view->g[view->activeGraph]);
}

void mNewNodeSlot(GtkWidget * widget, gpointer user_data)
{
  (void)widget;
  (void)user_data;
}

void mNewEdgeSlot(GtkWidget * widget, gpointer user_data)
{
  (void)widget;
  (void)user_data;
}

void mNewClusterSlot(GtkWidget * widget, gpointer user_data)
{
  (void)widget;
  (void)user_data;
}

void mGraphPropertiesSlot(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    //there has to be an active graph to open the graph prop page
    if (view->activeGraph > -1) {
	load_graph_properties();	//load from graph to gui
	gtk_dialog_set_response_sensitive((GtkDialog *)
					  glade_xml_get_widget(xml,
							       "dlgOpenGraph"),
					  1, 1);
	gtk_dialog_set_response_sensitive((GtkDialog *)
					  glade_xml_get_widget(xml,
							       "dlgOpenGraph"),
					  2, 1);
	gtk_dialog_run((GtkDialog *)glade_xml_get_widget(xml, "dlgOpenGraph"));
	//need to hide the dialog , again freaking GTK!!!!
	gtk_widget_hide(glade_xml_get_widget(xml, "dlgOpenGraph"));
    }
}

void mNodeFindSlot(GtkWidget * widget, gpointer user_data)
{
  (void)widget;
  (void)user_data;
}

static void mPropertiesSlot(void) {
    if (view->activeGraph >= 0)
	gtk_widget_hide(glade_xml_get_widget(xml, "frmObject"));
    gtk_widget_show(glade_xml_get_widget(xml, "frmObject"));
}

void mClusterPropertiesSlot(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    mPropertiesSlot();
}

void mNodePropertiesSlot(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    mPropertiesSlot();
}

void mEdgePropertiesSlot(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    mPropertiesSlot();
}

void mShowCodeSlot(GtkWidget * widget, gpointer user_data)
{
  (void)widget;
  (void)user_data;
}

void mAbout(GtkWidget * widget, gpointer user_data)
{
  (void)widget;
  (void)user_data;
}

void change_cursor(GdkCursorType C)
{
    GdkCursor *cursor;
    cursor = gdk_cursor_new(C);
    gdk_window_set_cursor((GdkWindow *) view->drawing_area->window,
			  cursor);
    gdk_cursor_destroy(cursor);
}

void mTestgvpr(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    char *bf2;
    GtkTextBuffer *gtkbuf;
    GtkTextIter startit;
    GtkTextIter endit;
    const char *args;
    int cloneGraph;

    args =
	gtk_entry_get_text((GtkEntry *)
			   glade_xml_get_widget(xml, "gvprargs"));
    gtkbuf =
	gtk_text_view_get_buffer((GtkTextView *)
				 glade_xml_get_widget(xml,
						      "gvprtextinput"));
    gtk_text_buffer_get_start_iter(gtkbuf, &startit);
    gtk_text_buffer_get_end_iter(gtkbuf, &endit);
    bf2 = gtk_text_buffer_get_text(gtkbuf, &startit, &endit, 0);

    if (*args == '\0' && *bf2 == '\0') {
	g_free(bf2);
	return;
    }

    size_t argc = 1;
    if (*args != '\0')
	argc += 2;
    if (*bf2 != '\0')
	argc++;
    if (gtk_toggle_button_get_active
	((GtkToggleButton *) glade_xml_get_widget(xml, "gvprapplycb"))) {
	cloneGraph = 1;
	argc++;
    } else
	cloneGraph = 0;
    assert(argc <= 5);
    char *argv[6] = {0};
    size_t j = 0;
    argv[j++] = "smyrna";
    if (cloneGraph)
	argv[j++] = "-C";
    if (*args != '\0') {
	argv[j++] = "-a";
// Suppress Clang/GCC -Wcast-qual warning. Casting away const here is acceptable
// as `run_gvpr` does not modify input arguments.
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif
	argv[j++] = (char*)args;
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
    }
    if (*bf2 != '\0') {
	argv[j++] = bf2;
    }
    assert(j == argc);

    run_gvpr(view->g[view->activeGraph], argc, argv);
    g_free(bf2);
}

/*
   opens a file open dialog and load a gvpr program to gvpr script text box
   if the current script is modified, user should be informed about it
*/
void on_gvprbuttonload_clicked(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    FILE *input_file = NULL;
    char *str;
    agxbuf xbuf = {0};
    GtkTextBuffer *gtkbuf;	/*GTK buffer from glade GUI */

    char buf[BUFSIZ];

    char *filename = NULL;
    if (openfiledlg(&filename)) {
	input_file = fopen(filename, "r");
	g_free(filename);
	if (input_file) {
	    while (fgets(buf, BUFSIZ, input_file))
		agxbput(&xbuf, buf);
	    gtkbuf =
		gtk_text_view_get_buffer((GtkTextView *)
					 glade_xml_get_widget(xml,
							      "gvprtextinput"));
	    str = agxbuse(&xbuf);
	    if (g_utf8_validate(str, -1, NULL)) {
		gtk_text_buffer_set_text(gtkbuf, str, -1);
	    } else {
		show_gui_warning("File format is not UTF8!");
	    }
	    fclose(input_file);
	} else {
	    show_gui_warning("file couldn't be opened\n");
	}
    }
    agxbfree(&xbuf);
}

/*
	opens a file save dialog and save a gvpr program from gvpr script text box
	if the current script is modified, user should be informed about it
*/
void on_gvprbuttonsave_clicked(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    FILE *output_file = NULL;
    GtkTextBuffer *gtkbuf;	/*GTK buffer from glade GUI */
    char *bf2;
    GtkTextIter startit;
    GtkTextIter endit;

    char *filename = NULL;
    if (savefiledlg(&filename)) {
	output_file = fopen(filename, "w");
	g_free(filename);
	if (output_file) {
	    gtkbuf =
		gtk_text_view_get_buffer((GtkTextView *)
					 glade_xml_get_widget(xml,
							      "gvprtextinput"));
	    gtk_text_buffer_get_start_iter(gtkbuf, &startit);
	    gtk_text_buffer_get_end_iter(gtkbuf, &endit);
	    bf2 = gtk_text_buffer_get_text(gtkbuf, &startit, &endit, 0);
	    fputs(bf2, output_file);
	    free(bf2);
	    fclose(output_file);
	}

	/*Code has not been completed for this function yet */
    }
}
