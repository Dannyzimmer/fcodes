/// \file
/// \brief \p qsort with carried along context
///
/// The non-standard \p qsort_r, Windows’ \p qsort_s, and C11’s \p qsort_s
/// provide a \p qsort alternative with an extra state parameter. Unfortunately
/// none of these are easily usable portably in Graphviz. This header implements
/// an alternative that hopefully is.

#pragma once

#include <assert.h>
#include <stdlib.h>

/// thread-local storage specifier
#ifdef _MSC_VER
#define TLS __declspec(thread)
#elif defined(__GNUC__)
#define TLS __thread
#else
// assume this environment does not support threads and fall back to (thread
// unsafe) globals
#define TLS /* nothing */
#endif

static TLS int (*gv_sort_compar)(const void *, const void *, void *);
static TLS void *gv_sort_arg;

static inline int gv_sort_compar_wrapper(const void *a, const void *b) {
  assert(gv_sort_compar != NULL && "no comparator set in gv_sort");
  return gv_sort_compar(a, b, gv_sort_arg);
}

/// \p qsort with an extra state parameter, ala \p qsort_r
static inline void gv_sort(void *base, size_t nmemb, size_t size,
                           int (*compar)(const void *, const void *, void *),
                           void *arg) {
  assert(gv_sort_compar == NULL && gv_sort_arg == NULL &&
         "unsupported recursive call to gv_sort");

  gv_sort_compar = compar;
  gv_sort_arg = arg;

  qsort(base, nmemb, size, gv_sort_compar_wrapper);

  gv_sort_compar = NULL;
  gv_sort_arg = NULL;
}
