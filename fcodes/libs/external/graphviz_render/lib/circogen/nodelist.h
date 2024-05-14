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

#ifdef __cplusplus
extern "C" {
#endif

#include  <cgraph/list.h>
#include  <render.h>
#include  <stddef.h>

    DEFINE_LIST(nodelist, node_t*)

    extern nodelist_t *mkNodelist(void);
    extern void freeNodelist(nodelist_t *);

    extern void appendNodelist(nodelist_t*, size_t, Agnode_t *n);

    extern void realignNodelist(nodelist_t *list, size_t np);
    extern void insertNodelist(nodelist_t *, Agnode_t *, Agnode_t *, int);

    extern void reverseAppend(nodelist_t *, nodelist_t *);
    extern void reverseNodelist(nodelist_t *list);
    extern nodelist_t *cloneNodelist(nodelist_t * list);

#ifdef DEBUG
    extern void printNodelist(nodelist_t * list);
#endif

#ifdef __cplusplus
}
#endif
