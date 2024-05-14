/// \file
/// \brief test case from Gitlab #2356 ported to C

#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>
#include <stdio.h>

static void GraphProc(void) {
  Agraph_t *g;
  Agnode_t *n, *m;
  Agedge_t *e;
  GVC_t *gvc;
  // set up a graphviz context
  gvc = gvContext();
  // Create a simple digraph
  Agdesc_t dir = {
      // graph descriptor
      1, // if edges are asymmetric
      1, // if multi-edges forbidden
      0, // if no loops
      1, // if this is the top level graph
      1, // if sets are flattened into lists in cdt
      0, // if a temporary subgraph
      1, // if string attr tables should be initialized
      1  // if may contain collapsed nodes
  };
  g = agopen("g", dir, 0);
  agattr(g, AGRAPH, "mindist", "0.3"); //!!!!!!the line causing the error
  n = agnode(g, "n", 1);
  m = agnode(g, "m", 1);
  e = agedge(g, n, m, 0, 1);
  gvLayout(gvc, g, "circo"); //!!!!!crashes here
  // Free layout data
  gvFreeLayout(gvc, g);
  // Free graph structures
  agclose(g);
  // close output file, free context, and return number of errors
  gvFreeContext(gvc);
}

int main(void) {
  for (int i = 0; i < 1000; i++) {
    GraphProc();
    printf("i = %d\n", i);
  }
  return 0;
}
