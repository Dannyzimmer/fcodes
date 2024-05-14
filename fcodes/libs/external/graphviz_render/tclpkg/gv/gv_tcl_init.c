/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <tcl.h>
#include <gvc/gvc.h>
#include <gvc/gvplugin.h>
#include <gvc/gvcjob.h>
#include <gvc/gvcint.h>
#include <limits.h>
#include "gv_channel.h"

static size_t gv_string_writer(GVJ_t *job, const char *s, size_t len)
{
  // clamp to INT_MAX
  int l = len > (size_t)INT_MAX ? INT_MAX : (int)len;
  Tcl_AppendToObj((Tcl_Obj*)(job->output_file), s, l);
  return (size_t)l;
}

static size_t gv_channel_writer(GVJ_t *job, const char *s, size_t len)
{
  // clamp to INT_MAX
  int l = len > (size_t)INT_MAX ? INT_MAX : (int)len;
  return (size_t)Tcl_Write((Tcl_Channel)(job->output_file), s, l);
}

void gv_string_writer_init(GVC_t *gvc)
{
    gvc->write_fn = gv_string_writer;
}

void gv_channel_writer_init(GVC_t *gvc)
{
    gvc->write_fn = gv_channel_writer;
}

void gv_writer_reset (GVC_t *gvc) {gvc->write_fn = NULL;}
