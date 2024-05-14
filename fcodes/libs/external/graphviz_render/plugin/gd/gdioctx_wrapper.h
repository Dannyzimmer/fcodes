#pragma once

#include <gd.h>
#include <gvc/gvcext.h>
#include <stddef.h>

typedef struct {
  gdIOCtx ctx;
  GVJ_t *job;
} gd_context_t;

static inline gd_context_t *get_containing_context(gdIOCtx *ctx) {
  return (gd_context_t *)((char *)ctx - offsetof(gd_context_t, ctx));
}
