#include <assert.h>
#include <graphviz/cgraph.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {

  int ret = EXIT_SUCCESS;

#define ERR(args...)                                                           \
  do {                                                                         \
    fprintf(stderr, "%s:%d: ", __FILE__, __LINE__);                            \
    fprintf(stderr, args);                                                     \
    ret = EXIT_FAILURE;                                                        \
  } while (0)

  // create a new, empty graph
  Agraph_t *g = agopen("foo", Agdirected, NULL);
  assert(g != NULL);

  // create a new reference counted string in this graph’s dictionary
  char bar[] = "bar";
  char *ag_bar1 = agstrdup(g, bar);
  assert(ag_bar1 != NULL);

  // this should not be considered an HTML-like string
  if (aghtmlstr(ag_bar1))
    ERR("agstrdup result incorrectly considered HTML-like\n");

  // create an HTML-like string with the same content
  char *ag_bar2 = agstrdup_html(g, bar);
  assert(ag_bar2 != NULL);

  // this should be considered an HTML-like string
  if (!aghtmlstr(ag_bar2))
    ERR("agstrdup_html result incorrectly not considered HTML-like\n");

  // note whether ag_bar1 and ag_bar2 are actually the same pointer, so help
  // anyone debugging this test case
  printf("ag_bar1 == %p, ag_bar2 == %p\n", ag_bar1, ag_bar2);

  // attempt the same sequence of steps but in reverse to make sure the HTML bit
  // is not incorrectly shared in the other direction either

  char baz[] = "baz";
  char *ag_baz1 = agstrdup_html(g, baz);
  assert(ag_baz1 != NULL);

  if (!aghtmlstr(ag_baz1))
    ERR("agstrdup_html result incorrectly not considered HTML-like\n");

  char *ag_baz2 = agstrdup(g, baz);
  assert(ag_baz2 != NULL);

  if (aghtmlstr(ag_baz2))
    ERR("agstrdup result incorrectly considered HTML-like\n");

  printf("ag_baz1 == %p, ag_baz2 == %p\n", ag_baz1, ag_baz2);

  agclose(g);

  return ret;
}
