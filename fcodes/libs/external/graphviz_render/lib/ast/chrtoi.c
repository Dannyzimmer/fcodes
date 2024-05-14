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
 * convert a 0 terminated character constant string to an int
 */

#include <ast/ast.h>
#include <limits.h>
#include <stddef.h>

int chrtoi(const char *s)
{
    int c;
    int x;
    char *p;

    c = 0;
    for (size_t n = 0; n < sizeof(int) * CHAR_BIT; n += CHAR_BIT) {
	switch (x = *((const unsigned char *) s++)) {
	case '\\':
	    x = chresc(s - 1, &p);
	    s = p;
	    break;
	case 0:
	    return (c);
	default: // nothing required
	    break;
	}
	c = (c << CHAR_BIT) | x;
    }
    return (c);
}
