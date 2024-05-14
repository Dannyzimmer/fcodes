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
 * AT&T Research
 *
 * return the next character in the string s
 * \ character constants are converted
 * p is updated to point to the next character in s
 */

#include <ast/ast.h>

int chresc(const char *s, char **p)
{
    const char *q;
    int c;

    switch (c = *s++) {
    case 0:
	s--;
	break;
    case '\\':
	switch (c = *s++) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	    c -= '0';
	    q = s + 2;
	    while (s < q)
		switch (*s) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		    c = (c << 3) + *s++ - '0';
		    break;
		default:
		    q = s;
		    break;
		}
	    break;
	case 'a':
	    c = CC_bel;
	    break;
	case 'b':
	    c = '\b';
	    break;
	case 'f':
	    c = '\f';
	    break;
	case 'n':
	    c = '\n';
	    break;
	case 'r':
	    c = '\r';
	    break;
	case 's':
	    c = ' ';
	    break;
	case 't':
	    c = '\t';
	    break;
	case 'v':
	    c = CC_vt;
	    break;
	case 'x':
	    c = 0;
	    q = s;
	    while (q)
		switch (*s) {
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		    c = (c << 4) + *s++ - 'a' + 10;
		    break;
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		    c = (c << 4) + *s++ - 'A' + 10;
		    break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		    c = (c << 4) + *s++ - '0';
		    break;
		default:
		    q = 0;
		    break;
		}
	    break;
	case 'E':
	    c = CC_esc;
	    break;
	case 0:
	    s--;
	    break;
	}
	break;
    }
    if (p)
	*p = (char *) s;
    return c;
}
