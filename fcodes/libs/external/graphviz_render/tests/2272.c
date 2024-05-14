/// \file
/// \brief test case for repeated use of agmemread() with unterminated strings
///
/// See test_regression.py:test_2272

#include <assert.h>
#include <graphviz/cgraph.h>

#ifdef NDEBUG
#error "this code is not intended to be compiled with assertions disabled"
#endif

int main(void) {

  // parse a graph with an unterminated string
  Agraph_t *g = agmemread("graph { a[label=\"abc");

  // this should have failed
  assert(g == NULL && "unterminated string was not rejected");

  // try parsing this graph again
  g = agmemread("graph { a[label=\"abc");

  // this one should fail too
  assert(g == NULL);

  return 0;
}
