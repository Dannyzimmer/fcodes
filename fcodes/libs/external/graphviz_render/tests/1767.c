/* test case for repeated use of Pango plugin (see
 * test_regression.py:test_1767())
 */

#include <assert.h>
#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>
#include <stdio.h>
#include <stdlib.h>

static void load(const char *filename) {
  Agraph_t *g;
  GVC_t *gvc;
  FILE *file;
  int r;
  Agraph_t *sg;

  gvc = gvContext();
  assert(gvc != NULL);

  file = fopen(filename, "r");
  assert(file != NULL);

  g = agread(file, NULL);
  printf("Loaded graph:%s\n", agnameof(g));

  r = gvLayout(gvc, g, "dot");
  assert(r == 0);

  for (sg = agfstsubg(g); sg != NULL; sg = agnxtsubg(sg)) {
    printf("%s contains %d nodes\n", agnameof(sg), (int)agnnodes(sg));
  }

  gvFreeLayout(gvc, g);
  agclose(g);
  fclose(file);
  gvFreeContext(gvc);
}

int main(int argc, char **argv) {

  assert(argc == 2);

  load(argv[1]);
  load(argv[1]);

  return 0;
}
