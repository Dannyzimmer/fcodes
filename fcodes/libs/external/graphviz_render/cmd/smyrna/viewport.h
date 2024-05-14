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

//view data structure
#include "smyrnadefs.h"
#include <gtk/gtk.h>
#include <xdot/xdot.h>
#include <cgraph/cgraph.h>

    void init_viewport(ViewInfo * view);
    void set_viewport_settings_from_template(ViewInfo * view, Agraph_t *);
    void switch_graph(int);
    void refreshViewport(void);
    int add_graph_to_viewport_from_file(char *fileName);
    int add_graph_to_viewport(Agraph_t * graph, char *);
    void close_graph(ViewInfo * view);
    int save_graph(void);
    int save_graph_with_file_name(Agraph_t * graph, char *fileName);
    int save_as_graph(void);

    int do_graph_layout(Agraph_t * graph, int Engine, int keeppos);

    void glexpose(void);
    void move_nodes(Agraph_t * g);
    extern void getcolorfromschema(colorschemaset * sc, float l,
				   float maxl, glCompColor * c);
    void updateRecord (Agraph_t* g);


    /* helper functions */
    extern char *get_attribute_value(char *attr, ViewInfo * view,
				     Agraph_t * g);
