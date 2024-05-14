/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <stddef.h>
#include <string.h>
#include <vmalloc/vmalloc.h>

/*
 * return a copy of s using vmalloc
 */

char *vmstrdup(Vmalloc_t * v, const char *s)
{

  size_t len = strlen(s) + 1;
  char *t = vmalloc(v, len);
  if (t == NULL) {
    return NULL;
  }

  memcpy(t, s, len);

  return t;
}
