/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include "config.h"

#include	<cgraph/alloc.h>
#include	<common/memory.h>
#include	<common/types.h>
#include        <gvc/gvplugin.h>
#include        <gvc/gvcjob.h>
#include        <gvc/gvcint.h>
#include        <gvc/gvcproc.h>
#include        <stdbool.h>

static GVJ_t *output_filename_job;
static GVJ_t *output_langname_job;

/*
 * -T and -o can be specified in any order relative to the other, e.g.
 *            -T -T -o -o
 *            -T -o -o -T
 * The first -T is paired with the first -o, the second with the second, and so on.
 *
 * If there are more -T than -o, then the last -o is repeated for the remaining -T
 * and vice-versa
 *
 * If there are no -T or -o then a single job is instantiated.
 *
 * If there is no -T on the first job, then "dot" is used.
 *
 * As many -R as are specified before a completed -T -o pair (according to the above rules)
 * are used as renderer-specific switches for just that one job.  -R must be restated for 
 * each job.
 */

/* -o switches */
void gvjobs_output_filename(GVC_t * gvc, const char *name)
{
    if (!gvc->jobs) {
	output_filename_job = gvc->job = gvc->jobs = gv_alloc(sizeof(GVJ_t));
    } else {
	if (!output_filename_job) {
	    output_filename_job = gvc->jobs;
	} else {
	    if (!output_filename_job->next) {
		output_filename_job->next = gv_alloc(sizeof(GVJ_t));
	    }
	    output_filename_job = output_filename_job->next;
	}
    }
    output_filename_job->output_filename = name;
    output_filename_job->gvc = gvc;
}

/* -T switches */
bool gvjobs_output_langname(GVC_t * gvc, const char *name)
{
    if (!gvc->jobs) {
	output_langname_job = gvc->job = gvc->jobs = gv_alloc(sizeof(GVJ_t));
    } else {
	if (!output_langname_job) {
	    output_langname_job = gvc->jobs;
	} else {
	    if (!output_langname_job->next) {
		output_langname_job->next = gv_alloc(sizeof(GVJ_t));
	    }
	    output_langname_job = output_langname_job->next;
	}
    }
    output_langname_job->output_langname = name;
    output_langname_job->gvc = gvc;

    /* load it now to check that it exists */
    if (gvplugin_load(gvc, API_device, name))
	return true;
    return false;
}

GVJ_t *gvjobs_first(GVC_t * gvc)
{
    return (gvc->job = gvc->jobs);
}

GVJ_t *gvjobs_next(GVC_t * gvc)
{
    GVJ_t *job = gvc->job->next;

    if (job) {
	/* if langname not specified, then repeat previous value */
	if (!job->output_langname)
	    job->output_langname = gvc->job->output_langname;
	/* if filename not specified, then leave NULL to indicate stdout */
    }
    return (gvc->job = job);
}

void gv_argvlist_set_item(gv_argvlist_t *list, int index, char *item)
{
    if (index >= list->alloc) {
	list->alloc = index + 10;
	list->argv = grealloc(list->argv, (list->alloc)*(sizeof(char*)));
    }
    list->argv[index] = item;
}

void gv_argvlist_reset(gv_argvlist_t *list)
{
    free(list->argv);
    list->argv = NULL;
    list->alloc = 0;
    list->argc = 0;
}

void gvjobs_delete(GVC_t * gvc)
{
    GVJ_t *job, *j;

    job = gvc->jobs;
    while ((j = job)) {
	job = job->next;
	gv_argvlist_reset(&(j->selected_obj_attributes));
	gv_argvlist_reset(&(j->selected_obj_type_name));
	free(j->active_tooltip);
	free(j->selected_href);
	free(j);
    }
    gvc->jobs = gvc->job = gvc->active_jobs = output_filename_job = output_langname_job =
	NULL;
    gvc->common.viewNum = 0;
}
