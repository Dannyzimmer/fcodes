#pragma once

#include <stddef.h>

/** retrieve a string representation of a lexer token
 *
 * \param id The numerical identifier of the sought token as an offset from
 *   MINTOKEN
 * \return The string name of the token or NULL if id is invalid
 */
const char *exop(size_t id);
