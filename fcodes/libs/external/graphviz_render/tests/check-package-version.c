// test that the Graphviz version header has a non-empty PACKAGE_VERSION

// force assertions on
#ifdef NDEBUG
  #undef NDEBUG
#endif

#include <assert.h>
#include <graphviz/graphviz_version.h>
#include <stdlib.h>
#include <string.h>

#ifndef PACKAGE_VERSION
#error PACKAGE_VERSION missing from graphviz_version.h
#endif

int main(void) {
  assert(strcmp(PACKAGE_VERSION, "") != 0 &&
    "invalid PACKAGE_VERSION (" PACKAGE_VERSION ") in graphviz_version.h");
  return EXIT_SUCCESS;
}
