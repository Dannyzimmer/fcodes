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
 * standalone mini error implementation
 */

#include <ast/ast.h>
#include <ast/error.h>
#include <cgraph/exit.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

Error_info_t error_info;

void setErrorLine (int line) { error_info.line = line; }
void setErrorFileLine (char* src, int line) {
    error_info.file = src;
    error_info.line = line;
}
void setErrorId (char* id) { error_info.id = id; }
void setErrorErrors (int errors) { error_info.errors = errors; }
int  getErrorErrors (void) { return error_info.errors; }
void setTraceLevel (int i) { error_info.trace = i; }

void errorv(const char *id, int level, const char *s, va_list ap)
{
    int flags;

    if (level < error_info.trace) return;
    if (level < 0)
	flags = 0;
    else {
	flags = level & ~ERROR_LEVEL;
	level &= ERROR_LEVEL;
    }
    const char *prefix;
    if (level && ((prefix = error_info.id) || (prefix = id))) {
	if (flags & ERROR_USAGE)
	    fprintf(stderr, "Usage: %s ", prefix);
	else
	    fprintf(stderr, "%s: ", prefix);
    }
    if (flags & ERROR_USAGE)
	/*nop */ ;
    else if (level < 0) {
	int i;
	for (i = 0; i < error_info.indent; i++)
	    fprintf(stderr, "  ");
	fprintf(stderr, "debug%d: ", level);
    } else if (level) {
	if (level == ERROR_WARNING) {
	    fprintf(stderr, "warning: ");
	    error_info.warnings++;
	} else {
	    error_info.errors++;
	    if (level == ERROR_PANIC)
		fprintf(stderr, "panic: ");
	}
	if (error_info.line) {
	    if (error_info.file && *error_info.file)
		fprintf(stderr, "\"%s\", ", error_info.file);
	    fprintf(stderr, "line %d: ", error_info.line);
	}
    }
    vfprintf(stderr, s, ap);
    if (flags & ERROR_SYSTEM)
	fprintf(stderr, "\n%s", strerror(errno));
    fprintf(stderr, "\n");
    if (level >= ERROR_FATAL)
	graphviz_exit(level - ERROR_FATAL + 1);
}

void error(int level, const char *s, ...)
{
    va_list ap;

    va_start(ap, s);
    errorv(NULL, level, s, ap);
    va_end(ap);
}

void errorf(void *handle, void *discipline, int level, const char *s, ...)
{
    va_list ap;

    va_start(ap, s);
    errorv((discipline
	    && handle) ? *((char **) handle) : (char *) handle, level, s, ap);
    va_end(ap);
}
