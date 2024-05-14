/// @file
/// @brief demonstrates usage of @ref gvContext, @ref agread, @ref gvLayout,
/// @ref gvRender, @ref gvFreeLayout and @ref agclose

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
#include <stddef.h>
#include <stdio.h>

int main(int argc, char **argv) {

  GVC_t *gvc = gvContext();

  FILE *fp;
  if (argc > 1)
    fp = fopen(argv[1], "r");
  else
    fp = stdin;
  graph_t *g = agread(fp, NULL);

  gvLayout(gvc, g, "dot");

  gvRender(gvc, g, "plain", stdout);

  gvFreeLayout(gvc, g);

  agclose(g);

  return gvFreeContext(gvc);
}
