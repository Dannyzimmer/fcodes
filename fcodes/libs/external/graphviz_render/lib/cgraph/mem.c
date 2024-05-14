/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <cgraph/cghdr.h>
#include <stdlib.h>

void *agalloc(Agraph_t * g, size_t size)
{
    (void)g;

    void *mem = calloc(1, size);
    if (mem == NULL)
	 agerr(AGERR,"memory allocation failure");
    return mem;
}

void *agrealloc(Agraph_t * g, void *ptr, size_t oldsize, size_t size)
{
    void *mem;

    if (size > 0) {
	if (ptr == 0)
	    mem = agalloc(g, size);
	else {
	    mem = realloc(ptr, size);
	    if (mem != NULL && size > oldsize) {
	        memset((char*)mem + oldsize, 0, size - oldsize);
	    }
	}
	if (mem == NULL)
	     agerr(AGERR,"memory re-allocation failure");
    } else
	mem = NULL;
    return mem;
}

void agfree(Agraph_t * g, void *ptr)
{
    (void)g;

    if (ptr)
	free(ptr);
}
