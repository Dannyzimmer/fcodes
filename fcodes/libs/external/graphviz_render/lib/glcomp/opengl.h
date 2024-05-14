// shim to deal with OpenGL headers being at a different path on macOS

#pragma once

// OpenGL headers on Windows use the `WINGDIAPI` macro
#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __APPLE__

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

// Apple considers OpenGL deprecated. So silence Clang’s warnings about our
// use of it.
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
