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

#include <expr/exlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * allocate a new expression program environment
 */

Expr_t*
exopen(Exdisc_t* disc)
{
	Expr_t*	program;
	Exid_t*	sym;

	if (!(program = calloc(1, sizeof(Expr_t))))
		return 0;
	program->symdisc.key = offsetof(Exid_t, name);
	if (!(program->symbols = dtopen(&program->symdisc, Dtset)) ||
	    !(program->vm = vmopen()) ||
	    !(program->ve = vmopen()))
	{
		exclose(program, 1);
		return 0;
	}
	program->id = "libexpr:expr";
	program->disc = disc;
	setcontext(program);
	program->file[0] = stdin;
	program->file[1] = stdout;
	program->file[2] = stderr;
	strcpy(program->main.name, "main");
	program->main.lex = PROCEDURE;
	program->main.index = PROCEDURE;
	dtinsert(program->symbols, &program->main);
	if (!(disc->flags & EX_PURE))
		for (sym = exbuiltin; *sym->name; sym++)
			dtinsert(program->symbols, sym);
	if ((sym = disc->symbols))
		for (; *sym->name; sym++)
			dtinsert(program->symbols, sym);
	return program;
}
