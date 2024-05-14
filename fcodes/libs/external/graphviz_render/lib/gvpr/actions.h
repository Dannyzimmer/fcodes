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

#include "cgraph.h"
#include <expr/expr.h>
#include <stdio.h>

    extern void nodeInduce(Agraph_t * selected);
    extern Agobj_t *cloneO(Agraph_t * g, Agobj_t * obj);
    extern Agraph_t *cloneG(Agraph_t * g, char* name);
    extern Agobj_t *copy(Agraph_t * g, Agobj_t * obj);
    extern int copyAttr(Agobj_t * obj, Agobj_t * obj1);
    extern int indexOf(char *s1, char *s2);
    extern long rindexOf(char *s1, char *s2);
    extern int match(char *str, char *pat);
    extern int lockGraph(Agraph_t * g, int);
    extern Agraph_t *compOf(Agraph_t * g, Agnode_t * n);
    extern Agedge_t *isEdge(Agraph_t* g, Agnode_t * t, Agnode_t * h, char *key);
    extern Agnode_t *addNode(Agraph_t * g, Agnode_t * n, int doAdd);
    extern Agedge_t *addEdge(Agraph_t * g, Agedge_t * e, int doAdd);
    extern Agraph_t *sameG(void *p1, void *p2, char *fn, char *msg);
    extern int compare(Agobj_t *, Agobj_t *);
    extern int sfioWrite(Agraph_t*, FILE*);
    extern int writeFile(Agraph_t*, char*);
    extern int fwriteFile(Expr_t*, Agraph_t*, int);
    extern Agraph_t *readFile(char *);
    extern Agraph_t *freadFile(Expr_t *, int);
    extern int openFile(Expr_t *, const char *, const char *);
    extern int closeFile(Expr_t *, int);
    extern char *readLine(Expr_t *, int);
    extern char *canon(Expr_t * pgm, char *);
    extern char *toHtml(Agraph_t*, char *);
    extern char *toLower(Expr_t *pgm, char *src);
    extern char *toUpper(Expr_t *pgm, char *src);
    extern int deleteObj(Agraph_t * g, Agobj_t * obj);
    extern char *colorx (Expr_t* ex, char* incolor, char* fmt);
    extern void gvstart_timer(void);
    extern double gvelapsed_sec(void);

#ifdef __cplusplus
}
#endif
