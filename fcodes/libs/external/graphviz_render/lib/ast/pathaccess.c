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
 * AT&T Bell Laboratories
 *
 * return path to file a/b using : separated dirs
 * both a and b may be 0
 * path returned in path buffer
 */

#include <ast/ast.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

char *pathaccess(char *path, const char *dirs, const char *a, const char *b) {
    int m = 0;
    int sep = ':';
    struct stat st;

#ifdef EFF_ONLY_OK
    m |= EFF_ONLY_OK;
#endif
    do {
	dirs = pathcat(path, dirs, sep, a, b);
	pathcanon(path);
	if (!access(path, m)) {
	    if (stat(path, &st) || S_ISDIR(st.st_mode))
		continue;
	    return path;
	}
    } while (dirs);
    return (0);
}
