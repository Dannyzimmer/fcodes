/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Glenn Fowler
 * AT&T Research
 *
 * expression library definitions
 */

#ifndef _EXPR_H
#define _EXPR_H

#include <ast/ast.h>
#include <inttypes.h>

#include <expr/exparse.h>

#undef	extern

#include <assert.h>
#include <cdt.h>
#include <cgraph/agxbuf.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <vmalloc/vmalloc.h>

#define EX_VERSION	20000101L

/*
 * flags
 */

#define EX_CHARSTRING	(1<<0)		/* '...' same as "..."		*/
#define EX_CONSTANT	(1<<1)		/* compile to constant expr	*/
#define EX_FATAL	(1<<2)		/* errors are fatal		*/
#define EX_INTERACTIVE	(1<<3)		/* interactive input		*/
#define EX_PURE		(1<<4)		/* no default symbols/keywords	*/
#define EX_QUALIFY	(1<<5)		/* '.' refs qualified in id tab	*/
#define EX_RETAIN	(1<<6)		/* retain expressions on redef	*/
#define EX_SIZED	(1<<7)		/* strings are sized buffers	*/
#define EX_STRICT	(1<<8)		/* don't override null label	*/
#define EX_UNDECLARED	(1<<9)		/* allow undeclared identifiers	*/

#define EX_ARRAY	(-3)		/* getval() array elt   */
#define EX_CALL		(-2)		/* getval() function call elt	*/
#define EX_SCALAR	(-1)		/* getval() scalar elt		*/

#define EX_NAMELEN	32		/* default Exid_t.name length	*/

#define EX_INTARRAY  1		/* integer-index array */

/* previously known as EXID, but EXID is also defined by bison in y.tab.h */
#define EX_ID(n,l,i,t,f)	{{0},(l),(i),(t),0,(f),0,{0},0,n}

#define DELETE_T		MINTOKEN		/* exexpr() delete `type'	*/

#define INTEGRAL(t)	((t)>=INTEGER&&(t)<=CHARACTER)
#define BUILTIN(t)  ((t) > MINTOKEN)

/* function type mechanism
 * types are encoded in TBITS
 * Thus, maximum # of parameters, including return type,
 * is sizeof(Exid_t.type)/TBITS. Also see T in exgram.h
 */

/*
 * arg 0 is the return value type
 */

#define F		01		/* FLOATING			*/
#define I		02		/* INTEGER			*/
#define S		03		/* STRING			*/

#define TBITS		4
#define TMASK		((1<<TBITS)-1)
#define A(n,t)		((t)<<((n)*TBITS))	/* function arg n is type t     */
#define N(t)		((t)>>=TBITS)	/* shift for next arg           */

#define exalloc(p,n)		vmalloc((p)->vm, (n))

typedef EX_STYPE Extype_t;

typedef union Exdata_u Exdata_t;
typedef struct Exdisc_s Exdisc_t;
typedef struct Exnode_s Exnode_t;
typedef struct Expr_s Expr_t;
typedef struct Exref_s Exref_t;

typedef int (*Exerror_f) (Expr_t *, Exdisc_t *, int, const char *, ...);
typedef void (*Exexit_f) (Expr_t *, Exdisc_t *, int);

typedef struct Exlocal_s		/* user defined member type	*/
{
	Sflong_t	number;
	char*		pointer;
} Exlocal_t;

typedef struct Exid_s			/* id symbol table info		*/
{
	Dtlink_t	link;		/* symbol table link		*/
	long		lex;		/* lex class			*/
	long		index;		/* user defined index		*/
	long		type;		/* symbol and arg types		*/
	long		index_type;	/* index type for arrays        */
	long		flags;		/* user defined flags		*/
	Exnode_t*	value;		/* value			*/
	Exlocal_t	local;		/* user defined local stuff	*/
	long		isstatic;	/* static			*/
	char		name[EX_NAMELEN];/* symbol name			*/
} Exid_t;

struct Exref_s				/* . reference list		*/
{
	Exref_t*	next;		/* next in list			*/
	Exid_t*		symbol;		/* reference id symbol		*/
	Exnode_t*	index;		/* optional reference index	*/
};

union Exdata_u
{

	struct
	{
	Extype_t	value;		/* constant value		*/
	Exid_t*		reference;	/* conversion reference symbol	*/
	}		constant;	/* variable reference		*/

	struct
	{
	Exnode_t*	left;		/* left operand			*/
	Exnode_t*	right;		/* right operand		*/
	Exnode_t*	last;		/* for cons			*/
	}		operand;	/* operands			*/

	struct
	{
	Exnode_t*	statement;	/* case label statement(s)	*/
	Exnode_t*	next;		/* next case item		*/
	Extype_t**	constant;	/* case label constant array	*/
	}		select;		/* case item			*/

	struct
	{
	Exid_t*		symbol;		/* id symbol table entry	*/
	Exref_t*	reference;	/* . reference list		*/
	Exnode_t*	index;		/* array index expression	*/
	Exnode_t*	dyna;		/* dynamic expression		*/
	}		variable;	/* variable reference		*/

#ifdef _EX_DATA_PRIVATE_
	_EX_DATA_PRIVATE_
#endif

};

struct Exnode_s				/* expression tree node		*/
{
	int	type;		/* value type			*/
	int	op;		/* operator			*/
	int	binary;		/* data.operand.{left,right} ok	*/
	Exlocal_t	local;		/* user defined local stuff	*/
	union
	{
	double	(*floating)(char**);	/* FLOATING return value	*/
	Sflong_t(*integer)(char**);	/* INTEGER|UNSIGNED return value*/
	char*	(*string)(char**);	/* STRING return value		*/
	}		compiled;	/* compiled function pointer	*/
	Exdata_t	data;		/* node data			*/

#ifdef _EX_NODE_PRIVATE_
	_EX_NODE_PRIVATE_
#endif

};

struct Exdisc_s				/* discipline			*/
{
	uint64_t	version;	/* EX_VERSION			*/
	uint64_t	flags;		/* EX_* flags			*/
	Exid_t*		symbols;	/* static symbols		*/
	char**		data;		/* compiled function arg data	*/
	char*		lib;		/* pathfind() lib		*/
	char*		type;		/* pathfind() type		*/
	int		(*castf)(Expr_t*, Exnode_t*, const char*, int, Exid_t*, int, Exdisc_t*);
					/* unknown cast function	*/
	int		(*convertf)(Exnode_t*, int, int);
					/* type conversion function	*/
	int		(*binaryf) (Exnode_t *, Exnode_t *, Exnode_t *, int);
					/* binary operator function     */
	char*		(*typename) (int);
					/* application type names       */
	int		(*stringof) (Expr_t *, Exnode_t *, int);
					/* value to string conversion   */
	Extype_t	(*keyf) (Extype_t, int);
					/* dictionary key for external type objects     */
	Exerror_f	errorf;		/* error function		*/
	Extype_t	(*getf)(Expr_t*, Exnode_t*, Exid_t*, Exref_t*, void*, int, Exdisc_t*);
					/* get value function		*/
	Extype_t	(*reff)(Expr_t*, Exnode_t*, Exid_t*, Exref_t*);
					/* reference value function	*/
	int		(*setf)(Expr_t*, Exnode_t*, Exid_t*, Exref_t*, void*, Extype_t);
					/* set value function		*/
	int		(*matchf)(const char*, const char*);
	/* exit function           */
	Exexit_f	exitf;
	int*		types;
	void*		user;
};

struct Expr_s				/* ex program state		*/
{
	const char*	id;		/* library id			*/
	Dt_t*		symbols;	/* symbol table			*/
	FILE*		file[10];	/* io streams			*/
	Vmalloc_t*	vm;		/* program store		*/

#ifdef _EX_PROG_PRIVATE_
	_EX_PROG_PRIVATE_
#endif

};

extern Exnode_t*	excast(Expr_t*, Exnode_t*, int, Exnode_t*, int);
extern Exnode_t*	exnoncast(Exnode_t *);
extern void		exclose(Expr_t*, int);

/** Compile an expression
 *
 * The callee takes ownership of the pointer \p prefix and will free it during
 * \p expop or \p exclose.
 *
 * \param p Program structure to store result in
 * \param name Filename of originating source file
 * \param line Line number of originating source file
 * \param fp Handle to source file
 * \param prefix Optional program text to include ahead of the file content
 * \return 0 on success
 */
extern int excomp(Expr_t *p, const char *name, int line, FILE *fp,
                  char *prefix);

extern char*		excontext(Expr_t*, char*, int);
extern int		exdump(Expr_t*, Exnode_t*, agxbuf*);
#ifdef __GNUC__
__attribute__((format(printf, 1, 2)))
#endif
extern void		exerror(const char*, ...);
#ifdef __GNUC__
__attribute__((format(printf, 1, 2)))
#endif
extern void		exwarn(const char *, ...);
extern Extype_t		exeval(Expr_t*, Exnode_t*, void*);
extern Exnode_t*	exexpr(Expr_t*, const char*, Exid_t*, int);
extern void		exfreenode(Expr_t*, Exnode_t*);
extern Exnode_t*	exnewnode(Expr_t*, int, int, int, Exnode_t*, Exnode_t*);
extern char*		exnospace(void);
extern Expr_t*		exopen(Exdisc_t*);
extern int		expop(Expr_t*);
extern int		expush(Expr_t*, const char*, int, FILE*);
extern int		extoken_fn(Expr_t*);
extern char*		exstring(Expr_t *, char *);
extern void*		exstralloc(Expr_t *, size_t);
extern char* extype(long int);
extern Extype_t exzero(long int);
extern char*	exopname(int);
extern void		exinit(void);
extern char*	extypename(Expr_t * p, int);
extern int		exisAssign(Exnode_t *);

#undef	extern

/** Construct a vmalloc-backed string.
 *
 * \param vm Allocator to use.
 * \param fmt printf-style format sting.
 * \param ... printf-style varargs.
 * \return Dynamically allocated string.
 */
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
static inline char *exprintf(Vmalloc_t *vm, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  // how many bytes do we need for this string?
  va_list ap2;
  va_copy(ap2, ap);
  int len = vsnprintf(NULL, 0, fmt, ap2);
  assert(len >= 0 && "invalid vsnprintf call");
  ++len; // for NUL terminator
  va_end(ap2);

  // ask vmalloc for enough space for this
  char *s = vmalloc(vm, (size_t)len);
  if (s == NULL) {
    va_end(ap);
    return exnospace();
  }

  // construct the output string
  (void)vsnprintf(s, (size_t)len, fmt, ap);
  va_end(ap);

  return s;
}

#endif

#ifdef __cplusplus
}
#endif
