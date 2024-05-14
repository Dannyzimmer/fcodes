/**
 * @file
 * @brief function colorxlate (one of many) for gvcolor.c
 */

/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <cgraph/agxbuf.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_SEARCH_H
#include <search.h>
#endif
#include <ctype.h>
typedef struct {
    char *name;
    unsigned char h, s, b;
} hsbcolor_t;

#include "colortbl.h"
#include "colorxlate.h"

static char *canoncolor(const char *orig, char *out)
{
    char c;
    char *p = out;
    while ((c = *orig++)) {
	if (!isalnum(c))
	    continue;
	if (isupper(c))
	    c = (char)tolower(c);
	*p++ = c;
    }
    *p = '\0';
    return out;
}

static int colorcmpf(const void *a0, const void *a1)
{
    const hsbcolor_t *p0 = a0;
    const hsbcolor_t *p1 = a1;
    return strcmp(p0->name, p1->name);
}

void colorxlate(char *str, agxbuf *buf) {
    static hsbcolor_t *last;
    char canon[128];
    hsbcolor_t fake;

    if (last == NULL || strcmp(last->name, str)) {
	fake.name = canoncolor(str, canon);
	last = bsearch(&fake, color_lib, sizeof(color_lib) / sizeof(hsbcolor_t),
	               sizeof(fake), colorcmpf);
    }
    if (last == NULL) {
	if (!isdigit((int)canon[0])) {
	    fprintf(stderr, "warning: %s is not a known color\n", str);
	    agxbput(buf, str);
	} else
	    for (const char *p = str; *p != '\0'; ++p)
		agxbputc(buf, *p == ',' ? ' ' : *p);
    } else
	agxbprint(buf, "%.3f %.3f %.3f", ((double) last->h) / 255,
		((double) last->s) / 255, ((double) last->b) / 255);
}
