/// \file
/// \brief basic unit tester for tokenize.h

#ifdef NDEBUG
#error this is not intended to be compiled with assertions off
#endif

#include <assert.h>
#include <cgraph/strview.h>
#include <cgraph/tokenize.h>
#include <stdio.h>
#include <stdlib.h>

/// basic lifecycle
static void test_basic(void) {
  const char input[] = "hello world";
  tok_t t = tok(input, " ");

  assert(!tok_end(&t));
  strview_t hello = tok_get(&t);
  assert(strview_str_eq(hello, "hello"));
  tok_next(&t);

  assert(!tok_end(&t));
  strview_t world = tok_get(&t);
  assert(strview_str_eq(world, "world"));
  tok_next(&t);

  assert(tok_end(&t));
}

/// create, but do not use a tokenizer in case ASan can detect leaking memory
static void test_no_usage(void) {
  const char input[] = "hello world";
  tok_t t = tok(input, " ");

  // squash compiler warnings for `t` being unused
  (void)t;
}

/// an example with multiple tokens
static void test_multiple(void) {
  const char input[] = "foo/bar/baz";
  tok_t t = tok(input, "/");

  assert(!tok_end(&t));
  strview_t foo = tok_get(&t);
  assert(strview_str_eq(foo, "foo"));
  tok_next(&t);

  assert(!tok_end(&t));
  strview_t bar = tok_get(&t);
  assert(strview_str_eq(bar, "bar"));
  tok_next(&t);

  assert(!tok_end(&t));
  strview_t baz = tok_get(&t);
  assert(strview_str_eq(baz, "baz"));
  tok_next(&t);

  assert(tok_end(&t));
}

/// input that starts with a separator
static void test_leading(void) {
  const char input[] = "/foo/bar";
  tok_t t = tok(input, "/");

  assert(!tok_end(&t));
  strview_t empty = tok_get(&t);
  assert(strview_str_eq(empty, ""));
  tok_next(&t);

  assert(!tok_end(&t));
  strview_t foo = tok_get(&t);
  assert(strview_str_eq(foo, "foo"));
  tok_next(&t);

  assert(!tok_end(&t));
  strview_t bar = tok_get(&t);
  assert(strview_str_eq(bar, "bar"));
  tok_next(&t);

  assert(tok_end(&t));
}

/// input that ends with a separator
static void test_trailing(void) {
  const char input[] = "foo/bar/";
  tok_t t = tok(input, "/");

  assert(!tok_end(&t));
  strview_t foo = tok_get(&t);
  assert(strview_str_eq(foo, "foo"));
  tok_next(&t);

  assert(!tok_end(&t));
  strview_t bar = tok_get(&t);
  assert(strview_str_eq(bar, "bar"));
  tok_next(&t);

  assert(!tok_end(&t));
  strview_t empty = tok_get(&t);
  assert(strview_str_eq(empty, ""));
  tok_next(&t);

  assert(tok_end(&t));
}

/// check multiple separators are coalesced into one
static void test_coalesce(void) {
  const char input[] = "foo//bar";
  tok_t t = tok(input, "/");

  assert(!tok_end(&t));
  strview_t foo = tok_get(&t);
  assert(strview_str_eq(foo, "foo"));
  tok_next(&t);

  assert(!tok_end(&t));
  strview_t bar = tok_get(&t);
  assert(strview_str_eq(bar, "bar"));
  tok_next(&t);

  assert(tok_end(&t));
}

/// tokenizing the empty string should produce one token
static void test_empty(void) {
  const char input[] = "";
  tok_t t = tok(input, "/");

  assert(!tok_end(&t));
  strview_t empty = tok_get(&t);
  assert(strview_str_eq(empty, ""));
  tok_next(&t);

  assert(tok_end(&t));
}

/// multiple different separators
static void test_multiple_separators(void) {
  const char input[] = "foo/bar:baz:/:qux";
  tok_t t = tok(input, "/:");

  assert(!tok_end(&t));
  strview_t foo = tok_get(&t);
  assert(strview_str_eq(foo, "foo"));
  tok_next(&t);

  assert(!tok_end(&t));
  strview_t bar = tok_get(&t);
  assert(strview_str_eq(bar, "bar"));
  tok_next(&t);

  assert(!tok_end(&t));
  strview_t baz = tok_get(&t);
  assert(strview_str_eq(baz, "baz"));
  tok_next(&t);

  assert(!tok_end(&t));
  strview_t qux = tok_get(&t);
  assert(strview_str_eq(qux, "qux"));
  tok_next(&t);

  assert(tok_end(&t));
}

/// input that consists solely of separators
static void test_only_separators(void) {
  const char input[] = "//:g/";
  tok_t t = tok(input, "/:g");

  assert(!tok_end(&t));
  strview_t empty = tok_get(&t);
  assert(strview_str_eq(empty, ""));
  tok_next(&t);

  assert(!tok_end(&t));
  strview_t empty2 = tok_get(&t);
  assert(strview_str_eq(empty2, ""));
  tok_next(&t);

  assert(tok_end(&t));
}

int main(void) {

#define RUN(t)                                                                 \
  do {                                                                         \
    printf("running test_%s... ", #t);                                         \
    fflush(stdout);                                                            \
    test_##t();                                                                \
    printf("OK\n");                                                            \
  } while (0)

  RUN(basic);
  RUN(no_usage);
  RUN(multiple);
  RUN(leading);
  RUN(trailing);
  RUN(coalesce);
  RUN(empty);
  RUN(multiple_separators);
  RUN(only_separators);

#undef RUN

  return EXIT_SUCCESS;
}
