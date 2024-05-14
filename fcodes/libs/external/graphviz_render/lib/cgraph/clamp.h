/// \file
/// \brief Arithmetic helper functions

#pragma once

#include <assert.h>
#include <limits.h>

/** scale up or down a non-negative integer, clamping to \p [0, INT_MAX]
 *
 * \param original Value to scale
 * \param scale Scale factor to apply
 * \return Clamped result
 */
static int scale_clamp(int original, double scale) {
  assert(original >= 0);

  if (scale < 0) {
    return 0;
  }

  if (scale > 1 && original > INT_MAX / scale) {
    return INT_MAX;
  }

  return (int)(original * scale);
}
