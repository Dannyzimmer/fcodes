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
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/** make room to store a new pointer we are about to allocate
 *
 * @param vm Vmalloc to operate on
 * @returns true on success
 */
static bool make_space(Vmalloc_t *vm) {

  if (vm->size == vm->capacity) {

    // expand our allocation storage
    size_t c = vm->capacity == 0 ? 1 : vm->capacity * 2;
    void **p = realloc(vm->allocated, sizeof(vm->allocated[0]) * c);
    if (p == NULL) {
      return false;
    }

    // save the new array
    vm->allocated = p;
    vm->capacity = c;
  }

  return true;
}

void *vmalloc(Vmalloc_t *vm, size_t size) {

  if (!make_space(vm)) {
    return NULL;
  }

  void *p = malloc(size);
  if (p == NULL) {
    return NULL;
  }

  vm->allocated[vm->size] = p;
  ++vm->size;

  return p;
}

void vmfree(Vmalloc_t *vm, void *data) {

  if (!data) { // ANSI-ism
    return;
  }

  // find the pointer we previously allocated
  for (size_t i = 0; i < vm->size; ++i) {
    if (vm->allocated[i] == data) {

      // clear this slot
      size_t extent = sizeof(vm->allocated[0]) * (vm->size - i - 1);
      memmove(vm->allocated + i, vm->allocated + i + 1, extent);
      --vm->size;

      // give this back to the underlying allocator
      free(data);

      return;
    }
  }

  // we did not find this pointer; free() of something we did not allocate
}
