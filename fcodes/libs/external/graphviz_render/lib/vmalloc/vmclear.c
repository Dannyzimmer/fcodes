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

/** Clear out all allocated space.
 *
 * Note that this leaves the allocation region itself usable, but just frees all
 * previous allocations made within this region.
 *
 * @param vm Vmalloc to operate on
 */
void vmclear(Vmalloc_t *vm) {

  // free all allocated pointers
  for (size_t i = 0; i < vm->size; ++i) {
    free(vm->allocated[i]);
  }

  // reset our metadata
  free(vm->allocated);
  vm->allocated = NULL;
  vm->size = vm->capacity = 0;
}
