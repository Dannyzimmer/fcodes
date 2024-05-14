// utility for translating xdot to JSON via the xdot API

#include <errno.h>
#include <graphviz/xdot.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {

  if (argc > 3 || (argc > 1 && strcmp(argv[1], "--help") == 0)) {
    fprintf(stderr, "usage: %s [input_file [output_file]]\n", argv[0]);
    return EXIT_FAILURE;
  }

  // setup input source
  FILE *input = stdin;
  if (argc > 1) {
    input = fopen(argv[1], "r");
    if (input == NULL) {
      fprintf(stderr, "failed to open %s: %s", argv[1], strerror(errno));
      return EXIT_FAILURE;
    }
  }

  // there is no API for parsing from a file handle, so read the entire file
  // into a string buffer
  char *in = NULL;
  size_t off = 0;
  size_t sz = 0;
  do {
    char buf[BUFSIZ];
    size_t r = fread(buf, 1, sizeof(buf), input);
    if (r == 0) {
      if (ferror(input)) {
        fprintf(stderr, "failed while reading %s\n", argv[1]);
        free(in);
        fclose(input);
        return EXIT_FAILURE;
      }
    } else {
      // do we need to expand our buffer?
      if (sz < r || sz - r < off) {
        size_t s = sz == 0 ? BUFSIZ : 2 * sz;
        char *i = realloc(in, s);
        if (i == NULL) {
          fprintf(stderr, "out of memory\n");
          free(in);
          fclose(input);
          return EXIT_FAILURE;
        }
        in = i;
        sz = s;
      }
      memcpy(in + off, buf, r);
      off += r;
    }
  } while (!feof(input));

  // we no longer need the input file
  fclose(input);

  // NUL-terminate the read input
  if (off == sz) {
    char *i = realloc(in, sz + 1);
    if (i == NULL) {
      fprintf(stderr, "out of memory\n");
      free(in);
      return EXIT_FAILURE;
    }
    in = i;
    ++sz;
  }
  in[off] = '\0';

  // parse input into an xdot object
  xdot *x = parseXDot(in);
  free(in);
  if (x == NULL) {
    fprintf(stderr, "failed to parse input into xdot\n");
    return EXIT_FAILURE;
  }

  // setup output sink
  FILE *output = stdout;
  if (argc > 2) {
    output = fopen(argv[2], "w");
    if (output == NULL) {
      fprintf(stderr, "failed to open %s: %s", argv[2], strerror(errno));
      freeXDot(x);
      return EXIT_FAILURE;
    }
  }

  // turn that into JSON
  jsonXDot(output, x);

  // clean up
  freeXDot(x);
  fclose(output);

  return EXIT_SUCCESS;
}
