/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

/*
 * Glenn Fowler
 * AT&T Labs Research
 *
 * in-place path name canonicalization -- preserves the logical view
 * pointer to trailing 0 in path returned
 *
 *	remove redundant .'s and /'s
 *	move ..'s to the front
 *	/.. preserved (for pdu and newcastle hacks)
 */

#include <ast/ast.h>
#include <ast/error.h>

char *pathcanon(char *path) {
    char *r;
    char *s;
    char *t;
    int dots;

    dots = 0;
    if (*path == '/' && *(path + 1) == '/')
	do
	    path++;
	while (*path == '/' && *(path + 1) == '/');
    r = s = t = path;
    for (;;)
	switch (*t++ = *s++) {
	case '.':
	    dots++;
	    break;
	case 0:
	    s--;
	 /*FALLTHROUGH*/ case '/':
	    while (*s == '/')
		s++;
	    switch (dots) {
	    case 1:
		t -= 2;
		break;
	    case 2:
		if (t - 5 < r) {
		    if (t - 4 == r)
			t = r + 1;
		    else
			r = t;
		}
		else
		    for (t -= 5; t > r && *(t - 1) != '/'; t--);
		break;
	    case 3:
		r = t;
		break;
	    default:
		break;
	    }
	    if (!*s) {
		if (t > path && !*(t - 1))
		    t--;
		if (t == path)
		    *t++ = '.';
		else if ((s <= path || *(s - 1) != '/') && t > path + 1
			 && *(t - 1) == '/')
		    t--;
		*t = 0;
		return t;
	    }
	    dots = 0;
	    break;
	default:
	    dots = 4;
	    break;
	}
}
