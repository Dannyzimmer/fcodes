/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

/*
 * Glenn Fowler
 * AT&T Research
 *
 * expression library
 */

#include <assert.h>
#include <cgraph/exit.h>
#include <expr/exlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * library error handler
 */

static char *make_msg(const char *format, va_list ap) {

  // retrieve buffered message
  char buf[64];
  excontext(expr.program, buf, sizeof(buf));

  // how many bytes do we need to construct the message?
  size_t len = (size_t)snprintf(NULL, 0, "%s\n -- ", buf);
  {
    va_list ap2;
    va_copy(ap2, ap);
    int r = vsnprintf(NULL, 0, format, ap2);
    va_end(ap2);
    if (r < 0) {
      return strdup("malformed format");
    }
    len += (size_t)r + 1; // +1 for NUL
  }

  char *s = malloc(len);
  if (s == NULL) {
    return NULL;
  }

  int offset = snprintf(s, len, "%s\n -- ", buf);
  assert(offset > 0);
  vsnprintf(s + offset, len - (size_t)offset, format, ap);

  return s;
}

void
exerror(const char* format, ...)
{
	if (expr.program->disc->errorf && !expr.program->errors)
	{
		va_list	ap;

		expr.program->errors = 1;
		va_start(ap, format);
		char *s = make_msg(format, ap);
		va_end(ap);
		(*expr.program->disc->errorf)(expr.program, expr.program->disc,
		  (expr.program->disc->flags & EX_FATAL) ? 3 : 2, "%s",
		  s ? s : "out of space");
    free(s);
	}
	else if (expr.program->disc->flags & EX_FATAL)
		graphviz_exit(1);
}

void 
exwarn(const char *format, ...)
{
	if (expr.program->disc->errorf) {
		va_list ap;

		va_start(ap, format);
		char *s = make_msg(format, ap);
		va_end(ap);
		(*expr.program->disc->errorf) (expr.program, expr.program->disc,
				       ERROR_WARNING, "%s", s ? s : "out of space");
		free(s);
	}
}
