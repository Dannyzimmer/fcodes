#pragma once

#include <cgraph/exit.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline FILE *openFile(const char *argv0, const char *name,
                             const char *mode) {
  FILE *fp = fopen(name, mode);
  if (fp == NULL) {
    const char *modestr = strcmp(mode, "r") == 0 ? "reading" : "writing";
    fprintf(stderr, "%s: could not open file %s for %s\n", argv0, name,
            modestr);
    perror(name);
    graphviz_exit(EXIT_FAILURE);
  }
  return fp;
}
