/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <gvc/gvc.h>
#include <gvc/gvplugin.h>
#include <gvc/gvcjob.h>
#include <gvc/gvcint.h>
#include "gv_channel.h"

typedef struct {
    char* data;
    size_t sz;       /* buffer size */
    size_t len;      /* length of array */
} BA;

static size_t gv_string_writer(GVJ_t *job, const char *s, size_t len)
{
    BA* bap = (BA*)(job->output_file);
    size_t newlen = bap->len + len;
    if (newlen > bap->sz) {
	bap->sz *= 2;
	if (newlen > bap->sz)
	    bap->sz = 2*newlen;
        bap->data = realloc(bap->data, bap->sz);
    }
    memcpy (bap->data+bap->len, s, len); 
    bap->len = newlen;
    return len;
}

void gv_string_writer_init(GVC_t *gvc)
{
    gvc->write_fn = gv_string_writer;
}

static size_t gv_channel_writer(GVJ_t *job, const char *s, size_t len)
{
    (void)job;
    (void)s;
    return len;
}

void gv_channel_writer_init(GVC_t *gvc)
{
    gvc->write_fn = gv_channel_writer;
}

void gv_writer_reset (GVC_t *gvc) {gvc->write_fn = NULL;}
