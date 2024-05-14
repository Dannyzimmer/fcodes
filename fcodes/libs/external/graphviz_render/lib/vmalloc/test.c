// basic unit tester for vmalloc

#ifdef NDEBUG
#error this is not intended to be compiled with assertions off
#endif

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// include vmalloc and some of its internals directly so we can call them
#include <vmalloc/vmalloc.h>
#include <vmalloc/vmalloc.c>
#include <vmalloc/vmclear.c>
#include <vmalloc/vmclose.c>
#include <vmalloc/vmopen.c>
#include <vmalloc/vmstrdup.c>

// trivial lifecycle of a vmalloc
static void test_basic_lifecycle(void) {

  // create a new vmalloc region
  Vmalloc_t *v = vmopen();

  // this program does not allocate much, so vmopen() should not have failed
  assert(v != NULL);

  // close the region
  vmclose(v);
}

// vmclear should be fine to call repeatedly on an empty region
static void test_empty_vmclear(void) {

  // create a new vmalloc region
  Vmalloc_t *v = vmopen();
  assert(v != NULL);

  // we should be able to clear this repeatedly with no effect
  vmclear(v);
  assert(v->size == 0);
  vmclear(v);
  assert(v->size == 0);
  vmclear(v);
  assert(v->size == 0);

  // close the region
  vmclose(v);
}

// a more realistic lifecycle usage of a vmalloc
static void test_lifecycle(void) {

  // some arbitrary sizes we will allocate
  static const size_t allocations[] = {123,  1,    42,   2,    3,
                                       4096, 1024, 1023, 20000};

  // length of the above array
  enum { allocations_len = sizeof(allocations) / sizeof(allocations[0]) };

  // somewhere to store pointers to memory we allocate
  unsigned char *p[allocations_len];

  // create a new vmalloc region
  Vmalloc_t *v = vmopen();
  assert(v != NULL);

  for (size_t i = 0; i < allocations_len; ++i) {

    // create the allocation
    p[i] = vmalloc(v, allocations[i]);
    assert(p[i] != NULL);

    // confirm we can write to it without faulting
    for (size_t j = 0; j < allocations[i]; ++j) {
      volatile unsigned char *q = (volatile unsigned char *)&p[i][j];
      *q = (unsigned char)(j % UCHAR_MAX);
    }

    // confirm we can read back what we wrote
    for (size_t j = 0; j < allocations[i]; ++j) {
      volatile unsigned char *q = (volatile unsigned char *)&p[i][j];
      assert(*q == (unsigned char)(j % UCHAR_MAX));
    }
  }

  // the allocator should have a correct account of what we have allocated
  assert(v->size == allocations_len);

  // free a few allocations out of order from that in which we allocated
  vmfree(v, p[3]);
  vmfree(v, p[6]);
  vmfree(v, p[5]);

  // the allocator should have correctly stopped tracking those pointers
  assert(v->size == allocations_len - 3);

  // free the rest of the allocations in one sweep
  vmclear(v);

  /* the allocator should have dropped all pointers it was tracking */
  assert(v->size == 0);

  // try the allocate-write-read test in reverse order
  for (size_t i = allocations_len - 1;; --i) {

    // create the allocation
    p[i] = vmalloc(v, allocations[i]);
    assert(p[i] != NULL);

    // confirm we can write to it without faulting
    for (size_t j = 0; j < allocations[i]; ++j) {
      volatile unsigned char *q = (volatile unsigned char *)&p[i][j];
      *q = (unsigned char)(j % UCHAR_MAX);
    }

    // confirm we can read back what we wrote
    for (size_t j = 0; j < allocations[i]; ++j) {
      volatile unsigned char *q = (volatile unsigned char *)&p[i][j];
      assert(*q == (unsigned char)(j % UCHAR_MAX));
    }

    if (i == 0) {
      break;
    }
  }

  // clean up
  vmclose(v);
}

// basic test of our strdup analogue
static void test_strdup(void) {

  // create a new vmalloc region
  Vmalloc_t *v = vmopen();
  assert(v != NULL);

  /* vmstrdup does not assume the source pointer came from its vmalloc, so lets
   * use a string in ordinary heap memory
   */
  char *s = strdup("hello world");
  assert(s != NULL);

  // ask vmalloc to strdup this
  char *t = vmstrdup(v, s);
  assert(t != NULL);

  // we should have received back the duplicated content
  assert(strcmp(s, t) == 0);

  // discard the original string
  free(s);

  // now ask vmalloc to strdup something it produced itself
  s = vmstrdup(v, t);
  assert(s != NULL);

  // we should still have the correct original string content
  assert(strcmp(s, t) == 0);

  // discard these strings
  vmfree(v, s);
  vmfree(v, t);

  /* vmstrdup does not assume the input string is mutable, so lets pass it a
   * non-writable string
   */
  const char sc[] = "foo bar";
  t = vmstrdup(v, sc);
  assert(t != NULL);

  // we should have received back the correct content
  assert(strcmp(sc, t) == 0);

  // clean up
  vmclose(v);
}

int main(void) {

#define RUN(t)                                                                 \
  do {                                                                         \
    printf("running test_%s... ", #t);                                         \
    fflush(stdout);                                                            \
    test_##t();                                                                \
    printf("OK\n");                                                            \
  } while (0)

  RUN(basic_lifecycle);
  RUN(empty_vmclear);
  RUN(lifecycle);
  RUN(strdup);

#undef RUN

  return EXIT_SUCCESS;
}
