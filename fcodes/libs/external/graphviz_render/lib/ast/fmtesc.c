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
 * return string with expanded escape chars
 */

#include <ast/ast.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

/*
 * quote string as of length n with qb...qe
 */

char *fmtquote(const char *as, const char *qb, const char *qe, size_t n) {
    const unsigned char *s = (const unsigned char *) as;
    const unsigned char *e = s + n;
    char *b;
    int escaped;
    int spaced;
    int shell;
    char *f;
    char *buf;

    size_t len = 4 * (n + 1);
    if (qb)
	len += strlen(qb);
    if (qe)
	len += strlen(qe);
    b = buf = fmtbuf(len);
    shell = 0;
    if (qb) {
	if (qb[0] == '$' && qb[1] == '\'' && qb[2] == 0)
	    shell = 1;
	while ((*b = *qb++))
	    b++;
    }
    f = b;
    escaped = spaced = 0;
    while (s < e) {
	    int c = *s++;
	    if (iscntrl(c) || !isprint(c) || c == '\\') {
		escaped = 1;
		*b++ = '\\';
		switch (c) {
		case CC_bel:
		    c = 'a';
		    break;
		case '\b':
		    c = 'b';
		    break;
		case '\f':
		    c = 'f';
		    break;
		case '\n':
		    c = 'n';
		    break;
		case '\r':
		    c = 'r';
		    break;
		case '\t':
		    c = 't';
		    break;
		case CC_vt:
		    c = 'v';
		    break;
		case CC_esc:
		    c = 'E';
		    break;
		case '\\':
		    break;
		default:
		    *b++ = (char)('0' + ((c >> 6) & 07));
		    *b++ = (char)('0' + ((c >> 3) & 07));
		    c = '0' + (c & 07);
		    break;
		}
	    } else if (c == '\\') {
		escaped = 1;
		*b++ = (char)c;
		if (*s)
		    c = *s++;
	    } else if (qe && strchr(qe, c)) {
		escaped = 1;
		*b++ = '\\';
	    } else if (!spaced &&
		       !escaped &&
		       (isspace(c) ||
			(shell &&
			 (strchr("\";~&|()<>[]*?", c) ||
			  (c == '#' && (b == f || isspace((int)*(b - 1)))
			  )
			 )
			)
		       )
		)
		spaced = 1;
	    *b++ = (char)c;
    }
    if (qb) {
	if (!escaped)
	    buf += shell + !spaced;
	if (qe && (escaped || spaced))
	    while ((*b = *qe++))
		b++;
    }
    *b = 0;
    return buf;
}

/*
 * escape the usual suspects and quote chars in qs
 */

char *fmtesq(const char *as, const char *qs)
{
  return fmtquote(as, NULL, qs, strlen(as));
}

/*
 * escape the usual suspects
 */

char *fmtesc(const char *as)
{
  return fmtquote(as, NULL, NULL, strlen(as));
}
