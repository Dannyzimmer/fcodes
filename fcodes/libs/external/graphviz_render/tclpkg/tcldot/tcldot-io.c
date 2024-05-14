/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/


#include "tcldot.h"

/*
 * myiodisc_afread - same api as read for libcgraph
 *
 * gets one line at a time from a Tcl_Channel and places it in a user buffer
 *    up to a maximum of n characters
 *
 * returns pointer to obtained line in user buffer, or
 * returns NULL when last line read from memory buffer
 *
 * This is probably innefficient because it introduces
 * one more stage of line buffering during reads (at least)
 * but it is needed so that we can take full advantage
 * of the Tcl_Channel mechanism.
 */
int myiodisc_afread(void* channel, char *ubuf, int n)
{
    static Tcl_DString dstr;
    static int strpos;
    int nput;

    if (!n) {			/* a call with n==0 (from aglexinit) resets */
	*ubuf = '\0';
	strpos = 0;
	return 0;
    }

    /* 
     * the user buffer might not be big enough to hold the line.
     */
    if (strpos) {
	nput = Tcl_DStringLength(&dstr) - strpos;
	if (nput > n) {
	    /* chunk between first and last */
	    memcpy(ubuf, strpos + Tcl_DStringValue(&dstr), n);
	    strpos += n;
	    nput = n;
	    ubuf[n] = '\0';
	} else {
	    /* last chunk */
	    memcpy(ubuf, strpos + Tcl_DStringValue(&dstr), nput);
	    strpos = 0;
	}
    } else {
	Tcl_DStringFree(&dstr);
	Tcl_DStringInit(&dstr);
	if (Tcl_Gets((Tcl_Channel) channel, &dstr) < 0) {
	    /* probably EOF, but could be other read errors */
	    *ubuf = '\0';
	    return 0;
	}
	/* linend char(s) were stripped off by Tcl_Gets,
	 * append a canonical linenend. */
	Tcl_DStringAppend(&dstr, "\n", 1);
	if (Tcl_DStringLength(&dstr) > n) {
	    /* first chunk */
	    nput = n;
	    memcpy(ubuf, Tcl_DStringValue(&dstr), n);
	    strpos = n;
	} else {
	    /* single chunk */
	    nput = Tcl_DStringLength(&dstr);
	    memcpy(ubuf, Tcl_DStringValue(&dstr),nput);
	}
    }
    return nput;
}


/* exact copy from cgraph/io.c - but that one is static */
int myiodisc_memiofread(void *chan, char *buf, int bufsize)
{
    const char *ptr;
    char *optr;
    char c;
    int l;
    rdr_t *s;

    if (bufsize == 0) return 0;
    s = chan;
    if (s->cur >= s->len)
        return 0;
    l = 0;
    ptr = s->data + s->cur;
    optr = buf;
    do {
        *optr++ = c = *ptr++;
        l++;
    } while (c && c != '\n' && l < bufsize);
    s->cur += l;
    return l;
}
