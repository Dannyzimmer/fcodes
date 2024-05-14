/// \file
/// \brief construct a graph using some escaped characters in a string

#include <graphviz/cgraph.h>
#include <stdio.h>

int main(void) {
  Agraph_t *g;
  Agnode_t *n;
  g = agopen("testgraph", Agdirected, 0);
  n = agnode(g, "foo\\\"bar", 1);
  agwrite(g, stdout);
  return 0;
}
