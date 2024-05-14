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
 * expression library evaluator
 */

#include <cgraph/agxbuf.h>
#include <cgraph/strview.h>
#include <cgraph/exit.h>
#include <cgraph/unreachable.h>
#include <expr/exlib.h>
#include <expr/exop.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#ifdef _WIN32
#define srand48 srand
#define drand48 rand
#endif

#define TIME_LEN 80             /* max. characters to store time */

static Extype_t	eval(Expr_t*, Exnode_t*, void*);

#define TOTNAME		4
#define MAXNAME		23
#define FRAME		64

static const char*
lexname(int op, int subop)
{
	char*	b;

	static int	n;
	static char	buf[TOTNAME][MAXNAME];

	if (op > MINTOKEN && op < MAXTOKEN)
		return exop((size_t)op - MINTOKEN);
	if (++n >= TOTNAME)
		n = 0;
	b = buf[n];
	if (op == '=')
	{
		if (subop > MINTOKEN && subop < MAXTOKEN)
			snprintf(b, MAXNAME, "%s=", exop((size_t)subop - MINTOKEN));
		else if (subop > ' ' && subop <= '~')
			snprintf(b, MAXNAME, "%c=", subop);
		else
			snprintf(b, MAXNAME, "(%d)=", subop);
	}
	else if (subop < 0)
		snprintf(b, MAXNAME, "(EXTERNAL:%d)", op);
	else if (op > ' ' && op <= '~')
		snprintf(b, MAXNAME, "%c", op);
	else
		snprintf(b, MAXNAME, "(%d)", op);
	return b;
}

/* evaldyn:
 * Evaluate item from array given key.
 * Returns 1 if item existed, zero otherwise
 * 
 */
static int evaldyn(Expr_t *ex, Exnode_t *exnode, void *env, int delete) {
	Exassoc_t *b;
	Extype_t v;
	char buf[32];
	Extype_t key;
	char *keyname;

	v = eval(ex, exnode->data.variable.index, env);
	if (exnode->data.variable.symbol->index_type == INTEGER) {
		if (!(b = dtmatch((Dt_t *)exnode->data.variable.symbol->local.pointer, &v))) {
			return 0;
		}
	} 
	else {
		int type = exnode->data.variable.index->type;
		if (type != STRING) {
			if (!BUILTIN(type)) {
				key = ex->disc->keyf(v, type);
			} else
				key.integer = v.integer;
			snprintf(buf, sizeof(buf), "%llx", (unsigned long long)key.integer);
			keyname = buf;
		} else
			keyname = v.string;
		if (!(b = dtmatch((Dt_t *)exnode->data.variable.
			symbol->local.pointer, keyname))) {
			return 0;
		}
	}
	if (delete) {
		dtdelete((Dt_t *)exnode->data.variable.symbol->local.pointer, b);
		free (b);
	}
	return 1;
}

/*
 * return dynamic (associative array) variable value
 * assoc will point to the associative array bucket
 */
static Extype_t getdyn(Expr_t *ex, Exnode_t *exnode, void *env,
                       Exassoc_t **assoc) {
	Exassoc_t*	b;
	Extype_t	v;

	if (exnode->data.variable.index)
	{
		Extype_t key;
		char	buf[2*sizeof(key.integer)+1];  /* no. of hex chars needed plus null byte */
		char *keyname;

		v = eval(ex, exnode->data.variable.index, env);
		if (exnode->data.variable.symbol->index_type == INTEGER) {
			if (!(b = dtmatch((Dt_t *) exnode->data.variable.symbol->local.pointer, &v)))
			{
				if (!(b = calloc(1, sizeof(Exassoc_t))))
					exnospace();
				b->key = v;
				dtinsert((Dt_t *)exnode->data.variable.symbol->local.pointer, b);
			}
		} else {
			int type = exnode->data.variable.index->type;
			if (type != STRING) {
				if (!BUILTIN(type)) {
					key = ex->disc->keyf(v, type);
				} else
					key.integer = v.integer;
				snprintf(buf, sizeof(buf), "%llx", (unsigned long long)key.integer);
				keyname = buf;
			} else
				keyname = v.string;
			if (!(b = dtmatch((Dt_t *)exnode->data.variable.symbol->local.pointer, keyname)))
			{
				if (!(b = calloc(1, sizeof(Exassoc_t) + strlen(keyname))))
					exnospace();
				strcpy(b->name, keyname);
				b->key = v;
				dtinsert((Dt_t *)exnode->data.variable.symbol->local.pointer, b);
			}
		}
		*assoc = b;
		if (b)
		{
			if (exnode->data.variable.symbol->type == STRING && !b->value.string)
				b->value = exzero(exnode->data.variable.symbol->type);
			return b->value;
		}
		v = exzero(exnode->data.variable.symbol->type);
		return v;
	}
	*assoc = 0;
	return exnode->data.variable.symbol->value->data.constant.value;
}

typedef struct
{
	Sffmt_t		fmt;
	Expr_t*		expr;
	void*		env;
	Print_t*	args;
	Extype_t	value;
	Exnode_t*	actuals;
} Fmt_t;

/*
 * printf %! extension function
 */

static int
prformat(void* vp, Sffmt_t* dp)
{
	Fmt_t*		fmt = (Fmt_t*)dp;
	Exnode_t*	node;
	char*		s;
	int			from;
	int			to = 0;
	time_t			tm;
    struct tm *stm;

	dp->flags |= SFFMT_VALUE;
	if (fmt->args)
	{
		if ((node = dp->fmt == '*' ? fmt->args->param[dp->size] : fmt->args->arg))
			fmt->value = exeval(fmt->expr, node, fmt->env);
		else
			fmt->value.integer = 0;
		to = fmt->args->arg->type;
	}
	else if (!(fmt->actuals = fmt->actuals->data.operand.right))
		exerror("printf: not enough arguments");
	else
	{
		node = fmt->actuals->data.operand.left;
		from = node->type;
		switch (dp->fmt)
		{
		case 'f':
		case 'g':
			to = FLOATING;
			break;
		case 's':
			to = STRING;
			break;
		default:
			switch (from)
			{
			case INTEGER:
			case UNSIGNED:
				to = from;
				break;
			default:
				to = INTEGER;
				break;
			}
			break;
		}
		if (to == from)
			fmt->value = exeval(fmt->expr, node, fmt->env);
		else
		{
			node = excast(fmt->expr, node, to, NULL, 0);
			fmt->value = exeval(fmt->expr, node, fmt->env);
			node->data.operand.left = 0;
			vmfree(fmt->expr->vm, node);
			if (to == STRING)
			{
				if (fmt->value.string)
				{
					size_t n = strlen(fmt->value.string);
					if ((s = fmtbuf(n + 1)))
						memcpy(s, fmt->value.string, n + 1);
					vmfree(fmt->expr->vm, fmt->value.string);
					fmt->value.string = s;
				}
				if (!fmt->value.string)
					fmt->value.string = "";
			}
		}
	}
	switch (to)
	{
	case STRING:
		*(char**)vp = fmt->value.string;
		fmt->fmt.size = -1;
		break;
	case FLOATING:
		*(double*)vp = fmt->value.floating;
		fmt->fmt.size = sizeof(double);
		break;
	default:
		*(Sflong_t*)vp = fmt->value.integer;
		dp->size = sizeof(Sflong_t);
		break;
	}
	strview_t txt = {0};
	if (dp->n_str > 0)
	{
		txt = (strview_t){.data = dp->t_str, .size = (size_t)dp->n_str};
	}
	switch (dp->fmt)
	{
	case 'q':
	case 'Q':
		s = *(char**)vp;
		*(char**)vp = fmtquote(s, "$'", "'", strlen(s));
		dp->fmt = 's';
		dp->size = -1;
		break;
	case 'S':
		dp->flags &= ~SFFMT_LONG;
		s = *(char**)vp;
		if (txt.data != NULL)
		{
			if (strview_str_eq(txt, "identifier"))
			{
				if (*s && !isalpha((int)*s))
					*s++ = '_';
				for (; *s; s++)
					if (!isalnum((int)*s))
						*s = '_';
			}
			else if (strview_str_eq(txt, "invert"))
			{
				for (; *s; s++)
					if (isupper((int)*s))
						*s = (char)tolower(*s);
					else if (islower((int)*s))
						*s = (char)toupper(*s);
			}
			else if (strview_str_eq(txt, "lower"))
			{
				for (; *s; s++)
					if (isupper((int)*s))
						*s = (char)tolower(*s);
			}
			else if (strview_str_eq(txt, "upper"))
			{
				for (; *s; s++)
					if (islower((int)*s))
						*s = (char)toupper(*s);
			}
			else if (strview_str_eq(txt, "variable"))
			{
				for (; *s; s++)
					if (!isalnum((int)*s) && *s != '_')
						*s = '.';
			}
		}
		dp->fmt = 's';
		dp->size = -1;
		break;
	case 't':
	case 'T':
		if ((tm = *(Sflong_t*)vp) == -1)
			tm = time(NULL);
        if (txt.data == NULL) {
            exerror("printf: no time format provided");
        } else {
            s = fmtbuf(TIME_LEN);
            stm = localtime(&tm);
            char *format = malloc(sizeof(char) * (txt.size + 1));
            if (format == NULL) {
                exerror("printf: out of memory");
            } else {
                strncpy(format, txt.data, txt.size);
                format[txt.size] = '\0';
                strftime(s, TIME_LEN, format, stm);
                free(format);
                *(char **)vp = s;
            }
        }
		dp->fmt = 's';
		dp->size = -1;
		break;
	}
	return 0;
}

/*
 * print a list of strings
 */
static int prints(Expr_t *ex, Exnode_t *exnode, void *env, FILE *sp) {
    Extype_t v;
    Exnode_t *args;

    args = exnode->data.operand.left;
    while (args) {
	v = eval(ex, args->data.operand.left, env);
	fputs(v.string, sp);
	args = args->data.operand.right;
    }
    putc('\n', sp);
    return 0;
}

/*
 * do printf
 */
static int print(Expr_t *ex, Exnode_t *exnode, void *env, FILE *sp) {
	Print_t*	x;
	Extype_t		v;
	Fmt_t			fmt;

	if (!sp)
	{
		v = eval(ex, exnode->data.print.descriptor, env);
		if (v.integer < 0 || (long long unsigned)v.integer >= elementsof(ex->file) ||
		    (!(sp = ex->file[v.integer]) &&
		    !(sp = ex->file[v.integer] = tmpfile())))
		{
			exerror("printf: %" PRIdMAX ": invalid descriptor", (intmax_t)v.integer);
			return -1;
		}
	}
	memset(&fmt, 0, sizeof(fmt));
	fmt.fmt.extf = prformat;
	fmt.expr = ex;
	fmt.env = env;
	x = exnode->data.print.args;
	if (x->format)
		do
		{
			if (x->arg)
			{
				fmt.fmt.form = x->format;
				fmt.args = x;
				sfprint(sp, &fmt.fmt);
			}
			else
				fputs(x->format, sp);
		} while ((x = x->next));
	else
	{
		v = eval(ex, x->arg->data.operand.left, env);
		fmt.fmt.form = v.string;
		fmt.actuals = x->arg;
		sfprint(sp, &fmt.fmt);
		if (fmt.actuals->data.operand.right)
			exerror("(s)printf: \"%s\": too many arguments", fmt.fmt.form);
	}
	return 0;
}

/*
 * scanf %! extension function
 */

static int
scformat(void* vp, Sffmt_t* dp)
{
	Fmt_t*		fmt = (Fmt_t*)dp;
	Exnode_t*	node;

	if (!fmt->actuals)
	{
		exerror("scanf: not enough arguments");
		return -1;
	}
	node = fmt->actuals->data.operand.left;
	switch (dp->fmt)
	{
	case 'f':
	case 'g':
		if (node->type != FLOATING)
		{
			exerror("scanf: %s: floating variable address argument expected", node->data.variable.symbol->name);
			return -1;
		}
		fmt->fmt.size = sizeof(double);
		*(void**)vp = &node->data.variable.symbol->value->data.constant.value;
		break;
	case 's':
	case '[': {
		if (node->type != STRING)
		{
			exerror("scanf: %s: string variable address argument expected", node->data.variable.symbol->name);
			return -1;
		}
		if (node->data.variable.symbol->value->data.constant.value.string == expr.nullstring)
			node->data.variable.symbol->value->data.constant.value.string = 0;
		fmt->fmt.size = 1024;
		char *s = node->data.variable.symbol->value->data.constant.value.string;
		vmfree(fmt->expr->vm, s);
		s = vmalloc(fmt->expr->vm, sizeof(char) * (size_t)fmt->fmt.size);
		memset(s, 0, sizeof(char) * (size_t)fmt->fmt.size);
		*(void**)vp = s;
		node->data.variable.symbol->value->data.constant.value.string = s;
		break;
	}
	case 'c':
		if (node->type != CHARACTER) {
			exerror("scanf: %s: char variable address argument expected", node->data.variable.symbol->name);
			return -1;
		}
		fmt->fmt.size = sizeof(Sflong_t);
		*((void **) vp) = &node->data.variable.symbol->value->data.constant.value;
		break;
	default:
		if (node->type != INTEGER && node->type != UNSIGNED)
		{
			exerror("scanf: %s: integer variable address argument expected", node->data.variable.symbol->name);
			return -1;
		}
		dp->size = sizeof(Sflong_t);
		*(void**)vp = &node->data.variable.symbol->value->data.constant.value;
		break;
	}
	fmt->actuals = fmt->actuals->data.operand.right;
	dp->flags |= SFFMT_VALUE;
	return 0;
}

/*
 * do scanf
 */

static int scan(Expr_t *ex, Exnode_t *exnode, void *env, FILE *sp) {
	Extype_t		v;
	Extype_t		u;
	Fmt_t			fmt;
	int			n;

	if (!sp)
	{
		if (exnode->data.scan.descriptor)
		{
			v = eval(ex, exnode->data.scan.descriptor, env);
			if (exnode->data.scan.descriptor->type == STRING)
				goto get;
		}
		else
			v.integer = 0;
		if (v.integer < 0 || (size_t)v.integer >= elementsof(ex->file) || (!(sp = ex->file[v.integer]) && !(sp = ex->file[v.integer] = tmpfile())))
		{
			exerror("scanf: %" PRIdMAX ": invalid descriptor", (intmax_t)v.integer);
			return 0;
		}
	}
 get:
	memset(&fmt, 0, sizeof(fmt));
	fmt.fmt.extf = scformat;
	fmt.expr = ex;
	fmt.env = env;
	u = eval(ex, exnode->data.scan.format, env);
	fmt.fmt.form = u.string;
	fmt.actuals = exnode->data.scan.args;
	if (sp == NULL) {
		sp = tmpfile();
		if (sp == NULL) {
			exerror("scanf: failed to open temporary file");
			return 0;
		}
		fputs(v.string, sp);
		rewind(sp);
		n = sfscanf(sp, "%!", &fmt);
		fclose(sp);
	} else {
		n = sfscanf(sp, "%!", &fmt);
	}
	if (fmt.actuals && !*fmt.fmt.form)
		exerror("scanf: %s: too many arguments", fmt.actuals->data.operand.left->data.variable.symbol->name);
	return n;
}

/*
 * string add
 */

static char*
str_add(Expr_t* ex, char* l, char* r)
{
	size_t sz = strlen(l) + strlen(r) + 1;
	char *s = vmalloc(ex->ve, sz);
	if (s == NULL) {
		return exnospace();
	}
	snprintf(s, sz, "%s%s", l, r);
	return s;
}

/*
 * string ior
 */

static char *str_ior(Expr_t *ex, const char *l, const char *r) {

  // compute how much space the result will occupy
  size_t len = 1; // 1 for NUL terminator
  for (const char *p = l; *p != '\0'; ++p) {
    if (strchr(p + 1, *p) == NULL) {
      ++len;
    }
  }
  for (const char *p = r; *p != '\0'; ++p) {
    if (strchr(l, *p) == NULL && strchr(p + 1, *p) == NULL) {
      ++len;
    }
  }

  // allocate a buffer to store this
  char *result = vmalloc(ex->ve, len);
  if (result == NULL) {
    return exnospace();
  }

  // write the result
  size_t i = 0;
  for (const char *p = l; *p != '\0'; ++p) {
    if (strchr(p + 1, *p) == NULL) {
      assert(i < len && "incorrect preceding length computation");
      result[i] = *p;
      ++i;
    }
  }
  for (const char *p = r; *p != '\0'; ++p) {
    if (strchr(l, *p) == NULL && strchr(p + 1, *p) == NULL) {
      assert(i < len && "incorrect preceding length computation");
      result[i] = *p;
      ++i;
    }
  }
  assert(i + 1 == len && "incorrect preceding length computation");
  result[i] = '\0';

  return result;
}

/*
 * string and
 */

static char *str_and(Expr_t *ex, const char *l, const char *r) {

  // compute how much space the result will occupy
  size_t len = 1; // 1 for NUL terminator
  for (const char *p = l; *p != '\0'; ++p) {
    if (strchr(r, *p) != NULL && strchr(p + 1, *p) == NULL) {
      ++len;
    }
  }

  // allocate a buffer to store this
  char *result = vmalloc(ex->ve, len);
  if (result == NULL) {
    return exnospace();
  }

  // write the result
  size_t i = 0;
  for (const char *p = l; *p != '\0'; ++p) {
    if (strchr(r, *p) != NULL && strchr(p + 1, *p) == NULL) {
      assert(i < len && "incorrect preceding length computation");
      result[i] = *p;
      ++i;
    }
  }
  assert(i + 1 == len && "incorrect preceding length computation");
  result[i] = '\0';

  return result;
}

/*
 * string xor
 */

static char *str_xor(Expr_t *ex, const char *l, const char *r) {

  // compute how much space the result will occupy
  size_t len = 1; // 1 for NUL terminator
  for (const char *p = l; *p != '\0'; ++p) {
    if (strchr(r, *p) == NULL && strchr(p + 1, *p) == NULL) {
      ++len;
    }
  }
  for (const char *p = r; *p != '\0'; ++p) {
    if (strchr(l, *p) == NULL && strchr(p + 1, *p) == NULL) {
      ++len;
    }
  }

  // allocate a buffer to store this
  char *result = vmalloc(ex->ve, len);
  if (result == NULL) {
    return exnospace();
  }

  // write the result
  size_t i = 0;
  for (const char *p = l; *p != '\0'; ++p) {
    if (strchr(r, *p) == NULL && strchr(p + 1, *p) == NULL) {
      assert(i < len && "incorrect preceding length computation");
      result[i] = *p;
      ++i;
    }
  }
  for (const char *p = r; *p != '\0'; ++p) {
    if (strchr(l, *p) == NULL && strchr(p + 1, *p) == NULL) {
      assert(i < len && "incorrect preceding length computation");
      result[i] = *p;
      ++i;
    }
  }
  assert(i + 1 == len && "incorrect preceding length computation");
  result[i] = '\0';

  return result;
}

/*
 * string mod
 */

static char *str_mod(Expr_t *ex, const char *l, const char *r) {

  // compute how much space the result will occupy
  size_t len = 1; // 1 for NUL terminator
  for (const char *p = l; *p != '\0'; ++p) {
    if (strchr(r, *p) == NULL && strchr(p + 1, *p) == NULL) {
      ++len;
    }
  }

  // allocate a buffer to store this
  char *result = vmalloc(ex->ve, len);
  if (result == NULL) {
    return exnospace();
  }

  // write the result
  size_t i = 0;
  for (const char *p = l; *p != '\0'; ++p) {
    if (strchr(r, *p) == NULL && strchr(p + 1, *p) == NULL) {
      assert(i < len && "incorrect preceding length computation");
      result[i] = *p;
      ++i;
    }
  }
  assert(i + 1 == len && "incorrect preceding length computation");
  result[i] = '\0';

  return result;
}

/*
 * string mpy
 */

static char *str_mpy(Expr_t *ex, const char *l, const char *r) {

  // compute how much space the result will occupy
  size_t len = strlen(l);
  {
    size_t len2 = strlen(r);
    if (len2 < len) {
      len = len2;
    }
  }
  ++len; // 1 for NUL terminator

  // allocate a buffer to store this
  char *result = vmalloc(ex->ve, len);
  if (result == NULL) {
    return exnospace();
  }

  // write the result
  size_t i = 0;
  for (; l[i] != '\0' && r[i] != '\0'; ++i) {
    assert(i < len && "incorrect preceding length computation");
    result[i] = l[i] == r[i] ? l[i] : ' ';
  }
  assert(i + 1 == len && "incorrect preceding length computation");
  result[i] = '\0';

  return result;
}

/* replace:
 * Add replacement string.
 * \digit is replaced with a subgroup match, if any.
 */
static void replace(agxbuf *s, char *base, char *repl, int ng, int *sub) {
  char c;
  int idx, offset;

  while ((c = *repl++)) {
    if (c == '\\') {
      if ((c = *repl) && isdigit(c)) {
        idx = c - '0';
        if (idx < ng) {
          offset = sub[2 * idx];
          agxbput_n(s, base + offset, (size_t)(sub[2 * idx + 1] - offset));
        }
        repl++;
      } else {
        agxbputc(s, '\\');
      }
    } else {
      agxbputc(s, c);
    }
  }
}

#define MCNT(s) (sizeof(s)/(2*sizeof(int)))

static void
addItem (Dt_t* arr, Extype_t v, char* tok)
{
	Exassoc_t* b;

	if (!(b = dtmatch(arr, &v))) {
		if (!(b = calloc(1, sizeof(Exassoc_t))))
	    	exerror("out of space [assoc]");
		b->key = v;
		dtinsert(arr, b);
	}
	b->value.string = tok;
}

/* exsplit:
 * break string into possibly empty fields and store in array
 * return number of fields
 */
static Extype_t exsplit(Expr_t *ex, Exnode_t *exnode, void *env) {
	Extype_t v;
	char *str;
	char *seps;
	char *tok;
	size_t sz;
	Dt_t* arr = (Dt_t*)exnode->data.split.array->local.pointer;

	str = eval(ex, exnode->data.split.string, env).string;
	if (exnode->data.split.seps)
		seps = eval(ex, exnode->data.split.seps, env).string;
	else
		seps = " \t\n";

	v.integer = 0;
	while (*str) {
		sz = strspn (str, seps);
	    if (sz) {
			if (v.integer == 0) {  /* initial separator => empty field */
	    		addItem (arr, v, "");
	    		v.integer++;
			}
			for (size_t i = 1; i < sz; i++) {
	    		addItem (arr, v, "");
	    		v.integer++;
			}
		}
		str += sz;
		if (*str == '\0') { /* terminal separator => empty field */
			addItem (arr, v, "");
			v.integer++;
	    	break;
		}
		sz = strcspn (str, seps);
		tok = vmalloc(ex->vm, sz + 1);
		if (tok == NULL) {
			tok = exnospace();
		} else {
			memcpy(tok, str, sz);
			tok[sz] = '\0';
		}
		addItem (arr, v, tok);
		v.integer++;
		str += sz;
	}

	return v;
}

/* extoken:
 * tokenize string and store in array
 * return number of tokens
 */
static Extype_t extokens(Expr_t *ex, Exnode_t *exnode, void *env) {
	Extype_t v;
	char *str;
	char *seps;
	char *tok;
	size_t sz;
	Dt_t* arr = (Dt_t*)exnode->data.split.array->local.pointer;

	str = eval(ex, exnode->data.split.string, env).string;
	if (exnode->data.split.seps)
		seps = eval(ex, exnode->data.split.seps, env).string;
	else
		seps = " \t\n";

	v.integer = 0;
	while (*str) {
		sz = strspn (str, seps);
		str += sz;
		if (*str == '\0')
	    	break;

		sz = strcspn (str, seps);
		assert (sz);
		tok = vmalloc(ex->vm, sz + 1);
		if (tok == NULL) {
			tok = exnospace();
		} else {
			memcpy(tok, str, sz);
			tok[sz] = '\0';
		}
		addItem (arr, v, tok);
		v.integer++;
		str += sz;
	}

	return v;
}

/* exsub:
 * return string after pattern substitution
 */
static Extype_t exsub(Expr_t *ex, Exnode_t *exnode, void *env, bool global) {
	char *str;
	char *pat;
	char *repl;
	char *p;
	char *s;
	Extype_t v;
	int sub[20];
	int flags = STR_MAXIMAL;
	int ng;

	str = eval(ex, exnode->data.string.base, env).string;
	pat = eval(ex, exnode->data.string.pat, env).string;
	if (exnode->data.string.repl)
		repl = eval(ex, exnode->data.string.repl, env).string;
	else
		repl = 0;

	if (!global) {
		if (*pat == '^') {
		    pat++;
		    flags |= STR_LEFT;
		}
		p = pat;
		while (*p)
		    p++;
		if (p > pat)
		    p--;
		if (*p == '$') {
		    if (p > pat && p[-1] == '\\') {
				*p-- = '\0';
				*p = '$';
	    	} else {
				flags |= STR_RIGHT;
				*p = '\0';
	    	}
		}
	}
	if (*pat == '\0') {
		v.string = vmstrdup(ex->ve, str);
		return v;
	}

	ng = strgrpmatch(str, pat, sub, MCNT(sub), flags);
	if (ng == 0) {
		v.string = vmstrdup(ex->ve, str);
		return v;
	}
    if (sub[0] == sub[1]) {
		exwarn("pattern match of empty string - ill-specified pattern \"%s\"?", pat);
		v.string = vmstrdup(ex->ve, str);
		return v;
    } 

	agxbuf buffer = {0};

	agxbput_n(&buffer, str, (size_t)sub[0]);

	if (repl) {
		replace(&buffer, str, repl, ng, sub);
	}

	s = str + sub[1];
	if (global) {
		while ((ng = strgrpmatch(s, pat, sub, MCNT(sub), flags))) {
			agxbput_n(&buffer, s, (size_t)sub[0]);
			if (repl) {
				replace(&buffer, s, repl, ng, sub);
			}
			s = s + sub[1];
		}
	}

	agxbput(&buffer, s);

	v.string = vmstrdup(ex->ve, agxbuse(&buffer));
	agxbfree(&buffer);
	return v;
}

/* exsubstr:
 * return substring.
 */
static Extype_t exsubstr(Expr_t *ex, Exnode_t *exnode, void *env) {
	Extype_t s;
	Extype_t i;
	Extype_t l;
	Extype_t v;
	int len;

	s = eval(ex, exnode->data.string.base, env);
	len = strlen(s.string);
	i = eval(ex, exnode->data.string.pat, env);
	if (i.integer < 0 || len < i.integer)
		exerror("illegal start index in substr(%s,%" PRIdMAX ")", s.string,
		        (intmax_t)i.integer);
	if (exnode->data.string.repl) {
		l = eval(ex, exnode->data.string.repl, env);
		if (l.integer < 0 || len - i.integer < l.integer)
	    exerror("illegal length in substr(%s,%" PRIdMAX ",%" PRIdMAX ")",
	            s.string, (intmax_t)i.integer, (intmax_t)l.integer);
	} else
		l.integer = len - i.integer;

	v.string = vmalloc(ex->ve, l.integer + 1);
	if (exnode->data.string.repl) {
		strncpy(v.string, s.string + i.integer, l.integer);
		v.string[l.integer] = '\0';
	} else
		strcpy(v.string, s.string + i.integer);
	return v;
}

/* xConvert:
 * Convert from external type.
 */
static void xConvert(Expr_t *ex, Exnode_t *exnode, int type, Extype_t v,
	 Exnode_t * tmp)
{
	*tmp = *exnode->data.operand.left;
	tmp->data.constant.value = v;
	if (ex->disc->convertf(tmp, type, 0)) {
		exerror("%s: cannot convert %s value to %s",
			exnode->data.operand.left->data.variable.symbol->name,
			extypename(ex, exnode->data.operand.left->type), extypename(ex, type));
	}
	tmp->type = type;
}

/* xPrint:
 * Generate string representation from value of external type.
 */
static void xPrint(Expr_t *ex, Exnode_t *exnode, Extype_t v, Exnode_t *tmp) {
	*tmp = *exnode->data.operand.left;
	tmp->data.constant.value = v;
	if (ex->disc->stringof(ex, tmp, 0))
	exerror("%s: no string representation of %s value",
		exnode->data.operand.left->data.variable.symbol->name,
		extypename(ex, exnode->data.operand.left->type));
	tmp->type = STRING;
}

/*
 * internal exeval
 */
static long seed;

static Extype_t eval(Expr_t *ex, Exnode_t *exnode, void *env) {
	Exnode_t*	x;
	Exnode_t*	a;
	Extype_t**	t;
	int		n;
	Extype_t		v;
	Extype_t		r = {0};
	Extype_t		i;
	char*			e;
	Exnode_t		tmp;
	Exnode_t		rtmp;
	Exnode_t*		rp;
	Exassoc_t*		assoc;
	Extype_t		args[FRAME+1];
	Extype_t		save[FRAME];

	if (!exnode || ex->loopcount)
	{
		v.integer = 1;
		return v;
	}
	x = exnode->data.operand.left;
	switch (exnode->op)
	{
	case BREAK:
	case CONTINUE:
		v = eval(ex, x, env);
		ex->loopcount = v.integer;
		ex->loopop = exnode->op;
		return v;
	case CONSTANT:
		return exnode->data.constant.value;
	case DEC:
		n = -1;
	add:
		if (x->op == DYNAMIC)
			r = getdyn(ex, x, env, &assoc);
		else
		{
			if (x->data.variable.index)
				i = eval(ex, x->data.variable.index, env);
			else
				i.integer = EX_SCALAR;
			if (x->data.variable.dyna) {
				Extype_t locv;
				locv = getdyn(ex, x->data.variable.dyna, env, &assoc);
				x->data.variable.dyna->data.variable.dyna->data.constant.value = locv;
			}
			r = ex->disc->getf(ex, x, x->data.variable.symbol,
			                   x->data.variable.reference, env, (int)i.integer,
			                   ex->disc);
		}
		v = r;
		switch (x->type)
		{
		case FLOATING:
			v.floating += n;
			break;
		case INTEGER:
		case UNSIGNED:
			v.integer += n;
			break;
		default:
			goto huh;
		}
	set:
		if (x->op == DYNAMIC)
		{
			if (x->type == STRING)
			{
				v.string = vmstrdup(ex->vm, v.string);
				if ((e = assoc ? assoc->value.string : x->data.variable.symbol->value->data.constant.value.string))
					vmfree(ex->vm, e);
			}
			if (assoc)
				assoc->value = v;
			else
				x->data.variable.symbol->value->data.constant.value = v;
		}
		else
		{
			if (x->data.variable.index)
				i = eval(ex, x->data.variable.index, env);
			else
				i.integer = EX_SCALAR;
			if (x->data.variable.dyna) {
				Extype_t locv;
				locv = getdyn(ex, x->data.variable.dyna, env, &assoc);
				x->data.variable.dyna->data.variable.dyna->data.constant.value = locv;
			}
			if (ex->disc->setf(ex, x, x->data.variable.symbol,
			                   x->data.variable.reference, env, v) < 0)
				exerror("%s: cannot set value", x->data.variable.symbol->name);
		}
		if (exnode->subop == PRE)
			r = v;
		return r;
	case DYNAMIC:
		return getdyn(ex, exnode, env, &assoc);
	case SPLIT:
		return exsplit(ex, exnode, env);
	case TOKENS:
		return extokens(ex, exnode, env);
	case GSUB:
		return exsub(ex, exnode, env, /* global = */ true);
	case SUB:
		return exsub(ex, exnode, env, /* global = */ false);
	case SUBSTR:
		return exsubstr(ex, exnode, env);
	case SRAND:
		v.integer = seed;
		if (exnode->binary) {
			seed = (long)eval(ex, x, env).integer;
		} else
			seed = (long)time(0);
		srand48(seed);
		return v;
	case RAND:
		v.floating = drand48();
		return v;
	case EXIT:
		v = eval(ex, x, env);
		if (ex->disc->exitf)
			ex->disc->exitf(ex, env, (int)v.integer);
		else
			graphviz_exit((int)v.integer);
		UNREACHABLE();
	case IF:
		v = eval(ex, x, env);
		if (v.integer)
			eval(ex, exnode->data.operand.right->data.operand.left, env);
		else
			eval(ex, exnode->data.operand.right->data.operand.right, env);
		v.integer = 1;
		return v;
	case FOR:
	case WHILE:
		exnode = exnode->data.operand.right;
		for (;;)
		{
			r = eval(ex, x, env);
			if (!r.integer)
			{
				v.integer = 1;
				return v;
			}
			if (exnode->data.operand.right)
			{
				eval(ex, exnode->data.operand.right, env);
				if (ex->loopcount > 0 && (--ex->loopcount > 0 || ex->loopop != CONTINUE))
				{
					v.integer = 0;
					return v;
				}
			}
			if (exnode->data.operand.left)
				eval(ex, exnode->data.operand.left, env);
		}
		/*NOTREACHED*/
	case SWITCH:
		v = eval(ex, x, env);
		i.integer = x->type;
		r.integer = 0;
		x = exnode->data.operand.right;
		a = x->data.select.statement;
		n = 0;
		while ((x = x->data.select.next))
		{
			if (!(t = x->data.select.constant))
				n = 1;
			else
				for (; *t; t++)
				{
					switch ((int)i.integer)
					{
					case INTEGER:
					case UNSIGNED:
						if ((*t)->integer == v.integer)
							break;
						continue;
					case STRING:
						if ((ex->disc->version >= 19981111L && ex->disc->matchf)
						      ? ex->disc->matchf((*t)->string, v.string)
						      : strmatch((*t)->string, v.string))
							break;
						continue;
					case FLOATING:
						if ((*t)->floating == v.floating)
							break;
						continue;
					}
					n = 1;
					break;
				}
			if (n)
			{
				if (!x->data.select.statement)
				{
					r.integer = 1;
					break;
				}
				r = eval(ex, x->data.select.statement, env);
				if (ex->loopcount > 0)
				{
					ex->loopcount--;
					break;
				}
			}
		}
		if (!n && a)
		{
			r = eval(ex, a, env);
			if (ex->loopcount > 0)
				ex->loopcount--;
		}
		return r;
	case ITERATE:
		v.integer = 0;
		if (exnode->data.generate.array->op == DYNAMIC)
		{
			n = exnode->data.generate.index->type == STRING;
			for (assoc = dtfirst((Dt_t*)exnode->data.generate.array->data.variable.symbol->local.pointer); assoc; assoc = dtnext((Dt_t*)exnode->data.generate.array->data.variable.symbol->local.pointer, assoc))
			{
				v.integer++;
				if (n)
					exnode->data.generate.index->value->data.constant.value.string = assoc->name;
				else
					exnode->data.generate.index->value->data.constant.value = assoc->key;
				eval(ex, exnode->data.generate.statement, env);
				if (ex->loopcount > 0 && (--ex->loopcount > 0 || ex->loopop != CONTINUE))
				{
					v.integer = 0;
					break;
				}
			}
		}
		else
		{
			r = ex->disc->getf(ex, exnode, exnode->data.generate.array->data.variable.symbol,
			                   exnode->data.generate.array->data.variable.reference, env,
			                   0, ex->disc);
			for (v.integer = 0; v.integer < r.integer; v.integer++)
			{
				exnode->data.generate.index->value->data.constant.value.integer = v.integer;
				eval(ex, exnode->data.generate.statement, env);
				if (ex->loopcount > 0 && (--ex->loopcount > 0 || ex->loopop != CONTINUE))
				{
					v.integer = 0;
					break;
				}
			}
		}
		return v;
    case ITERATER:
		v.integer = 0;
		if (exnode->data.generate.array->op == DYNAMIC) {
			n = exnode->data.generate.index->type == STRING;
			for (assoc = dtlast((Dt_t *) exnode->data.generate.array->
						   data.variable.symbol->local.
						   pointer); assoc;
		 		assoc = dtprev((Dt_t *) exnode->data.generate.array->
						  data.variable.symbol->local.pointer,
						  assoc)) {
				v.integer++;
				if (n)
					exnode->data.generate.index->value->data.constant.value.string = assoc->name;
				else
					exnode->data.generate.index->value->data.constant.value = assoc->key;
				eval(ex, exnode->data.generate.statement, env);
				if (ex->loopcount > 0 && (--ex->loopcount > 0 || ex->loopop != CONTINUE)) {
					v.integer = 0;
					break;
				}
			}
		} else {
			r = ex->disc->getf(ex, exnode, exnode->data.generate.array->data.variable.symbol,
			                   exnode->data.generate.array->data.variable.reference, env,
			                   0, ex->disc);
			for (v.integer = r.integer-1; 0 <= v.integer; v.integer--) {
				exnode->data.generate.index->value->data.constant.value.integer = v.integer;
				eval(ex, exnode->data.generate.statement, env);
				if (ex->loopcount > 0 && (--ex->loopcount > 0 || ex->loopop != CONTINUE)) {
					v.integer = 0;
					break;
				}
			}
		}
		return v;
	case '#':
		v.integer = dtsize ((Dt_t*)exnode->data.variable.symbol->local.pointer);
		return v;
	case IN_OP:
		v.integer = evaldyn (ex, exnode, env, 0);
		return v;
	case UNSET:
		if (exnode->data.variable.index) {
			v.integer = evaldyn (ex, exnode, env, 1);
		}
		else {
			dtclear ((Dt_t*)exnode->data.variable.symbol->local.pointer);
			v.integer = 0;
		}
		return v;
	case CALL:
		x = exnode->data.call.args;
		for (n = 0, a = exnode->data.call.procedure->value->data.procedure.args; a && x; a = a->data.operand.right)
		{
			if (n < elementsof(args))
			{
				save[n] = a->data.operand.left->data.variable.symbol->value->data.constant.value;
				args[n++] = eval(ex, x->data.operand.left, env);
			}
			else
				a->data.operand.left->data.variable.symbol->value->data.constant.value = eval(ex, x->data.operand.left, env);
			x = x->data.operand.right;
		}
		for (n = 0, a = exnode->data.call.procedure->value->data.procedure.args; a && n < elementsof(save); a = a->data.operand.right)
			a->data.operand.left->data.variable.symbol->value->data.constant.value = args[n++];
		if (x)
			exerror("too many actual args");
		else if (a)
			exerror("not enough actual args");
		v = exeval(ex, exnode->data.call.procedure->value->data.procedure.body, env);
		for (n = 0, a = exnode->data.call.procedure->value->data.procedure.args; a && n < elementsof(save); a = a->data.operand.right)
			a->data.operand.left->data.variable.symbol->value->data.constant.value = save[n++];
		return v;
	case ARRAY:
		n = 0;
		for (x = exnode->data.operand.right; x && n < elementsof(args); x = x->data.operand.right)
			args[n++] = eval(ex, x->data.operand.left, env);
		return ex->disc->getf(ex, exnode->data.operand.left,
		                      exnode->data.operand.left->data.variable.symbol,
		                      exnode->data.operand.left->data.variable.reference, args,
		                      EX_ARRAY, ex->disc);
	case FUNCTION:
		n = 0;
		args[n++].string = env;
		for (x = exnode->data.operand.right; x && n < elementsof(args); x = x->data.operand.right)
			args[n++] = eval(ex, x->data.operand.left, env);
		return ex->disc->getf(ex, exnode->data.operand.left,
		                      exnode->data.operand.left->data.variable.symbol,
		                      exnode->data.operand.left->data.variable.reference,
		                      args+1, EX_CALL, ex->disc);
	case ID:
		if (exnode->data.variable.index)
			i = eval(ex, exnode->data.variable.index, env);
		else
			i.integer = EX_SCALAR;
		if (exnode->data.variable.dyna) {
			Extype_t locv;
			locv = getdyn(ex, exnode->data.variable.dyna, env, &assoc);
			exnode->data.variable.dyna->data.variable.dyna->data.constant.value = locv;
		}
		return ex->disc->getf(ex, exnode, exnode->data.variable.symbol,
		                      exnode->data.variable.reference, env, (int)i.integer,
		                      ex->disc);
	case INC:
		n = 1;
		goto add;
	case PRINT:
		v.integer = prints(ex, exnode, env, stdout);
		return v;
	case PRINTF:
		v.integer = print(ex, exnode, env, NULL);
		return v;
	case RETURN:
		ex->loopret = eval(ex, x, env);
		ex->loopcount = 32767;
		ex->loopop = exnode->op;
		return ex->loopret;
	case SCANF:
	case SSCANF:
		v.integer = scan(ex, exnode, env, NULL);
		return v;
	case SPRINTF: {
		FILE *buffer = tmpfile();
		if (buffer == NULL) {
			fprintf(stderr, "out of memory\n");
			graphviz_exit(EXIT_FAILURE);
		}
		print(ex, exnode, env, buffer);
		size_t size = (size_t)ftell(buffer);
		rewind(buffer);
		v.string = vmalloc(ex->ve, size + 1);
		if (v.string == NULL) {
			v.string = exnospace();
		} else {
			fread(v.string, size, 1, buffer);
			v.string[size] = '\0';
		}
		fclose(buffer);
		return v;
	}
	case '=':
		v = eval(ex, exnode->data.operand.right, env);
		if (exnode->subop != '=')
		{
			r = v;
			if (x->op == DYNAMIC)
				v = getdyn(ex, x, env, &assoc);
			else
			{
				if (x->data.variable.index)
					v = eval(ex, x->data.variable.index, env);
				else
					v.integer = EX_SCALAR;
				if (x->data.variable.dyna) {
					Extype_t locv;
					locv = getdyn(ex, x->data.variable.dyna, env, &assoc);
					x->data.variable.dyna->data.variable.dyna->data.constant.value = locv;
				}
				v = ex->disc->getf(ex, x, x->data.variable.symbol,
				                   x->data.variable.reference, env, (int)v.integer,
				                   ex->disc);
			}
			switch (x->type)
			{
			case FLOATING:
				switch (exnode->subop)
				{
				case '+':
					v.floating += r.floating;
					break;
				case '-':
					v.floating -= r.floating;
					break;
				case '*':
					v.floating *= r.floating;
					break;
				case '/':
					if (r.floating == 0.0)
						exerror("floating divide by 0");
					else
						v.floating /= r.floating;
					break;
				case '%':
					if ((r.integer = r.floating) == 0)
						exerror("floating 0 modulus");
					else
						v.floating = (Sflong_t)v.floating % r.integer;
					break;
				case '&':
					v.floating = (Sflong_t)v.floating & (Sflong_t)r.floating;
					break;
				case '|':
					v.floating = (Sflong_t)v.floating | (Sflong_t)r.floating;
					break;
				case '^':
					v.floating = (Sflong_t)v.floating ^ (Sflong_t)r.floating;
					break;
				case LSH:
					v.floating = (Sflong_t)v.floating << (Sflong_t)r.floating;
					break;
				case RSH:
#ifdef _WIN32
					v.floating = (Sflong_t)((Sfulong_t)v.floating >> (Sflong_t)r.floating);
#else
					v.floating = (Sfulong_t)v.floating >> (Sflong_t)r.floating;
#endif
					break;
				default:
					goto huh;
				}
				break;
			case INTEGER:
			case UNSIGNED:
				switch (exnode->subop)
				{
				case '+':
					v.integer += r.integer;
					break;
				case '-':
					v.integer -= r.integer;
					break;
				case '*':
					v.integer *= r.integer;
					break;
				case '/':
					if (r.integer == 0)
						exerror("integer divide by 0");
					else
						v.integer /= r.integer;
					break;
				case '%':
					if (r.integer == 0)
						exerror("integer 0 modulus");
					else
						v.integer %= r.integer;
					break;
				case '&':
					v.integer &= r.integer;
					break;
				case '|':
					v.integer |= r.integer;
					break;
				case '^':
					v.integer ^= r.integer;
					break;
				case LSH:
					v.integer <<= r.integer;
					break;
				case RSH:
					v.integer = (Sfulong_t)v.integer >> r.integer;
					break;
				default:
					goto huh;
				}
				break;
			case STRING:
				switch (exnode->subop)
				{
				case '+':
					v.string = str_add(ex, v.string, r.string);
					break;
				case '|':
					v.string = str_ior(ex, v.string, r.string);
					break;
				case '&':
					v.string = str_and(ex, v.string, r.string);
					break;
				case '^':
					v.string = str_xor(ex, v.string, r.string);
					break;
				case '%':
					v.string = str_mod(ex, v.string, r.string);
					break;
				case '*':
					v.string = str_mpy(ex, v.string, r.string);
					break;
				default:
					goto huh;
				}
				break;
			default:
				goto huh;
			}
		}
		else if (x->op == DYNAMIC)
			getdyn(ex, x, env, &assoc);
		else
			assoc = 0;
		r = v;
		goto set;
	case ';':
	case ',':
		v = eval(ex, x, env);
		while ((exnode = exnode->data.operand.right) && (exnode->op == ';' || exnode->op == ','))
		{
			v = eval(ex, exnode->data.operand.left, env);
			if (ex->loopcount)
				return v;
		}
		return exnode ? eval(ex, exnode, env) : v;
	case '?':
		v = eval(ex, x, env);
		return v.integer ? eval(ex, exnode->data.operand.right->data.operand.left, env) : eval(ex, exnode->data.operand.right->data.operand.right, env);
	case AND:
		v = eval(ex, x, env);
		return v.integer ? eval(ex, exnode->data.operand.right, env) : v;
	case OR:
		v = eval(ex, x, env);
		return v.integer ? v : eval(ex, exnode->data.operand.right, env);
	}
	v = eval(ex, x, env);
	if ((x = exnode->data.operand.right)) {
		r = eval(ex, x, env);
		if (!BUILTIN(x->type) && exnode->binary) {
			tmp = *exnode->data.operand.left;
			tmp.data.constant.value = v;
			rtmp = *x;
			rtmp.data.constant.value = r;
			if (!ex->disc->binaryf(&tmp, exnode, &rtmp, 0))
			  return tmp.data.constant.value;
		}
	}
	switch (exnode->data.operand.left->type)
	{
	case FLOATING:
		switch (exnode->op)
		{
		case F2I:
			v.integer = v.floating;
			return v;
		case F2S:
			tmp = *exnode->data.operand.left;
			tmp.data.constant.value = v;
			if (exnode->data.operand.left->op != DYNAMIC && exnode->data.operand.left->op != ID)
			{
				tmp.data.constant.value.string = exprintf(ex->ve, "%g", v.floating);
			}
			else if (ex->disc->convertf(&tmp, STRING, 0)) {
				tmp.data.constant.value.string = exprintf(ex->ve, "%g", v.floating);
			}
			tmp.type = STRING;
			return tmp.data.constant.value;
		case F2X:
			tmp = *exnode->data.operand.left;
			tmp.data.constant.value = v;
			if (ex->disc->convertf(&tmp, exnode->type, 0))
				exerror("%s: cannot convert floating value to external", tmp.data.variable.symbol->name);
			tmp.type = exnode->type;
			return tmp.data.constant.value;
		case '!':
			v.floating = !(Sflong_t)v.floating;
			return v;
		case '~':
			v.floating = ~(Sflong_t)v.floating;
			return v;
		case '-':
			if (x)
				v.floating -= r.floating;
			else
				v.floating = -v.floating;
			return v;
		case '+':
			v.floating += r.floating;
			return v;
		case '&':
			v.floating = (Sflong_t)v.floating & (Sflong_t)r.floating;
			return v;
		case '|':
			v.floating = (Sflong_t)v.floating | (Sflong_t)r.floating;
			return v;
		case '^':
			v.floating = (Sflong_t)v.floating ^ (Sflong_t)r.floating;
			return v;
		case '*':
			v.floating *= r.floating;
			return v;
		case '/':
			if (r.floating == 0.0)
				exerror("floating divide by 0");
			else
				v.floating /= r.floating;
			return v;
		case '%':
			if ((r.integer = r.floating) == 0)
				exerror("floating 0 modulus");
			else
				v.floating = (Sflong_t)v.floating % r.integer;
			return v;
		case '<':
			v.integer = v.floating < r.floating;
			return v;
		case LE:
			v.integer = v.floating <= r.floating;
			return v;
		case EQ:
			v.integer = v.floating == r.floating;
			return v;
		case NE:
			v.integer = v.floating != r.floating;
			return v;
		case GE:
			v.integer = v.floating >= r.floating;
			return v;
		case '>':
			v.integer = v.floating > r.floating;
			return v;
		case LSH:
			v.integer = (Sflong_t) ((Sfulong_t)v.floating << (Sflong_t)r.floating);
			return v;
		case RSH:
			v.integer = (Sflong_t) ((Sfulong_t)v.floating >> (Sflong_t)r.floating);
			return v;
		}
		break;
	default:
		switch (exnode->op)
		{
		case X2F:
			xConvert(ex, exnode, FLOATING, v, &tmp);
			return tmp.data.constant.value;
		case X2I:
			xConvert(ex, exnode, INTEGER, v, &tmp);
			return tmp.data.constant.value;
		case X2S:
			xConvert(ex, exnode, STRING, v, &tmp);
			return tmp.data.constant.value;
		case X2X:
			xConvert(ex, exnode, exnode->type, v, &tmp);
			return tmp.data.constant.value;
		case XPRINT:
			xPrint(ex, exnode, v, &tmp);
			return tmp.data.constant.value;
		default:
			tmp = *exnode->data.operand.left;
			tmp.data.constant.value = v;
			if (x) {
				rtmp = *x;
				rtmp.data.constant.value = r;
				rp = &rtmp;
			} else
				rp = 0;
			if (!ex->disc->binaryf(&tmp, exnode, rp, 0))
				return tmp.data.constant.value;
		}
		goto integer;
	case UNSIGNED:
		switch (exnode->op)
		{
		case '<':
			v.integer = (Sfulong_t)v.integer < (Sfulong_t)r.integer;
			return v;
		case LE:
			v.integer = (Sfulong_t)v.integer <= (Sfulong_t)r.integer;
			return v;
		case GE:
			v.integer = (Sfulong_t)v.integer >= (Sfulong_t)r.integer;
			return v;
		case '>':
			v.integer = (Sfulong_t)v.integer > (Sfulong_t)r.integer;
			return v;
		}
		/*FALLTHROUGH*/
	case INTEGER:
	integer:
		switch (exnode->op)
		{
		case I2F:
#ifdef _WIN32
			v.floating = v.integer;
#else
			if (exnode->type == UNSIGNED)
				v.floating = (Sfulong_t)v.integer;
			else
				v.floating = v.integer;
#endif
			return v;
		case I2S:
			tmp = *exnode->data.operand.left;
			tmp.data.constant.value = v;
			if (exnode->data.operand.left->op != DYNAMIC && exnode->data.operand.left->op != ID)
			{
				char *str;
				if (exnode->data.operand.left->type == UNSIGNED)
					str = exprintf(ex->ve, "%llu", (unsigned long long)v.integer);
				else
					str = exprintf(ex->ve, "%lld", (long long)v.integer);
				tmp.data.constant.value.string = str;
			}
			else if (ex->disc->convertf(&tmp, STRING, 0)) {
				char *str = NULL;
				if (exnode->data.operand.left->type == UNSIGNED)
					str = exprintf(ex->ve, "%llu", (unsigned long long)v.integer);
				else
					str = exprintf(ex->ve, "%lld", (long long)v.integer);
				tmp.data.constant.value.string = str;
			}
			tmp.type = STRING;
			return tmp.data.constant.value;
		case I2X:
			tmp = *exnode->data.operand.left;
			tmp.data.constant.value = v;
			if (ex->disc->convertf(&tmp, exnode->type, 0))
				exerror("%s: cannot convert integer value to external", tmp.data.variable.symbol->name);
			tmp.type = exnode->type;
			return tmp.data.constant.value;
		case '!':
			v.integer = !v.integer;
			return v;
		case '~':
			v.integer = ~v.integer;
			return v;
		case '-':
			if (x)
				v.integer -= r.integer;
			else
				v.integer = -v.integer;
			return v;
		case '+':
			v.integer += r.integer;
			return v;
		case '&':
			v.integer &= r.integer;
			return v;
		case '|':
			v.integer |= r.integer;
			return v;
		case '^':
			v.integer ^= r.integer;
			return v;
		case '*':
			v.integer *= r.integer;
			return v;
		case '/':
			if (r.integer == 0)
				exerror("integer divide by 0");
			else
				v.integer /= r.integer;
			return v;
		case '%':
			if (r.integer == 0)
				exerror("integer 0 modulus");
			else
				v.integer %= r.integer;
			return v;
		case EQ:
			v.integer = v.integer == r.integer;
			return v;
		case NE:
			v.integer = v.integer != r.integer;
			return v;
		case LSH:
			v.integer = (Sflong_t)v.integer << (Sflong_t)r.integer;
			return v;
		case RSH:
			v.integer = (Sfulong_t)v.integer >> (Sflong_t)r.integer;
			return v;
		case '<':
			v.integer = v.integer < r.integer;
			return v;
		case LE:
			v.integer = v.integer <= r.integer;
			return v;
		case GE:
			v.integer = v.integer >= r.integer;
			return v;
		case '>':
			v.integer = v.integer > r.integer;
			return v;
		}
		break;
	case STRING:
		switch (exnode->op)
		{
		case S2B:
			v.integer = *v.string != 0;
			return v;
		case S2F:
			tmp = *exnode->data.operand.left;
			tmp.data.constant.value = v;
			if (ex->disc->convertf(&tmp, FLOATING, 0))
			{
				tmp.data.constant.value.floating = strtod(v.string, &e);
				if (*e)
					tmp.data.constant.value.floating = *v.string != 0;
			}
			tmp.type = FLOATING;
			return tmp.data.constant.value;
		case S2I:
			tmp = *exnode->data.operand.left;
			tmp.data.constant.value = v;
			if (ex->disc->convertf(&tmp, INTEGER, 0))
			{
				if (v.string) {
					tmp.data.constant.value.integer = strtoll(v.string, &e, 0);
                    if (*e)
                        tmp.data.constant.value.integer = *v.string != 0;
				}
				else
					tmp.data.constant.value.integer = 0;
			}
			tmp.type = INTEGER;
			return tmp.data.constant.value;
		case S2X:
			tmp = *exnode->data.operand.left;
			tmp.data.constant.value = v;
			if (ex->disc->convertf(&tmp, exnode->type, 0))
				exerror("%s: cannot convert string value to external", tmp.data.variable.symbol->name);
			tmp.type = exnode->type;
			return tmp.data.constant.value;
		case EQ:
		case NE:
			v.integer = ((v.string && r.string)
			              ? ((ex->disc->version >= 19981111L && ex->disc->matchf)
			                ? ex->disc->matchf(v.string, r.string)
			                : strmatch(v.string, r.string))
			              : (v.string == r.string)) == (exnode->op == EQ);
			return v;
		case '+':
			v.string = str_add(ex, v.string, r.string);
			return v;
		case '|':
			v.string = str_ior(ex, v.string, r.string);
			return v;
		case '&':
			v.string = str_and(ex, v.string, r.string);
			return v;
		case '^':
			v.string = str_xor(ex, v.string, r.string);
			return v;
		case '%':
			v.string = str_mod(ex, v.string, r.string);
			return v;
		case '*':
			v.string = str_mpy(ex, v.string, r.string);
			return v;
		}
		v.integer = strcoll(v.string, r.string);
		switch (exnode->op)
		{
		case '<':
			v.integer = v.integer < 0;
			return v;
		case LE:
			v.integer = v.integer <= 0;
			return v;
		case GE:
			v.integer = v.integer >= 0;
			return v;
		case '>':
			v.integer = v.integer > 0;
			return v;
		}
		goto huh;
	}
 huh:
	if (exnode->binary)
		exerror("operator %s %s %s not implemented", lexname(exnode->data.operand.left->type, -1), lexname(exnode->op, exnode->subop), exnode->data.operand.right ? lexname(exnode->data.operand.right->type, -1) : "UNARY");
	else
		exerror("operator %s %s not implemented", lexname(exnode->op, exnode->subop), lexname(exnode->data.operand.left->type, -1));
	return exzero(exnode->type);
}

/*
 * evaluate expression expr
 */

Extype_t exeval(Expr_t *ex, Exnode_t *exnode, void *env) {
	Extype_t	v;

	if (exnode->compiled.integer)
	{
		switch (exnode->type)
		{
		case FLOATING:
			v.floating = exnode->compiled.floating(ex->disc->data);
			break;
		case STRING:
			v.string = exnode->compiled.string(ex->disc->data);
			break;
		default:
			v.integer = exnode->compiled.integer(ex->disc->data);
			break;
		}
	}
	else
	{
		v = eval(ex, exnode, env);
		if (ex->loopcount > 0)
		{
			ex->loopcount = 0;
			if (ex->loopop == RETURN)
				return ex->loopret;
		}
	}
	return v;
}

/* exstring:
 * Generate copy of input string using
 * string memory.
 */
char *exstring(Expr_t * ex, char *s)
{
    return vmstrdup(ex->ve, s);
}

/* exstralloc:
 * allocate sz bytes in expression memory.
 */
void *exstralloc(Expr_t * ex, size_t sz)
{
    return vmalloc(ex->ve, sz);
}
