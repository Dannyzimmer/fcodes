/// \file
/// \brief test case that \p gvplugin_list returns full plugin names
///
/// See https://gitlab.com/graphviz/graphviz/-/issues/2375.

#include <assert.h>
#include <graphviz/gvc.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

// this is technically not an API function, but the macOS Graphviz.app calls it
extern char *gvplugin_list(GVC_t *gvc, api_t api, const char *str);

int main(void) {

  // setup a Graphviz context
  GVC_t *gvc = gvContext();

  // ask what plugins we have
  char *plugins = gvplugin_list(gvc, API_device, ":");

  // display the result for anyone debugging this test failing
  printf("gvplugin_list(…, \":\") returned: %s\n", plugins);

  // check we got full names, not :-truncated names
  assert(strchr(plugins, ':') != NULL && "plugin list not full names");

  return 0;
}
