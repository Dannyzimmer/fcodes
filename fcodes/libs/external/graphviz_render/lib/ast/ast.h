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

#include <sfio/sfio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PATH_MAX
#define PATH_MAX    1024
#endif

/*
 * strgrpmatch() flags
 */

#define STR_MAXIMAL 01		/* maximal match        */
#define STR_LEFT    02		/* implicit left anchor     */
#define STR_RIGHT   04		/* implicit right anchor    */
#define STR_ICASE   010		/* ignore case          */

#define CC_bel      0007	/* bel character        */
#define CC_esc      0033	/* esc character        */
#define CC_vt       0013	/* vt character         */

#define elementsof(x)   (sizeof(x)/sizeof(x[0]))
#define streq(a,b)  (!strcmp(a,b))
#define strneq(a,b,n)     (!strncmp(a,b,n))
#define memzero(b,n)    memset(b,0,n)

    extern char *pathpath(char *, const char *);
    extern char *pathfind(const char *, const char *, const char *, char *,
			  size_t);
    extern char *pathaccess(char*, const char*, const char*, const char*);
    extern char *pathcanon(char*);
    extern const char *pathcat(char *, const char *, int, const char *,
			 const char *);

    extern int chresc(const char *, char **);
    extern int chrtoi(const char *);
    extern char *fmtesq(const char *, const char *);
    extern char *fmtesc(const char *as);
    extern char *fmtbuf(size_t n);
    extern char *fmtquote(const char*, const char*, const char*, size_t);

    extern int strmatch(const char *, const char *);
    extern int strgrpmatch(const char *, const char *, int *, int, int);
    extern void stresc(char *);
    extern char *strcopy(char *s, const char *t);

#ifdef __cplusplus
}
#endif
