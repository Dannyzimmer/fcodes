/// \file
/// \brief test ordering of \p agmemread and \p gvContext
///
/// See https://gitlab.com/graphviz/graphviz/-/issues/2434.

#ifdef NDEBUG
#error "this program is not intended to be compiled with assertions disabled"
#endif

#include <assert.h>
#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static const char dotString[] = "\
digraph {\n\
    compound=true;\n\
    node [shape=Mrecord]\n\
    rankdir=\"LR\"\n\
\n\
    subgraph \"clusterOpen\"\n\
    {\n\
        label = \"Open\"\n\
        \"Assigned\" [label=\"Assigned|exit / OnDeassigned\"];\n\
    }\n\
    \"Deferred\" [label=\"Deferred|entry / Function\"];\n\
    \"Closed\" [label=\"Closed\"];\n\
\n\
    \"OpenNode\" -> \"Assigned\" [style=\"solid\", label=\"Assign / OnAssigned\"];\n\
    \"Assigned\" -> \"Assigned\" [style=\"solid\", label=\"Assign\"];\n\
    \"Assigned\" -> \"Closed\" [style=\"solid\", label=\"Close\"];\n\
    \"Assigned\" -> \"Deferred\" [style=\"solid\", label=\"Defer\"];\n\
    \"Deferred\" -> \"Assigned\" [style=\"solid\", label=\"Assign / OnAssigned\"];\n\
    init [label=\"\", shape=point];\n\
    init -> \"Open\"[style = \"solid\"]\n\
}\n\
";

int main(int argc, char **argv) {

  assert(argc == 2);
  assert(strcmp(argv[1], "before") == 0 || strcmp(argv[1], "after") == 0);

  GVC_t *gvc = NULL;
  if (strcmp(argv[1], "before") == 0) {
    gvc = gvContext();
    assert(gvc != NULL);
  }

  Agraph_t *graph = agmemread(dotString);
  assert(graph != NULL);

  if (strcmp(argv[1], "after") == 0) {
    assert(gvc == NULL);
    gvc = gvContext();
    assert(gvc != NULL);
  }

  {
    int rc = gvLayout(gvc, graph, "dot");
    assert(rc == 0);
  }

  {
    int rc = gvRender(gvc, graph, "svg", stdout);
    assert(rc == 0);
  }

  {
    int rc = gvFreeLayout(gvc, graph);
    assert(rc == 0);
  }

  {
    int rc = agclose(graph);
    assert(rc == 0);
  }

  return 0;
}
