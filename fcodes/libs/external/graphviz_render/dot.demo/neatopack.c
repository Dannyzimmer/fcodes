/// @file
/// @brief demonstrates usage of @ref aginit, @ref ccomps, @ref nodeInduce,
/// @ref pack_graph and @ref agdelete

/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <graphviz/gvc.h>
#include <graphviz/pack.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  GVC_t *gvc = gvContext();

  FILE *fp;
  if (argc > 1)
    fp = fopen(argv[1], "r");
  else
    fp = stdin;
  graph_t *g = agread(fp, NULL);

  aginit(g, AGRAPH, "Agraphinfo_t", sizeof(Agraphinfo_t), true);
  aginit(g, AGNODE, "Agnodeinfo_t", sizeof(Agnodeinfo_t), true);

  int ncc;
  graph_t **cc = ccomps(g, &ncc, NULL);

  for (int i = 0; i < ncc; i++) {
    graph_t *sg = cc[i];
    nodeInduce(sg);
    gvLayout(gvc, sg, "neato");
  }
  pack_graph(ncc, cc, g, 0);

  gvRender(gvc, g, "ps", stdout);

  for (int i = 0; i < ncc; i++) {
    graph_t *sg = cc[i];
    gvFreeLayout(gvc, sg);
    agdelete(g, sg);
  }
  free(cc);

  agclose(g);

  return gvFreeContext(gvc);
}
