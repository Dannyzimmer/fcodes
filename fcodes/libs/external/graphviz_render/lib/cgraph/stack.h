/// \file
/// \brief Implementation of a dynamically expanding stack data structure

#pragma once

#include <assert.h>
#include <cgraph/list.h>
#include <stdbool.h>

DEFINE_LIST(gv_stack, void *)

static inline size_t stack_size(const gv_stack_t *stack) {
  return gv_stack_size(stack);
}

static inline bool stack_is_empty(const gv_stack_t *stack) {
  return gv_stack_is_empty(stack);
}

static inline void stack_push(gv_stack_t *stack, void *item) {
  gv_stack_push(stack, item);
}

static inline void *stack_top(gv_stack_t *stack) {

  assert(stack != NULL);
  assert(!stack_is_empty(stack) && "access to top of an empty stack");

  return gv_stack_get(stack, gv_stack_size(stack) - 1);
}

static inline void *stack_pop(gv_stack_t *stack) { return gv_stack_pop(stack); }

static inline void stack_reset(gv_stack_t *stack) { gv_stack_free(stack); }
