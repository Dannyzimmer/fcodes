// basic unit tester for stack.h

#ifdef NDEBUG
#error this is not intended to be compiled with assertions off
#endif

#include <assert.h>
#include <cgraph/stack.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// an stack should start in a known initial state
static void test_init(void) {
  gv_stack_t s = {0};
  assert(stack_is_empty(&s));
  assert(stack_size(&s) == 0);
}

// reset of an initialized stack should be OK and idempotent
static void test_init_reset(void) {
  gv_stack_t s = {0};
  stack_reset(&s);
  stack_reset(&s);
  stack_reset(&s);
}

// basic push then pop
static void test_push_one(void) {
  gv_stack_t s = {0};
  void *arbitrary = (void *)0x42;
  stack_push(&s, arbitrary);
  assert(stack_size(&s) == 1);
  void *top = stack_pop(&s);
  assert(top == arbitrary);
  assert(stack_is_empty(&s));
  stack_reset(&s);
}

static void push_then_pop(size_t count) {
  gv_stack_t s = {0};
  for (uintptr_t i = 0; i < (uintptr_t)count; ++i) {
    stack_push(&s, (void *)i);
    assert(stack_size(&s) == (size_t)i + 1);
  }
  for (uintptr_t i = (uintptr_t)count - 1;; --i) {
    assert(stack_size(&s) == (size_t)i + 1);
    void *p = stack_pop(&s);
    assert((uintptr_t)p == i);
    if (i == 0) {
      break;
    }
  }
  stack_reset(&s);
}

// push a series of items
static void test_push_then_pop_ten(void) { push_then_pop(10); }

// push enough to cause an expansion
static void test_push_then_pop_many(void) { push_then_pop(4096); }

// interleave some push and pop operations
static void test_push_pop_interleaved(void) {
  gv_stack_t s = {0};
  size_t size = 0;
  for (uintptr_t i = 0; i < 4096; ++i) {
    if (i % 3 == 1) {
      void *p = stack_pop(&s);
      assert((uintptr_t)p == i - 1);
      --size;
    } else {
      stack_push(&s, (void *)i);
      ++size;
    }
    assert(stack_size(&s) == size);
  }
  stack_reset(&s);
}

int main(void) {

#define RUN(t)                                                                 \
  do {                                                                         \
    printf("running test_%s... ", #t);                                         \
    fflush(stdout);                                                            \
    test_##t();                                                                \
    printf("OK\n");                                                            \
  } while (0)

  RUN(init);
  RUN(init_reset);
  RUN(push_one);
  RUN(push_then_pop_ten);
  RUN(push_then_pop_many);
  RUN(push_pop_interleaved);

#undef RUN

  return EXIT_SUCCESS;
}
