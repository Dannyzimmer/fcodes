#pragma once

#define Y(i)    (1<<(i))

#define V       0x4      /* NODE         */
#define E       0x5      /* EDGE         */
#define G       0x6      /* GRAPH        */
#define O       0x7      /* OBJECT       */
#define TV      0x8      /* TV_          */
#define YALL    (Y(V)|Y(E)|Y(G))

enum {
  RESERVED,
#define X(prefix, name, str, type, subtype, ...) prefix##name,
#include <gvpr/gprdata.inc>
#undef X
};

enum { LAST_V = 0
#define X(prefix, name, str, type, subtype, ...) + (prefix == V_ ? 1 : 0)
#define V_ 1
#define M_ 2
#define T_ 3
#define A_ 4
#define F_ 5
#define C_ 6
#include <gvpr/gprdata.inc>
#undef X
};

enum { LAST_M = 0
#define X(prefix, name, str, type, subtype, ...) \
  + ((prefix == V_ || prefix == M_) ? 1 : 0)
#include <gvpr/gprdata.inc>
#undef C_
#undef F_
#undef A_
#undef T_
#undef M_
#undef V_
#undef X
};

enum { MINNAME = 1 };

enum { MAXNAME = 0
#define X(prefix, name, str, type, subtype, ...) + 1
#include <gvpr/gprdata.inc>
#undef X
};

static Exid_t symbols[] = {
#define X(prefix, name, str, type, subtype, ...) \
  EX_ID(str, type, prefix##name, subtype, 0),
#include <gvpr/gprdata.inc>
#undef X
  EX_ID({0}, 0, 0, 0, 0),
};

static char* typenames[] = {
#define X(prefix, name, str, type, subtype, ...) type ## _(str)
#define ID_(str) // nothing
#define DECLARE_(str) str,
#define ARRAY_(str) // nothing
#define FUNCTION_(str) // nothing
#define CONSTANT_(str) // nothing
#include <gvpr/gprdata.inc>
#undef CONSTANT_
#undef FUNCTION_
#undef ARRAY_
#undef DECLARE_
#undef ID_
#undef X
};

#ifdef DEBUG
static char* gprnames[] = {
  "",
#define X(prefix, name, str, type, subtype, ...) str,
#include <gvpr/gprdata.inc>
#undef X
};
#endif

typedef unsigned short tctype;

static tctype tchk[][2] = {
  {0, 0},
#define X(prefix, name, str, type, subtype, ...) type ## _(__VA_ARGS__)
#define ID_(...) { __VA_ARGS__ },
#define DECLARE_() // nothing
#define ARRAY_() // nothing
#define FUNCTION_() // nothing
#define CONSTANT_() // nothing
#include <gvpr/gprdata.inc>
#undef CONSTANT_
#undef FUNCTION_
#undef ARRAY_
#undef DECLARE_
#undef ID_
#undef X
};
