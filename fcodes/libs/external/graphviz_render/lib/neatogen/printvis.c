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
#include <pathplan/vis.h>
typedef Ppoint_t point;


void printvis(vconfig_t * cp)
{
    int i, j;
    int *next, *prev;
    point *pts;
    array2 arr;

    next = cp->next;
    prev = cp->prev;
    pts = cp->P;
    arr = cp->vis;

    printf("this next prev point\n");
    for (i = 0; i < cp->N; i++)
	printf("%3d  %3d  %3d    (%f,%f)\n", i, next[i], prev[i], pts[i].x, pts[i].y);

    printf("\n\n");

    for (i = 0; i < cp->N; i++) {
	for (j = 0; j < cp->N; j++)
	    printf("%4.1f ", arr[i][j]);
	printf("\n");
    }
}
