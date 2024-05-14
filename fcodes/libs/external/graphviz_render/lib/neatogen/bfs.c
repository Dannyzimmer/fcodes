/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/


/******************************************

	Breadth First Search
	Computes single-source distances for
	unweighted graphs

******************************************/

#include <cgraph/alloc.h>
#include <neatogen/bfs.h>
#include <stdbool.h>
#include <stdlib.h>

void bfs(int vertex, vtx_data *graph, int n, DistType *dist)
 /* compute vector 'dist' of distances of all nodes from 'vertex' */
{
    int i;
    int closestVertex, neighbor;
    DistType closestDist = INT_MAX;

    /* initial distances with edge weights: */
    for (i = 0; i < n; i++)
	dist[i] = -1;
    dist[vertex] = 0;

    Queue Q;
    mkQueue(&Q, n);
    initQueue(&Q, vertex);

    if (graph[0].ewgts == NULL) {
	while (deQueue(&Q, &closestVertex)) {
	    closestDist = dist[closestVertex];
	    for (i = 1; i < graph[closestVertex].nedges; i++) {
		neighbor = graph[closestVertex].edges[i];
		if (dist[neighbor] < -0.5) {	/* first time to reach neighbor */
		    dist[neighbor] = closestDist + 1;
		    enQueue(&Q, neighbor);
		}
	    }
	}
    } else {
	while (deQueue(&Q, &closestVertex)) {
	    closestDist = dist[closestVertex];
	    for (i = 1; i < graph[closestVertex].nedges; i++) {
		neighbor = graph[closestVertex].edges[i];
		if (dist[neighbor] < -0.5) {	/* first time to reach neighbor */
		    dist[neighbor] =
			closestDist +
			(DistType) graph[closestVertex].ewgts[i];
		    enQueue(&Q, neighbor);
		}
	    }
	}
    }

    /* For dealing with disconnected graphs: */
    for (i = 0; i < n; i++)
	if (dist[i] < -0.5)	/* 'i' is not connected to 'vertex' */
	    dist[i] = closestDist + 10;

    freeQueue(&Q);
}

int
bfs_bounded(int vertex, vtx_data * graph, DistType * dist,
	    int bound, int *visited_nodes, int queue_size)
 /* compute vector 'dist' of distances of all nodes  from 'vertex' */
 /* ignore nodes whose distance to 'vertex' is more than bound */
{
    /* we assume here, that all distances are initialized with -1 !!!! */

    int i;
    int num_visit;
    int closestVertex, neighbor;
    DistType closestDist;

    dist[vertex] = 0;

    Queue Q;
    mkQueue(&Q, queue_size);
    initQueue(&Q, vertex);

    num_visit = 0;
    while (deQueue(&Q, &closestVertex)) {
	closestDist = dist[closestVertex];
	if (closestDist > bound) {
	    dist[closestVertex] = -1;
	    break;
	} else {
	    visited_nodes[num_visit++] = closestVertex;
	}
	for (i = 1; i < graph[closestVertex].nedges; i++) {
	    neighbor = graph[closestVertex].edges[i];
	    if (dist[neighbor] < -0.5) {	/* first time to reach neighbor */
		dist[neighbor] = closestDist + 1;
		enQueue(&Q, neighbor);
	    }
	}
    }
    freeQueue(&Q);

    /* set distances of all nodes in Queue to -1 */
    /* for next run */
    while (deQueue(&Q, &closestVertex)) {
	dist[closestVertex] = -1;
    }
    dist[vertex] = -1;
    return num_visit;
}

void mkQueue(Queue * qp, int size)
{
    qp->data = gv_calloc(size, sizeof(int));
    qp->queueSize = size;
    qp->start = qp->end = 0;
}

void freeQueue(Queue * qp)
{
    free(qp->data);
}

void initQueue(Queue * qp, int startVertex)
{
    qp->data[0] = startVertex;
    qp->start = 0;
    qp->end = 1;
}

bool deQueue(Queue * qp, int *vertex)
{
    if (qp->start >= qp->end)
	return false;		/* underflow */
    *vertex = qp->data[qp->start++];
    return true;
}

bool enQueue(Queue * qp, int vertex)
{
    if (qp->end >= qp->queueSize)
	return false;		/* overflow */
    qp->data[qp->end++] = vertex;
    return true;
}
