/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <vmalloc/vmalloc.h>
#include <stdlib.h>

/** Open a new region of allocation.
 *
 * @returns A newly allocated Vmalloc or NULL on failure
 */
Vmalloc_t *vmopen(void) {

  Vmalloc_t *vm = calloc(1, sizeof(*vm));
  if (vm == NULL) {
    return NULL;
  }

  return vm;
}
