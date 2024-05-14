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
 * expression library C program generator
 */

#include <cgraph/agxbuf.h>
#include <cgraph/exit.h>
#include <expr/exlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Excc_s Excc_t;
typedef struct Exccdisc_s Exccdisc_t;

struct Exccdisc_s			/* excc() discipline		*/
{
  agxbuf *text; // text output buffer
  char*		id;		/* symbol prefix		*/
  uint64_t	flags;		/* EXCC_* flags			*/
  int		(*ccf)(Excc_t*, Exnode_t*, Exid_t*, Exref_t*, Exnode_t*, Exccdisc_t*);
          /* program generator function	*/
};

struct Excc_s				/* excc() state			*/
{
	Expr_t*		expr;		/* exopen() state		*/
	Exdisc_t*	disc;		/* exopen() discipline		*/
	char*		id;		/* prefix + _			*/
	int		lastop;		/* last op			*/
	int		tmp;		/* temp var index		*/
	Exccdisc_t*	ccdisc;		/* excc() discipline		*/
};

#define EX_CC_DUMP	0x8000

static const char	quote[] = "\"";

static void		gen(Excc_t*, Exnode_t*);

/*
 * return C name for op
 */

char*
exopname(int op)
{
	static char	buf[16];

	switch (op)
	{
	case '!':
		return "!";
	case '%':
		return "%";
	case '&':
		return "&";
	case '(':
		return "(";
	case '*':
		return "*";
	case '+':
		return "+";
	case ',':
		return ",";
	case '-':
		return "-";
	case '/':
		return "/";
	case ':':
		return ":";
	case '<':
		return "<";
	case '=':
		return "=";
	case '>':
		return ">";
	case '?':
		return "?";
	case '^':
		return "^";
	case '|':
		return "|";
	case '~':
		return "~";
	case AND:
		return "&&";
	case EQ:
		return "==";
	case GE:
		return ">=";
	case LE:
		return "<=";
	case LSH:
		return "<<";
	case NE:
		return "!=";
	case OR:
		return "||";
	case RSH:
		return ">>";
	}
	snprintf(buf, sizeof(buf) - 1, "(OP=%03o)", op);
	return buf;
}

/*
 * generate printf()
 */

static void print(Excc_t *cc, Exnode_t *exnode) {
	Print_t*	x;

	if ((x = exnode->data.print.args))
	{
		agxbprint(cc->ccdisc->text, "sfprintf(%s, \"%s", exnode->data.print.descriptor->op == CONSTANT && exnode->data.print.descriptor->data.constant.value.integer == 2 ? "sfstderr" : "sfstdout", fmtesq(x->format, quote));
		while ((x = x->next))
			agxbput(cc->ccdisc->text, fmtesq(x->format, quote));
		agxbputc(cc->ccdisc->text, '"');
		for (x = exnode->data.print.args; x; x = x->next)
		{
			if (x->arg)
			{
				for (size_t i = 0; i < elementsof(x->param) && x->param[i]; i++)
				{
					agxbput(cc->ccdisc->text, ", (");
					gen(cc, x->param[i]);
					agxbputc(cc->ccdisc->text, ')');
				}
				agxbput(cc->ccdisc->text, ", (");
				gen(cc, x->arg);
				agxbputc(cc->ccdisc->text, ')');
			}
		}
		agxbput(cc->ccdisc->text, ");\n");
	}
}

/*
 * generate scanf()
 */

static void scan(Excc_t *cc, Exnode_t *exnode) {
	Print_t*	x;

	if ((x = exnode->data.print.args))
	{
		agxbprint(cc->ccdisc->text, "sfscanf(sfstdin, \"%s", fmtesq(x->format, quote));
		while ((x = x->next))
			agxbput(cc->ccdisc->text, fmtesq(x->format, quote));
		agxbputc(cc->ccdisc->text, '"');
		for (x = exnode->data.print.args; x; x = x->next)
		{
			if (x->arg)
			{
				for (size_t i = 0; i < elementsof(x->param) && x->param[i]; i++)
				{
					agxbput(cc->ccdisc->text, ", &(");
					gen(cc, x->param[i]);
					agxbputc(cc->ccdisc->text, ')');
				}
				agxbput(cc->ccdisc->text, ", &(");
				gen(cc, x->arg);
				agxbputc(cc->ccdisc->text, ')');
			}
		}
		agxbput(cc->ccdisc->text, ");\n");
	}
}

/*
 * internal excc
 */

static void gen(Excc_t *cc, Exnode_t *exnode) {
	Exnode_t*	x;
	Exnode_t*	y;
	int		n;
	int		m;
	int		t;
	char*			s;
	Extype_t*		v;
	Extype_t**		p;

	if (!exnode)
		return;
	if (exnode->op == CALL) {
		agxbprint(cc->ccdisc->text, "%s(", exnode->data.call.procedure->name);
		if (exnode->data.call.args)
			gen(cc, exnode->data.call.args);
		agxbputc(cc->ccdisc->text, ')');
		return;
	}
	x = exnode->data.operand.left;
	switch (exnode->op)
	{
	case BREAK:
		agxbput(cc->ccdisc->text, "break;\n");
		return;
	case CONTINUE:
		agxbput(cc->ccdisc->text, "continue;\n");
		return;
	case CONSTANT:
		switch (exnode->type)
		{
		case FLOATING:
			agxbprint(cc->ccdisc->text, "%g", exnode->data.constant.value.floating);
			break;
		case STRING:
			agxbprint(cc->ccdisc->text, "\"%s\"", fmtesq(exnode->data.constant.value.string, quote));
			break;
		case UNSIGNED:
			agxbprint(cc->ccdisc->text, "%llu",
			          (long long unsigned)exnode->data.constant.value.integer);
			break;
		default:
			agxbprint(cc->ccdisc->text, "%lld", exnode->data.constant.value.integer);
			break;
		}
		return;
	case DEC:
		agxbprint(cc->ccdisc->text, "%s--", x->data.variable.symbol->name);
		return;
	case DYNAMIC:
		agxbput(cc->ccdisc->text, exnode->data.variable.symbol->name);
		return;
	case EXIT:
		agxbput(cc->ccdisc->text, "exit(");
		gen(cc, x);
		agxbput(cc->ccdisc->text, ");\n");
		return;
	case FUNCTION:
		gen(cc, x);
		agxbputc(cc->ccdisc->text, '(');
		if ((y = exnode->data.operand.right)) {
			gen(cc, y);
		}
		agxbputc(cc->ccdisc->text, ')');
		return;
	case RAND:
		agxbput(cc->ccdisc->text, "rand();\n");
		return;
	case SRAND:
		if (exnode->binary) {
			agxbput(cc->ccdisc->text, "srand(");
			gen(cc, x);
			agxbput(cc->ccdisc->text, ");\n");
		} else
			agxbput(cc->ccdisc->text, "srand();\n");
		return;
   	case GSUB:
   	case SUB:
   	case SUBSTR:
		s = (exnode->op == GSUB ? "gsub(" : exnode->op == SUB ? "sub(" : "substr(");
		agxbput(cc->ccdisc->text, s);
		gen(cc, exnode->data.string.base);
		agxbput(cc->ccdisc->text, ", ");
		gen(cc, exnode->data.string.pat);
		if (exnode->data.string.repl) {
			agxbput(cc->ccdisc->text, ", ");
			gen(cc, exnode->data.string.repl);
		}
		agxbputc(cc->ccdisc->text, ')');
		return;
   	case IN_OP:
		gen(cc, exnode->data.variable.index);
		agxbprint(cc->ccdisc->text, " in %s", exnode->data.variable.symbol->name);
		return;
	case IF:
		agxbput(cc->ccdisc->text, "if (");
		gen(cc, x);
		agxbput(cc->ccdisc->text, ") {\n");
		gen(cc, exnode->data.operand.right->data.operand.left);
		if (exnode->data.operand.right->data.operand.right)
		{
			agxbput(cc->ccdisc->text, "} else {\n");
			gen(cc, exnode->data.operand.right->data.operand.right);
		}
		agxbput(cc->ccdisc->text, "}\n");
		return;
	case FOR:
		agxbput(cc->ccdisc->text, "for (;");
		gen(cc, x);
		agxbput(cc->ccdisc->text, ");");
		if (exnode->data.operand.left)
		{
			agxbputc(cc->ccdisc->text, '(');
			gen(cc, exnode->data.operand.left);
			agxbputc(cc->ccdisc->text, ')');
		}
		agxbput(cc->ccdisc->text, ") {");
		if (exnode->data.operand.right)
			gen(cc, exnode->data.operand.right);
		agxbputc(cc->ccdisc->text, '}');
		return;
	case ID:
		if (cc->ccdisc->ccf)
			cc->ccdisc->ccf(cc, exnode, exnode->data.variable.symbol, exnode->data.variable.reference, exnode->data.variable.index, cc->ccdisc);
		else
			agxbput(cc->ccdisc->text, exnode->data.variable.symbol->name);
		return;
	case INC:
		agxbprint(cc->ccdisc->text, "%s++", x->data.variable.symbol->name);
		return;
	case ITERATE:
	case ITERATER:
		if (exnode->op == DYNAMIC)
		{
			agxbprint(cc->ccdisc->text, "{ Exassoc_t* %stmp_%d;", cc->id, ++cc->tmp);
			agxbprint(cc->ccdisc->text, "for (%stmp_%d = (Exassoc_t*)dtfirst(%s); %stmp_%d && (%s = %stmp_%d->name); %stmp_%d = (Exassoc_t*)dtnext(%s, %stmp_%d)) {", cc->id, cc->tmp, exnode->data.generate.array->data.variable.symbol->name, cc->id, cc->tmp, exnode->data.generate.index->name, cc->id, cc->tmp, cc->id, cc->tmp, exnode->data.generate.array->data.variable.symbol->name, cc->id, cc->tmp);
			gen(cc, exnode->data.generate.statement);
			agxbput(cc->ccdisc->text, "} }");
		}
		return;
	case PRINT:
		agxbput(cc->ccdisc->text, "print");
		if (x)
			gen(cc, x);
		else
			agxbput(cc->ccdisc->text, "()");
		return;
	case PRINTF:
		print(cc, exnode);
		return;
	case RETURN:
		agxbput(cc->ccdisc->text, "return(");
		gen(cc, x);
		agxbput(cc->ccdisc->text, ");\n");
		return;
	case SCANF:
		scan(cc, exnode);
		return;
	case SPLIT:
	case TOKENS:
		if (exnode->op == SPLIT)
			agxbput(cc->ccdisc->text, "split (");
		else
			agxbput(cc->ccdisc->text, "tokens (");
		gen(cc, exnode->data.split.string);
		agxbprint(cc->ccdisc->text, ", %s", exnode->data.split.array->name);
		if (exnode->data.split.seps) {
			agxbputc(cc->ccdisc->text, ',');
			gen(cc, exnode->data.split.seps);
		}
		agxbputc(cc->ccdisc->text, ')');
		return;
	case SWITCH:
		t = x->type;
		agxbprint(cc->ccdisc->text, "{ %s %stmp_%d = ", extype(t), cc->id, ++cc->tmp);
		gen(cc, x);
		agxbputc(cc->ccdisc->text, ';');
		x = exnode->data.operand.right;
		y = x->data.select.statement;
		n = 0;
		while ((x = x->data.select.next))
		{
			if (n)
				agxbput(cc->ccdisc->text, "else ");
			if (!(p = x->data.select.constant))
				y = x->data.select.statement;
			else
			{
				m = 0;
				while ((v = *p++))
				{
					if (m)
						agxbput(cc->ccdisc->text, "||");
					else
					{
						m = 1;
						agxbput(cc->ccdisc->text, "if (");
					}
					if (t == STRING)
						agxbprint(cc->ccdisc->text, "strmatch(%stmp_%d, \"%s\")", cc->id, cc->tmp, fmtesq(v->string, quote));
					else
					{
						agxbprint(cc->ccdisc->text, "%stmp_%d == ", cc->id, cc->tmp);
						switch (t)
						{
						case INTEGER:
						case UNSIGNED:
							agxbprint(cc->ccdisc->text, "%llu",
							          (unsigned long long)v->integer);
							break;
						default:
							agxbprint(cc->ccdisc->text, "%g", v->floating);
							break;
						}
					}
				}
				agxbput(cc->ccdisc->text, ") {");
				gen(cc, x->data.select.statement);
				agxbputc(cc->ccdisc->text, '}');
			}
		}
		if (y)
		{
			if (n)
				agxbput(cc->ccdisc->text, "else ");
			agxbputc(cc->ccdisc->text, '{');
			gen(cc, y);
			agxbputc(cc->ccdisc->text, '}');
		}
		agxbputc(cc->ccdisc->text, '}');
		return;
	case UNSET:
		agxbprint(cc->ccdisc->text, "unset(%s", exnode->data.variable.symbol->name);
		if (exnode->data.variable.index) {
			agxbputc(cc->ccdisc->text, ',');
			gen(cc, exnode->data.variable.index);
		}
		agxbputc(cc->ccdisc->text, ')');
		return;
	case WHILE:
		agxbput(cc->ccdisc->text, "while (");
		gen(cc, x);
		agxbput(cc->ccdisc->text, ") {");
		if (exnode->data.operand.right)
			gen(cc, exnode->data.operand.right);
		agxbputc(cc->ccdisc->text, '}');
		return;
    case '#':
		agxbprint(cc->ccdisc->text, "# %s", exnode->data.variable.symbol->name);
		return;
	case '=':
		agxbprint(cc->ccdisc->text, "(%s%s=", x->data.variable.symbol->name, exnode->subop == '=' ? "" : exopname(exnode->subop));
		gen(cc, exnode->data.operand.right);
		agxbputc(cc->ccdisc->text, ')');
		return;
	case ';':
		for (;;)
		{
			if (!(x = exnode->data.operand.right))
				switch (cc->lastop = exnode->data.operand.left->op)
				{
				case FOR:
				case IF:
				case PRINTF:
				case PRINT:
				case RETURN:
				case WHILE:
					break;
				default:
					agxbprint(cc->ccdisc->text, "_%svalue=", cc->id);
					break;
				}
			gen(cc, exnode->data.operand.left);
			agxbput(cc->ccdisc->text, ";\n");
			if (!(exnode = x))
				break;
			switch (cc->lastop = exnode->op)
			{
			case ';':
				continue;
			case FOR:
			case IF:
			case PRINTF:
			case PRINT:
			case RETURN:
			case WHILE:
				break;
			default:
				agxbprint(cc->ccdisc->text, "_%svalue=", cc->id);
				break;
			}
			gen(cc, exnode);
			agxbput(cc->ccdisc->text, ";\n");
			break;
		}
		return;
	case ',':
		agxbputc(cc->ccdisc->text, '(');
		gen(cc, x);
		while ((exnode = exnode->data.operand.right) && exnode->op == ',')
		{
			agxbput(cc->ccdisc->text, "), (");
			gen(cc, exnode->data.operand.left);
		}
		if (exnode)
		{
			agxbput(cc->ccdisc->text, "), (");
			gen(cc, exnode);
		}
		agxbputc(cc->ccdisc->text, ')');
		return;
	case '?':
		agxbputc(cc->ccdisc->text, '(');
		gen(cc, x);
		agxbput(cc->ccdisc->text, ") ? (");
		gen(cc, exnode->data.operand.right->data.operand.left);
		agxbput(cc->ccdisc->text, ") : (");
		gen(cc, exnode->data.operand.right->data.operand.right);
		agxbputc(cc->ccdisc->text, ')');
		return;
	case AND:
		agxbputc(cc->ccdisc->text, '(');
		gen(cc, x);
		agxbput(cc->ccdisc->text, ") && (");
		gen(cc, exnode->data.operand.right);
		agxbputc(cc->ccdisc->text, ')');
		return;
	case OR:
		agxbputc(cc->ccdisc->text, '(');
		gen(cc, x);
		agxbput(cc->ccdisc->text, ") || (");
		gen(cc, exnode->data.operand.right);
		agxbputc(cc->ccdisc->text, ')');
		return;
	case F2I:
		agxbprint(cc->ccdisc->text, "(%s)(", extype(INTEGER));
		gen(cc, x);
		agxbputc(cc->ccdisc->text, ')');
		return;
	case I2F:
		agxbprint(cc->ccdisc->text, "(%s)(", extype(FLOATING));
		gen(cc, x);
		agxbputc(cc->ccdisc->text, ')');
		return;
	case S2I:
		agxbput(cc->ccdisc->text, "strtoll(");
		gen(cc, x);
		agxbput(cc->ccdisc->text, ",(char**)0,0)");
		return;
    case X2I:
		agxbput(cc->ccdisc->text, "X2I(");
		gen(cc, x);
		agxbputc(cc->ccdisc->text, ')');
		return;
	case X2X:
		agxbput(cc->ccdisc->text, "X2X(");
		gen(cc, x);
		agxbputc(cc->ccdisc->text, ')');
		return;
	}
	y = exnode->data.operand.right;
	if (x->type == STRING)
	{
		switch (exnode->op)
		{
		case S2B:
			agxbput(cc->ccdisc->text, "*(");
			gen(cc, x);
			agxbput(cc->ccdisc->text, ")!=0");
			return;
		case S2F:
			agxbput(cc->ccdisc->text, "strtod(");
			gen(cc, x);
			agxbput(cc->ccdisc->text, ",0)");
			return;
		case S2I:
			agxbput(cc->ccdisc->text, "strtol(");
			gen(cc, x);
			agxbput(cc->ccdisc->text, ",0,0)");
			return;
		case S2X:
			agxbput(cc->ccdisc->text, "** cannot convert string value to external **");
			return;
		case NE:
			agxbputc(cc->ccdisc->text, '!');
			/*FALLTHROUGH*/
		case EQ:
			agxbput(cc->ccdisc->text, "strmatch(");
			gen(cc, x);
			agxbputc(cc->ccdisc->text, ',');
			gen(cc, y);
			agxbputc(cc->ccdisc->text, ')');
			return;
		case '+':
		case '|':
		case '&':
		case '^':
		case '%':
		case '*':
			agxbput(cc->ccdisc->text, "** string bits not supported **");
			return;
		}
		switch (exnode->op)
		{
		case '<':
			s = "<0";
			break;
		case LE:
			s = "<=0";
			break;
		case GE:
			s = ">=0";
			break;
		case '>':
			s = ">0";
			break;
		default:
			s = "** unknown string op **";
			break;
		}
		agxbput(cc->ccdisc->text, "strcoll(");
		gen(cc, x);
		agxbputc(cc->ccdisc->text, ',');
		gen(cc, y);
		agxbprint(cc->ccdisc->text, ")%s", s);
		return;
	}
	else
	{
		if (!y)
			agxbput(cc->ccdisc->text, exopname(exnode->op));
		agxbputc(cc->ccdisc->text, '(');
		gen(cc, x);
		if (y)
		{
			agxbprint(cc->ccdisc->text, ")%s(", exopname(exnode->op));
			gen(cc, y);
		}
		agxbputc(cc->ccdisc->text, ')');
	}
	return;
}

/*
 * generate global declarations
 */

static int global(void *object, void *handle) {
	agxbuf *stream = handle;
	Exid_t*	sym = object;

	if (sym->lex == DYNAMIC)
		agxbprint(stream, "static %s	%s;\n", extype(sym->type), sym->name);
	return 0;
}

/*
 * open C program generator context
 */

static Excc_t *exccopen(Expr_t *ex, Exccdisc_t *disc) {
	Excc_t*	cc;
	char*			id;

	if (!(id = disc->id))
		id = "";
	if (!(cc = calloc(1, sizeof(Excc_t) + strlen(id) + 2)))
		return 0;
	cc->expr = ex;
	cc->disc = ex->disc;
	cc->id = (char*)(cc + 1);
	cc->ccdisc = disc;
	if (!(disc->flags & EX_CC_DUMP))
	{
		agxbprint(disc->text, "/* : : generated by %s : : */\n", exversion);
		agxbput(disc->text, "\n#include <ast/ast.h>\n");
		if (*id)
			snprintf(cc->id, strlen(id) + 2, "%s_", id);
		agxbputc(disc->text, '\n');
		dtwalk(ex->symbols, global, disc->text);
	}
	return cc;
}

/*
 * close C program generator context
 */

static int exccclose(Excc_t *cc) {
	int	r = 0;

	if (!cc)
		r = -1;
	else
	{
		if (!(cc->ccdisc->flags & EX_CC_DUMP))
		{
			if (cc->ccdisc->text)
				agxbuse(cc->ccdisc->text);
			else
				r = -1;
		}
		free(cc);
	}
	return r;
}

/*
 * dump an expression tree to a buffer
 */

int exdump(Expr_t *ex, Exnode_t *node, agxbuf *xb) {
	Excc_t*		cc;
	Exccdisc_t	ccdisc;
	Exid_t*		sym;

	memset(&ccdisc, 0, sizeof(ccdisc));
	ccdisc.flags = EX_CC_DUMP;
	ccdisc.text = xb;
	if (!(cc = exccopen(ex, &ccdisc)))
		return -1;
	if (node)
		gen(cc, node);
	else
		for (sym = dtfirst(ex->symbols); sym; sym = dtnext(ex->symbols, sym))
			if (sym->lex == PROCEDURE && sym->value)
			{
				agxbprint(xb, "%s:\n", sym->name);
				gen(cc, sym->value->data.procedure.body);
			}
	agxbputc(xb, '\n');
	return exccclose(cc);
}
