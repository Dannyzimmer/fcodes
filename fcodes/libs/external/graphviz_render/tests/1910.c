/* test case for repeated use of agmemread()
 * (see test_regression.py:test_1910())
 */

#include <assert.h>
#include <graphviz/cgraph.h>

#ifdef NDEBUG
#error "this code is not intended to be compiled with assertions disabled"
#endif

int main(void) {

  /* we should be able to parse a trivial graph successfully twice */
  assert(agmemread("digraph { a -> b }"));
  assert(agmemread("digraph { a -> b }"));

  return 0;
}
