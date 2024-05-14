/// \file
/// \brief language-specific bindings API
///
/// Each Graphviz language binding is expected to implement the following
/// functions as relevant for their implementation.

#pragma once

#include <gvc/gvc.h>

#ifdef __cplusplus
extern "C" {
#endif

void gv_string_writer_init(GVC_t *gvc);
void gv_channel_writer_init(GVC_t *gvc);
void gv_writer_reset(GVC_t *gvc);

#ifdef __cplusplus
}
#endif
