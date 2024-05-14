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
 * AT&T Bell Laboratories
 *
 * convert \x character constants in s in place
 */

#include <ast/ast.h>

void stresc(char *s)
{
    char *t;
    int c;
    char *p;

    t = s;
    for (;;) {
	switch (c = *s++) {
	case '\\':
	    c = chresc(s - 1, &p);
	    s = p;
	    break;
	case 0:
	    *t = 0;
	    return;
	default: // nothing required
	    break;
	}
	*t++ = c;
    }
}
