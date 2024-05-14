/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <stdbool.h>
#include <stdio.h>

#include <stdlib.h>
#include "gui.h"
#include <glade/glade.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include "viewport.h"
#include "frmobjectui.h"
#include <assert.h>
#include "gvprpipe.h"
#include <cgraph/agxbuf.h>
#include <cgraph/alloc.h>
#include <cgraph/strcasecmp.h>
#include <cgraph/strview.h>
#include <stdint.h>
#include <string.h>

static int sel_node;
static int sel_edge;
static int sel_graph;

static char *safestrdup(const char *src) {
    if (!src)
	return NULL;
    else
	return gv_strdup(src);
}
static int get_object_type(void)
{
    if (gtk_toggle_button_get_active
	((GtkToggleButton *) glade_xml_get_widget(xml, "attrRB0")))
	return AGRAPH;
    if (gtk_toggle_button_get_active
	((GtkToggleButton *) glade_xml_get_widget(xml, "attrRB1")))
	return AGNODE;
    if (gtk_toggle_button_get_active
	((GtkToggleButton *) glade_xml_get_widget(xml, "attrRB2")))
	return AGEDGE;
    return -1;
}

static attr_t *new_attr(void) {
  return gv_alloc(sizeof(attr_t));
}

static attr_t *new_attr_with_ref(Agsym_t * sym)
{
    attr_t *a = new_attr();
    a->name = safestrdup(sym->name);
    switch (sym->kind) {
    case AGRAPH:
	a->objType[0] = 1;
	a->defValG = safestrdup(sym->defval);
	break;
    case AGNODE:
	a->objType[1] = 1;
	a->defValN = safestrdup(sym->defval);
	break;
    case AGEDGE:
	a->objType[2] = 1;
	a->defValE = safestrdup(sym->defval);
	break;
    }
    return a;
}

static attr_t *new_attr_ref(attr_t * refAttr)
{
    attr_t *a = gv_alloc(sizeof(attr_t));
    *a = *refAttr;
    a->defValG = safestrdup(refAttr->defValG);
    a->defValN = safestrdup(refAttr->defValN);
    a->defValE = safestrdup(refAttr->defValE);
    a->name = safestrdup(refAttr->name);
    a->value = safestrdup(refAttr->value);
    return a;
}

static void reset_attr_list_widgets(attr_list * l)
{
    int id;
    for (id = 0; id < MAX_FILTERED_ATTR_COUNT; id++) {
	gtk_label_set_text(l->fLabels[id], "");
    }
}

// creates a new attr_list
// attr_list is a basic stack implementation
// with alphanumeric sorting functions
// that uses quicksort
static attr_list *attr_list_new(bool with_widgets) {
    int id;
    attr_list *l = gv_alloc(sizeof(attr_list));
    l->with_widgets = with_widgets;
    /*create filter widgets */

    if (with_widgets) {
	for (id = 0; id < MAX_FILTERED_ATTR_COUNT; id++) {
	    l->fLabels[id] = (GtkLabel *) gtk_label_new("");

	    gtk_widget_add_events((GtkWidget *) l->fLabels[id],
				  GDK_BUTTON_MOTION_MASK |
				  GDK_POINTER_MOTION_MASK |
				  GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS |
				  GDK_BUTTON_RELEASE_MASK |
				  GDK_SCROLL | GDK_VISIBILITY_NOTIFY_MASK);

	    gtk_widget_show((GtkWidget *) l->fLabels[id]);
	    Color_Widget_bg("blue", (GtkWidget *) l->fLabels[id]);
	    gtk_fixed_put((GtkFixed *) glade_xml_get_widget(xml, "fixed6"),
			  (GtkWidget *) l->fLabels[id], 10, 110 + id * 13);
	}
    }
    return l;
}

static int attr_compare(const void *a, const void *b)
{
    const attr_t *a1 = *(attr_t *const *) a;
    const attr_t *a2 = *(attr_t *const *) b;
    return strcasecmp(a1->name, a2->name);
}

static void attr_list_add(attr_list *l, attr_t *a) {
    if (!l || !a)
	return;
    attrs_append(&l->attributes, a);
    attrs_sort(&l->attributes, (int(*)(const attr_t**, const attr_t**))attr_compare);
    /*update indices */
    for (size_t id = 0; id < attrs_size(&l->attributes); ++id)
	attrs_get(&l->attributes, id)->index = id;
}

static attr_data_type get_attr_data_type(char c)
{
    switch (c) {
    case 'A':
	return attr_alpha;
	break;
    case 'F':
	return attr_float;
	break;
    case 'B':
	return attr_bool;
	break;
    case 'I':
	return attr_int;
	break;
    }
    return attr_alpha;
}

static void object_type_helper(strview_t a, int *t) {
    if (strview_str_eq(a, "GRAPH"))
	t[0] = 1;
    if (strview_str_eq(a, "CLUSTER"))
	t[0] = 1;
    if (strview_str_eq(a, "NODE"))
	t[1] = 1;
    if (strview_str_eq(a, "EDGE"))
	t[2] = 1;
    if (strview_str_eq(a, "ANY_ELEMENT")) {
	t[0] = 1;
	t[1] = 1;
	t[2] = 1;
    }
}

static void set_attr_object_type(const char *str, int *t) {
  strview_t a = strview(str, ' ');
  object_type_helper(a, t);
  while (true) {
    const char *start = a.data + a.size;
    if (strncmp(start, " or ", strlen(" or ")) != 0) {
      break;
    }
    start += strlen(" or ");
    const char *end = strstr(start, " or ");
    if (end == NULL) {
      a = strview(start, '\0');
    } else {
      a = (strview_t){.data = start, .size = (size_t)(end - start)};
    }
    object_type_helper(a, t);
  }
}

static attr_t *binarySearch(attr_list * l, char *searchKey)
{
  const attr_t key = {.name = searchKey};
  const attr_t *keyp = &key;
  attr_t **attrp = bsearch(&keyp, attrs_at(&l->attributes, 0),
                           attrs_size(&l->attributes), sizeof(attr_t*),
                           attr_compare);
  if (attrp != NULL) {
    return *attrp;
  }
  return NULL;
}

static attr_t *pBinarySearch(attr_list *l, const char *searchKey) {
    size_t low = 0;
    size_t high = attrs_size(&l->attributes) - 1;

    while (high != SIZE_MAX && low <= high) {
	size_t middle = (low + high) / 2;
	int res = strncasecmp(searchKey, attrs_get(&l->attributes, middle)->name, strlen(searchKey));
	if (res == 0) {
	    return attrs_get(&l->attributes, middle);
	}
	else if (res < 0) {
	    high = middle - 1;
	} else {
	    low = middle + 1;
	}
    }
    return NULL;
}

static void create_filtered_list(const char *prefix, attr_list *sl,
                                 attr_list *tl) {
    int res;
    attr_t *at;
    int objKind = get_object_type();

    if (strlen(prefix) == 0)
	return;
    /*locate first occurrence */
    at = pBinarySearch(sl, prefix);
    if (!at)
	return;

    res = 0;
    /*go backward to get the first */
    while (at->index > 0 && res == 0) {
	at = attrs_get(&sl->attributes, at->index - 1);
	res = strncasecmp(prefix, at->name, strlen(prefix));
    }
    res = 0;
    while (at->index < attrs_size(&sl->attributes) && res == 0) {
	at = attrs_get(&sl->attributes, at->index + 1);
	res = strncasecmp(prefix, at->name, strlen(prefix));
	if (res == 0 && at->objType[objKind] == 1)
	    attr_list_add(tl, new_attr_ref(at));
    }
}

static void filter_attributes(const char *prefix, topview *t) {
    int tmp;

    attr_list *l = t->attributes;
    int objKind = get_object_type();

    attr_list *fl = attr_list_new(false);
    reset_attr_list_widgets(l);
    create_filtered_list(prefix, l, fl);
    for (size_t ind = 0; ind < attrs_size(&fl->attributes); ++ind) {
	gtk_label_set_text(l->fLabels[ind], attrs_get(&fl->attributes, ind)->name);
    }

    Color_Widget_bg("white", glade_xml_get_widget(xml, "txtAttr"));
    if (attrs_is_empty(&fl->attributes))
	Color_Widget_bg("red", glade_xml_get_widget(xml, "txtAttr"));
    /*a new attribute can be entered */

    gtk_widget_show(glade_xml_get_widget(xml, "txtValue"));
    gtk_widget_show(glade_xml_get_widget(xml, "txtDefValue"));

    gtk_entry_set_text((GtkEntry *)
		       glade_xml_get_widget(xml, "txtDefValue"), "");
    gtk_entry_set_text((GtkEntry *) glade_xml_get_widget(xml, "txtValue"),
		       "");
    gtk_widget_set_sensitive(glade_xml_get_widget(xml, "txtDefValue"), 1);
    gtk_widget_show(glade_xml_get_widget(xml, "attrAddBtn"));
    gtk_widget_hide(glade_xml_get_widget(xml, "attrApplyBtn"));
    gtk_widget_hide(glade_xml_get_widget(xml, "attrApplyAllBtn"));
    gtk_widget_hide(glade_xml_get_widget(xml, "attrSearchBtn"));
    gtk_toggle_button_set_active((GtkToggleButton *)
				 glade_xml_get_widget(xml, "attrProg"), 0);

    if (strlen(prefix) == 0) {
	gtk_widget_hide(glade_xml_get_widget(xml, "attrAddBtn"));
	gtk_widget_hide(glade_xml_get_widget(xml, "attrApplyBtn"));
	gtk_widget_hide(glade_xml_get_widget(xml, "attrApplyAllBtn"));
	gtk_widget_hide(glade_xml_get_widget(xml, "attrSearchBtn"));
	gtk_widget_hide(glade_xml_get_widget(xml, "attrAddBtn"));
	gtk_widget_hide(glade_xml_get_widget(xml, "txtValue"));
	gtk_widget_hide(glade_xml_get_widget(xml, "txtDefValue"));
	Color_Widget_bg("white", glade_xml_get_widget(xml, "txtAttr"));
    }

    for (size_t ind = 0; ind < attrs_size(&fl->attributes); ++ind) {
	if (strcasecmp(prefix, attrs_get(&fl->attributes, ind)->name) == 0) { // an existing attribute

	    Color_Widget_bg("green", glade_xml_get_widget(xml, "txtAttr"));

	    if (get_object_type() == AGRAPH)
		gtk_entry_set_text((GtkEntry *)
				   glade_xml_get_widget(xml,
							"txtDefValue"),
				   attrs_get(&fl->attributes, 0)->defValG);
	    if (get_object_type() == AGNODE)
		gtk_entry_set_text((GtkEntry *)
				   glade_xml_get_widget(xml,
							"txtDefValue"),
				   attrs_get(&fl->attributes, 0)->defValN);
	    if (get_object_type() == AGEDGE)
		gtk_entry_set_text((GtkEntry *)
				   glade_xml_get_widget(xml,
							"txtDefValue"),
				   attrs_get(&fl->attributes, 0)->defValE);
	    gtk_widget_set_sensitive(glade_xml_get_widget
				     (xml, "txtDefValue"), 0);
	    gtk_widget_hide(glade_xml_get_widget(xml, "attrAddBtn"));
	    gtk_widget_show(glade_xml_get_widget(xml, "attrApplyBtn"));
	    gtk_widget_show(glade_xml_get_widget(xml, "attrApplyAllBtn"));
	    gtk_widget_show(glade_xml_get_widget(xml, "attrSearchBtn"));
	    gtk_toggle_button_set_active((GtkToggleButton *)
					 glade_xml_get_widget(xml,
							      "attrProg"),
					 attrs_get(&fl->attributes, 0)->propagate);
	    break;
	}
    }

    tmp = (objKind == AGNODE && sel_node)
       || (objKind == AGEDGE && sel_edge)
       || (objKind == AGRAPH && sel_graph);
    gtk_widget_set_sensitive(glade_xml_get_widget(xml, "attrApplyBtn"),
			     tmp);
}

// attribute text changed call back

_BB void on_txtAttr_changed(GtkWidget * widget, gpointer user_data)
{
  (void)user_data;
  filter_attributes(gtk_entry_get_text((GtkEntry*)widget), view->Topview);
}

static void set_refresh_filters(ViewInfo * v, char *name)
{
    if (strcasecmp(name, "pos") == 0)
	v->refresh.pos = 1;
    if (strcasecmp(name, "color") == 0)
	v->refresh.color = 1;
    if (strcasecmp(name, "selected") == 0)
	v->refresh.selection = 1;
}

static void doApply(void)
{
    char *attr_name;
    int prog;
    Agnode_t *v;
    Agedge_t *e;
    Agraph_t *g;
    int objKind;
    Agsym_t *sym;
    attr_t *attr;

    /*values to be applied to selected objects */
    attr_name =
	(char *) gtk_entry_get_text((GtkEntry *)
				    glade_xml_get_widget(xml, "txtAttr"));
    const char *def_val = gtk_entry_get_text((GtkEntry *)
				    glade_xml_get_widget(xml,
							 "txtDefValue"));
    const char *value = gtk_entry_get_text((GtkEntry *)
				    glade_xml_get_widget(xml, "txtValue"));
    prog =
	gtk_toggle_button_get_active((GtkToggleButton *)
				     glade_xml_get_widget(xml,
							  "attrProg"));
    g = view->g[view->activeGraph];
    objKind = get_object_type();
    attr = binarySearch(view->Topview->attributes, attr_name);
    assert(attr);
    attr->propagate = prog;
    sym = agattr(g, objKind, attr_name, NULL);
    if (!sym)			/*it shouldnt be null, just in case it is null */
	sym = agattr(g, objKind, attr_name, def_val);
    /*graph */
    if (objKind == AGRAPH)
	agset(g, attr_name, value);
    /*nodes */
    else if (objKind == AGNODE) {
	for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	    if (ND_selected(v))
		agxset(v, sym, value);
	}
    }
    /*edges */
    else if (objKind == AGEDGE) {
	for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	    for (e = agfstout(g, v); e; e = agnxtout(g, e)) {
		if (ED_selected(e))
		    agxset(e, sym, value);
	    }
	}
    } else
	fprintf(stderr,
	    "on_attrApplyBtn_clicked: unknown object kind %d\n",
	    objKind);
    set_refresh_filters(view, attr_name);
}

_BB void on_attrApplyBtn_clicked(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    doApply();
}

_BB void on_attrRB0_clicked(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    filter_attributes(gtk_entry_get_text((GtkEntry*)glade_xml_get_widget(xml,
							      "txtAttr")), view->Topview);

}

/* This is the action attached to the publish button on the attributes
 * window. What should happen?
 */
_BB void on_attrProg_toggled(GtkWidget * widget, gpointer user_data)
{
  (void)widget;
  (void)user_data;
  /* FIX */
}

_BB void on_attrAddBtn_clicked(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    char *attr_name;
    int objKind;
    topview *t;
    Agraph_t *g;
    attr_t *attr;
    objKind = get_object_type();

    /*values to be applied to selected objects */
    attr_name =
	(char *) gtk_entry_get_text((GtkEntry *)
				    glade_xml_get_widget(xml, "txtAttr"));
    const char *defValue = gtk_entry_get_text((GtkEntry *)
				    glade_xml_get_widget(xml,
							 "txtDefValue"));
    g = view->g[view->activeGraph];
    t = view->Topview;
    /*try to find first */
    attr = binarySearch(t->attributes, attr_name);
    if (!attr) {
	attr = new_attr();
	attr->index = 0;
	attr->name = safestrdup(attr_name);
	attr->type = attr_alpha;
	attr->value = gv_strdup("");
	attr->widget = NULL;
	attr_list_add(t->attributes, attr);
    }
    attr->propagate = 0;

    if (objKind == AGRAPH) {
	agattr(g, AGRAPH, attr_name, defValue);
	attr->defValG = safestrdup(defValue);
	attr->objType[0] = 1;
    }

    /*nodes */
    else if (objKind == AGNODE) {
	agattr(g, AGNODE, attr_name, defValue);
	attr->defValN = safestrdup(defValue);
	attr->objType[1] = 1;
    } else if (objKind == AGEDGE) {
	agattr(g, AGEDGE, attr_name, defValue);
	attr->defValE = safestrdup(defValue);
	attr->objType[2] = 1;
    } else
	fprintf(stderr, "on_attrAddBtn_clicked: unknown object kind %d\n",
		objKind);
    filter_attributes(attr_name, view->Topview);
}

attr_list *load_attr_list(Agraph_t * g)
{
    attr_t *attr;
    attr_list *l;
    FILE *file;
    char buffer[BUFSIZ];
    static char *smyrna_attrs;
    char *a;

    if (!smyrna_attrs)
	smyrna_attrs = smyrnaPath("attrs.txt");
    g = view->g[view->activeGraph];
    l = attr_list_new(true);
    file = fopen(smyrna_attrs, "r");
    if (file != NULL) {
	for (size_t i = 0; fgets(buffer, sizeof(buffer), file) != NULL; ++i) {
	    attr = new_attr();
	    a = strtok(buffer, ",");
	    attr->index = i;
	    attr->type = get_attr_data_type(a[0]);
	    for (int idx = 0; (a = strtok(NULL, ",")); ++idx) {
		/*C,(0)color, (1)black, (2)EDGE Or NODE Or CLUSTER, (3)ALL_ENGINES */

		switch (idx) {
		case 0:
		    attr->name = safestrdup(a);
		    break;
		case 1:
		    attr->defValG = safestrdup(a);
		    attr->defValN = safestrdup(a);
		    attr->defValE = safestrdup(a);
		    break;
		case 2:
		    set_attr_object_type(a, attr->objType);
		    break;
		}
	    }
	    attr_list_add(l, attr);

	}
	fclose(file);
    }
    for (Agsym_t *sym = NULL; (sym = agnxtattr(g, AGRAPH, sym)); ) {
	attr = binarySearch(l, sym->name);
	if (attr)
	    attr->objType[0] = 1;
	else {
	    attr = new_attr_with_ref(sym);
	    attr_list_add(l, attr);
	}
    }
    for (Agsym_t *sym = NULL; (sym = agnxtattr(g, AGNODE, sym)); ) {
	attr = binarySearch(l, sym->name);
	if (attr) {
	    attr->objType[1] = 1;

	} else {
	    attr = new_attr_with_ref(sym);
	    attr_list_add(l, attr);
	}

    }
    for (Agsym_t *sym = NULL; (sym = agnxtattr(g, AGEDGE, sym)); ) {
	attr = binarySearch(l, sym->name);
	if (attr)
	    attr->objType[2] = 1;
	else {
	    attr = new_attr_with_ref(sym);
	    attr_list_add(l, attr);
	}
    }
    return l;
}

static void set_header_text(void)
{
    int nodeCnt = 0;
    int edgeCnt = 0;
    char buf[512];
    Agedge_t* ep;
    Agnode_t* v;
    Agraph_t* g;

    g = view->g[view->activeGraph];
    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	if (ND_selected(v))
	    nodeCnt++;
	for (ep = agfstout(g, v); ep; ep = agnxtout(g, ep)) {
	    if (ND_selected(v))
		edgeCnt++;
	}
    }
    sel_node = nodeCnt;
    sel_edge = edgeCnt;
    sel_graph = 1;

    snprintf(buf, sizeof(buf), "%d Nodes and %d edges selected", nodeCnt,
             edgeCnt);
    gtk_label_set_text((GtkLabel *) glade_xml_get_widget(xml, "label124"),
		       buf);
    gtk_entry_set_text((GtkEntry *) glade_xml_get_widget(xml, "txtAttr"),
		       "");
    Color_Widget_bg("white", glade_xml_get_widget(xml, "fixed6"));
}

void showAttrsWidget(void) {
    gtk_widget_hide(glade_xml_get_widget(xml, "dlgSettings"));
    gtk_widget_show(glade_xml_get_widget(xml, "dlgSettings"));
    gtk_notebook_set_current_page((GtkNotebook *)
				  glade_xml_get_widget(xml, "notebook3"),
				  ATTR_NOTEBOOK_IDX);
    set_header_text();
    filter_attributes("", view->Topview);
}

static void gvpr_select(const char *attrname, const char *regex_str,
                        int objType) {

    char *bf2;
    int i, argc;

    agxbuf sf = {0};

    if (objType == AGNODE)
	agxbprint(&sf, "N[%s==\"%s\"]{selected = \"1\"}", attrname, regex_str);
    else if (objType == AGEDGE)
	agxbprint(&sf, "E[%s==\"%s\"]{selected = \"1\"}", attrname, regex_str);

    bf2 = agxbdisown(&sf);

    argc = 1;
    if (*bf2 != '\0')
	argc++;
    char *argv[3] = {0};
    size_t j = 0;
    argv[j++] = "smyrna";
    argv[j++] = bf2;

    run_gvpr(view->g[view->activeGraph], j, argv);
    for (i = 1; i < argc; i++)
	free(argv[i]);
    set_header_text();
}

_BB void on_attrSearchBtn_clicked(GtkWidget * widget, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    const char *attrname = gtk_entry_get_text((GtkEntry *)
				    glade_xml_get_widget(xml, "txtAttr"));
    const char *regex_str = gtk_entry_get_text((GtkEntry *)
				    glade_xml_get_widget(xml, "txtValue"));
    gvpr_select(attrname, regex_str, get_object_type());

}
