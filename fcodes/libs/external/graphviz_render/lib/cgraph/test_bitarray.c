// basic unit tester for bitarray.h

#ifdef NDEBUG
#error this is not intended to be compiled with assertions off
#endif

#include <assert.h>
#include <cgraph/bitarray.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// helper for basic construction and destruction, with nothing in-between
static void create_reset(size_t size) {
  bitarray_t b = bitarray_new(size);
  bitarray_reset(&b);
}

// basic creation of a small array
static void test_create_reset_small(void) { create_reset(10); }

// basic creation of a large array
static void test_create_reset_large(void) { create_reset(1023); }

// setting and unsetting of all bits
static void set_unset(size_t size) {

  bitarray_t b = bitarray_new(size);

  // set and unset each bit
  for (size_t i = 0; i < size; ++i) {
    for (size_t j = 0; j < size; ++j)
      assert(!bitarray_get(b, j));
    bitarray_set(&b, i, true);
    for (size_t j = 0; j < size; ++j)
      assert(bitarray_get(b, j) == (i == j));
    bitarray_set(&b, i, false);
    for (size_t j = 0; j < size; ++j)
      assert(!bitarray_get(b, j));
  }

  // try the same in reverse
  for (size_t i = size - 1;; --i) {
    for (size_t j = 0; j < size; ++j)
      assert(!bitarray_get(b, j));
    bitarray_set(&b, i, true);
    for (size_t j = 0; j < size; ++j)
      assert(bitarray_get(b, j) == (i == j));
    bitarray_set(&b, i, false);
    for (size_t j = 0; j < size; ++j)
      assert(!bitarray_get(b, j));
    if (i == 0)
      break;
  }

  // a different, arbitrary order of a few bits
  static const size_t INDEX[] = {3, 1, 4, 42, 6, 7, 123};
  for (size_t i = 0; i < sizeof(INDEX) / sizeof(INDEX[0]); ++i) {
    if (INDEX[i] >= size)
      continue;
    for (size_t j = 0; j < size; ++j)
      assert(!bitarray_get(b, j));
    bitarray_set(&b, INDEX[i], true);
    for (size_t j = 0; j < size; ++j)
      assert(bitarray_get(b, j) == (INDEX[i] == j));
    bitarray_set(&b, INDEX[i], false);
    for (size_t j = 0; j < size; ++j)
      assert(!bitarray_get(b, j));
  }

  bitarray_reset(&b);
}

// set/unset for a small array
static void test_set_unset_small(void) { set_unset(10); }

// set/unset for a large array
static void test_set_unset_large(void) { set_unset(1023); }

// using an 8-bit aligned array
static void test_set_unset_aligned(void) {
  set_unset(8);
  set_unset(16);
  set_unset(64);
  set_unset(1024);
  set_unset(4096);
  set_unset(8192);
}

// reusing an array
static void test_reuse(void) {

  size_t size = 10;

  bitarray_t b = bitarray_new(size);

  // set and unset each bit
  for (size_t i = 0; i < size; ++i) {
    for (size_t j = 0; j < size; ++j)
      assert(!bitarray_get(b, j));
    bitarray_set(&b, i, true);
    for (size_t j = 0; j < size; ++j)
      assert(bitarray_get(b, j) == (i == j));
    bitarray_set(&b, i, false);
    for (size_t j = 0; j < size; ++j)
      assert(!bitarray_get(b, j));
  }

  bitarray_reset(&b);

  // reuse it with a different size
  size = 1023;
  b = bitarray_new(size);

  // set and unset each bit
  for (size_t i = 0; i < size; ++i) {
    for (size_t j = 0; j < size; ++j)
      assert(!bitarray_get(b, j));
    bitarray_set(&b, i, true);
    for (size_t j = 0; j < size; ++j)
      assert(bitarray_get(b, j) == (i == j));
    bitarray_set(&b, i, false);
    for (size_t j = 0; j < size; ++j)
      assert(!bitarray_get(b, j));
  }

  bitarray_reset(&b);
}

// redundant write to a bit
static void double_set(size_t size, bool value) {

  bitarray_t b = bitarray_new(size);

  static const size_t index = 7;
  assert(!bitarray_get(b, index));
  bitarray_set(&b, index, value);
  assert(bitarray_get(b, index) == value);
  bitarray_set(&b, index, value);
  assert(bitarray_get(b, index) == value);

  bitarray_reset(&b);
}

// various versions of the above
static void test_double_set_small(void) { double_set(10, true); }
static void test_double_set_large(void) { double_set(1023, true); }
static void test_double_clear_small(void) { double_set(10, false); }
static void test_double_clear_large(void) { double_set(1023, false); }

int main(void) {

#define RUN(t)                                                                 \
  do {                                                                         \
    printf("running test_%s... ", #t);                                         \
    fflush(stdout);                                                            \
    test_##t();                                                                \
    printf("OK\n");                                                            \
  } while (0)

  RUN(create_reset_small);
  RUN(create_reset_large);
  RUN(set_unset_small);
  RUN(set_unset_large);
  RUN(set_unset_aligned);
  RUN(reuse);
  RUN(double_set_small);
  RUN(double_set_large);
  RUN(double_clear_small);
  RUN(double_clear_large);

#undef RUN

  return EXIT_SUCCESS;
}
