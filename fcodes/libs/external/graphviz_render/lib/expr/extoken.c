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
 * expression library default lexical analyzer
 */

#include "config.h"
#include <cgraph/agxbuf.h>
#include <ctype.h>
#include <expr/exlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(TRACE_lex) && TRACE_lex

/*
 * trace c for op
 */

static void
trace(Expr_t* ex, int lev, char* op, int c)
{
	char*	s = 0;
	char*	t;
	char	buf[16];
	void*	x = 0;

	t = "";
	switch (c)
	{
	case 0:
		s = " EOF";
		break;
	case '=':
		s = t = buf;
		*t++ = ' ';
		if (!lev && ex_lval.op != c)
			*t++ = ex_lval.op;
		*t++ = c;
		*t = 0;
		break;
	case AND:
		s = " AND ";
		t = "&&";
		break;
	case DEC:
		s = " DEC ";
		t = "--";
		break;
	case DECLARE:
		s = " DECLARE ";
		t = ex_lval.id->name;
		break;
	case DYNAMIC:
		s = " DYNAMIC ";
		t = ex_lval.id->name;
		x = (void*)ex_lval.id;
		break;
	case EQ:
		s = " EQ ";
		t = "==";
		break;
	case FLOATING:
		s = " FLOATING ";
		snprintf(t = buf, sizeof(buf), "%f", ex_lval.floating);
		break;
	case GE:
		s = " GE ";
		t = ">=";
		break;
	case CONSTANT:
		s = " CONSTANT ";
		t = ex_lval.id->name;
		break;
	case ID:
		s = " ID ";
		t = ex_lval.id->name;
		break;
	case INC:
		s = "INC ";
		t = "++";
		break;
	case INTEGER:
		s = " INTEGER ";
		snprintf(t = buf, sizeof(buf), "%lld", (long long)ex_lval.integer);
		break;
	case LABEL:
		s = " LABEL ";
		t = ex_lval.id->name;
		break;
	case LE:
		s = " LE ";
		t = "<=";
		break;
	case LSH:
		s = " LSH ";
		t = "<<";
		break;
	case NAME:
		s = " NAME ";
		t = ex_lval.id->name;
		x = (void*)ex_lval.id;
		break;
	case NE:
		s = " NE ";
		t = "!=";
		break;
	case OR:
		s = " OR ";
		t = "||";
		break;
	case RSH:
		s = " RSH ";
		t = ">>";
		break;
	case STRING:
		s = " STRING ";
		t = fmtesc(ex_lval.string);
		break;
	case UNSIGNED:
		s = " UNSIGNED ";
		snprintf(t = buf, sizeof(buf), "%llu", (unsigned long long)ex_lval.integer);
		break;
	case BREAK:
		s = " break";
		break;
	case CASE:
		s = " case";
		break;
	case CONTINUE:
		s = " continue";
		break;
	case DEFAULT:
		s = " default";
		break;
	case ELSE:
		s = " else";
		break;
	case EXIT:
		s = " exit";
		break;
	case FOR:
		s = " for";
		break;
	case ITERATER:
		s = " forf";
		break;
	case GSUB:
		s = " gsub";
		break;
	case IF:
		s = " if";
		break;
	case IN_OP:
		s = " in";
		break;
	case PRAGMA:
		s = " pragma";
		break;
	case PRINT:
		s = " print";
		break;
	case PRINTF:
		s = " printf";
		break;
	case QUERY:
		s = " query";
		break;
	case RAND:
		s = " rand";
		break;
	case RETURN:
		s = " return";
		break;
	case SPLIT:
		s = " split";
		break;
	case SPRINTF:
		s = " sprintf";
		break;
	case SRAND:
		s = " srand";
		break;
	case SUB:
		s = " sub";
		break;
	case SUBSTR:
		s = " substr";
		break;
	case SWITCH:
		s = " switch";
		break;
	case TOKENS:
		s = " tokens";
		break;
	case UNSET:
		s = " unset";
		break;
	case WHILE:
		s = " while";
		break;
	default:
		if (c < 0177)
		{
			s = buf;
			*s++ = c;
			*s = 0;
			t = fmtesc(buf);
			s = " ";
		}
		break;
	}
	if (x)
		error(TRACE_lex + lev, "%s: [%d] %04d%s%s (%x)", op, ex->input->nesting, c, s, t, x);

	else
		error(TRACE_lex + lev, "%s: [%d] %04d%s%s", op, ex->input->nesting, c, s, t);
}

/*
 * trace wrapper for extoken()
 */

extern int	_extoken_fn_(Expr_t*);

int
extoken_fn(Expr_t* ex)
{
	int	c;

#define extoken_fn	_extoken_fn_

	c = extoken_fn(ex);
	trace(ex, 0, "ex_lex", c);
	return c;
}

#else

#define trace(p,a,b,c) do { } while (0)

#endif

/*
 * get the next expression char
 */

static int
lex(Expr_t* ex)
{
	int	c;

	for (;;)
	{
		if ((c = ex->input->peek))
			ex->input->peek = 0;
		else if (ex->input->pp)
		{
			if (!(c = *ex->input->pp++))
			{
				ex->input->pp = 0;
				continue;
			}
		}
		else if (ex->input->sp)
		{
			if (!(c = *ex->input->sp++))
			{
				if (!expop(ex))
					continue;
				else trace(ex, -1, "expop sp FAIL", 0);
				ex->input->sp--;
			}
		}
		else if (ex->input->fp)
		{
			if ((c = getc(ex->input->fp)) == EOF)
			{
				if (!expop(ex))
					continue;
				else trace(ex, -1, "expop fp FAIL", 0);
				c = 0;
			}
			else if ((ex->disc->flags & EX_INTERACTIVE) && c == '\n' && ex->input->next && !ex->input->next->next && ex->input->nesting <= 0)
			{
				error_info.line++;
				expop(ex);
				trace(ex, -1, "expop sp FORCE", 0);
				c = 0;
			}
		}
		else c = 0;
		if (c == '\n')
			setcontext(ex);
		else if (c)
			putcontext(ex, c);
		trace(ex, -3, "ex--lex", c);
		return c;
	}
}

/*
 * get the next expression token
 */

int
extoken_fn(Expr_t* ex)
{
	int	c;
	char*	s;
	int	q;
	char*		e;
	Dt_t*		v;

	if (ex->eof || ex->errors)
		return 0;
 again:
	for (;;)
		switch (c = lex(ex))
		{
		case 0:
			goto eof;
		case '/':
			switch (q = lex(ex))
			{
			case '*':
				for (;;) switch (lex(ex))
				{
				case '\n':
					if (error_info.line)
						error_info.line++;
					else error_info.line = 2;
					continue;
				case '*':
					switch (lex(ex))
					{
					case 0:
						goto eof;
					case '\n':
						if (error_info.line)
							error_info.line++;
						else error_info.line = 2;
						break;
					case '*':
						exunlex(ex, '*');
						break;
					case '/':
						goto again;
					}
					break;
				}
				break;
			case '/':
				while ((c = lex(ex)) != '\n')
					if (!c)
						goto eof;
				break;
			default:
				goto opeq;
			}
			/*FALLTHROUGH*/
		case '\n':
			if (error_info.line)
				error_info.line++;
			else error_info.line = 2;
			/*FALLTHROUGH*/
		case ' ':
		case '\t':
		case '\r':
			break;
		case '(':
		case '{':
		case '[':
			ex->input->nesting++;
			return ex_lval.op = c;
		case ')':
		case '}':
		case ']':
			ex->input->nesting--;
			return ex_lval.op = c;
		case '+':
		case '-':
			if ((q = lex(ex)) == c)
				return ex_lval.op = c == '+' ? INC : DEC;
			goto opeq;
		case '*':
		case '%':
		case '^':
			q = lex(ex);
		opeq:
			ex_lval.op = c;
			if (q == '=')
				c = '=';
			else if (q == '%' && c == '%')
			{
				goto eof;
			}
			else exunlex(ex, q);
			return c;
		case '&':
		case '|':
			if ((q = lex(ex)) == '=')
			{
				ex_lval.op = c;
				return '=';
			}
			if (q == c)
				c = c == '&' ? AND : OR;
			else exunlex(ex, q);
			return ex_lval.op = c;
		case '<':
		case '>':
			if ((q = lex(ex)) == c)
			{
				ex_lval.op = c = c == '<' ? LSH : RSH;
				if ((q = lex(ex)) == '=')
					c = '=';
				else exunlex(ex, q);
				return c;
			}
			goto relational;
		case '=':
		case '!':
			q = lex(ex);
		relational:
			if (q == '=') switch (c)
			{
			case '<':
				c = LE;
				break;
			case '>':
				c = GE;
				break;
			case '=':
				c = EQ;
				break;
			case '!':
				c = NE;
				break;
			}
			else exunlex(ex, q);
			return ex_lval.op = c;
		case '#':
			if (!ex->linewrap && !(ex->disc->flags & EX_PURE))
			{
				s = ex->linep - 1;
				while (s > ex->line && isspace((int)*(s - 1)))
					s--;
				if (s == ex->line)
				{
					switch (extoken_fn(ex))
					{
					case DYNAMIC:
					case ID:
					case NAME:
						s = ex_lval.id->name;
						break;
					default:
						s = "";
						break;
					}
					if (streq(s, "include"))
					{
						if (extoken_fn(ex) != STRING)
							exerror("#%s: string argument expected", s);
						else if (!expush(ex, ex_lval.string, 1, NULL))
						{
							setcontext(ex);
							goto again;
						}
					}
					else exerror("unknown directive");
				}
			}
			return ex_lval.op = c;
		case '\'':
		case '"':
			q = c;
			agxbclear(&ex->tmp);
			ex->input->nesting++;
			while ((c = lex(ex)) != q)
			{
				if (c == '\\')
				{
					agxbputc(&ex->tmp, '\\');
					c = lex(ex);
				}
				if (!c)
				{
					exerror("unterminated %c string", q);
					goto eof;
				}
				if (c == '\n')
				{
					if (error_info.line)
						error_info.line++;
					else error_info.line = 2;
				}
				agxbputc(&ex->tmp, (char)c);
			}
			ex->input->nesting--;
			s = agxbuse(&ex->tmp);
			if (q == '"' || (ex->disc->flags & EX_CHARSTRING))
			{
				if (!(ex_lval.string = vmstrdup(ex->vm, s)))
					goto eof;
				stresc(ex_lval.string);
				return STRING;
			}
			ex_lval.integer = chrtoi(s);
			return INTEGER;
		case '.':
			if (isdigit(c = lex(ex)))
			{
				agxbclear(&ex->tmp);
				agxbput(&ex->tmp, "0.");
				goto floating;
			}
			exunlex(ex, c);
			return ex_lval.op = '.';
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9': {
			agxbclear(&ex->tmp);
			agxbputc(&ex->tmp, (char)c);
			q = INTEGER;
			int b = 0;
			if ((c = lex(ex)) == 'x' || c == 'X')
			{
				b = 16;
				agxbputc(&ex->tmp, (char)c);
				for (c = lex(ex); isxdigit(c); c = lex(ex))
				{
					agxbputc(&ex->tmp, (char)c);
				}
			}
			else
			{
				while (isdigit(c))
				{
					agxbputc(&ex->tmp, (char)c);
					c = lex(ex);
				}
				if (c == '#')
				{
					agxbputc(&ex->tmp, (char)c);
					do
					{
						agxbputc(&ex->tmp, (char)c);
					} while (isalnum(c = lex(ex)));
				}
				else
				{
					if (c == '.')
					{
					floating:
						q = FLOATING;
						agxbputc(&ex->tmp, (char)c);
						while (isdigit(c = lex(ex)))
							agxbputc(&ex->tmp, (char)c);
					}
					if (c == 'e' || c == 'E')
					{
						q = FLOATING;
						agxbputc(&ex->tmp, (char)c);
						if ((c = lex(ex)) == '-' || c == '+')
						{
							agxbputc(&ex->tmp, (char)c);
							c = lex(ex);
						}
						while (isdigit(c))
						{
							agxbputc(&ex->tmp, (char)c);
							c = lex(ex);
						}
					}
				}
			}
			s = agxbuse(&ex->tmp);
			if (q == FLOATING)
				ex_lval.floating = strtod(s, &e);
			else
			{
				if (c == 'u' || c == 'U')
				{
					q = UNSIGNED;
					c = lex(ex);
					ex_lval.integer = strtoull(s, &e, b);
				}
				else
					ex_lval.integer = strtoll(s, &e, b);
			}
			exunlex(ex, c);
			if (*e || isalpha(c) || c == '_' || c == '$')
			{
				exerror("%s: invalid numeric constant", s);
				goto eof;
			}
			return q;
		}
		default:
			if (isalpha(c) || c == '_' || c == '$')
			{
				agxbclear(&ex->tmp);
				agxbputc(&ex->tmp, (char)c);
				while (isalnum(c = lex(ex)) || c == '_' || c == '$')
					agxbputc(&ex->tmp, (char)c);
				exunlex(ex, c);
				s = agxbuse(&ex->tmp);
				/* v = expr.declare ? dtview(ex->symbols, NULL) : (Dt_t*)0; FIX */
				v = (Dt_t*)0;
				ex_lval.id = dtmatch(ex->symbols, s);
				if (v)
					dtview(ex->symbols, v);
				if (!ex_lval.id)
				{
					if (!(ex_lval.id = calloc(1, sizeof(Exid_t) + strlen(s) - EX_NAMELEN + 1)))
					{
						exnospace();
						goto eof;
					}
					strcpy(ex_lval.id->name, s);
					ex_lval.id->lex = NAME;
					expr.statics += ex_lval.id->isstatic = expr.instatic;

					/*
					 * LABELs are in the parent scope!
					 */

					if (c == ':' && !expr.nolabel && ex->frame && ex->frame->view)
						dtinsert(ex->frame->view, ex_lval.id);
					else
						dtinsert(ex->symbols, ex_lval.id);
				}

				/*
				 * lexical analyzer state controlled by the grammar
				 */

				switch (ex_lval.id->lex)
				{
				case DECLARE:
					if (ex_lval.id->index == CHARACTER)
					{
						/*
						 * `char*' === `string'
						 * the * must immediately follow char
						 */

						if (c == '*')
						{
							lex(ex);
							ex_lval.id = id_string;
						}
					}
					break;
				case NAME:
					/*
					 * action labels are disambiguated from ?:
					 * through the expr.nolabel grammar hook
					 * the : must immediately follow labels
					 */

					if (c == ':' && !expr.nolabel)
						return LABEL;
					break;
				case PRAGMA:
					/*
					 * user specific statement stripped and
					 * passed as string
					 */

					{
						int	b;
						int	n;
						int	pc = 0;
						int	po;
						int	t;

						/*UNDENT...*/
		agxbclear(&ex->tmp);
		b = 1;
		n = 0;
		po = 0;
		for (c = t = lex(ex);; c = lex(ex))
		{
			switch (c)
			{
			case 0:
				goto eof;
			case '/':
				switch (q = lex(ex))
				{
				case '*':
					for (;;)
					{
						switch (lex(ex))
						{
						case '\n':
							if (error_info.line)
								error_info.line++;
							else error_info.line = 2;
							continue;
						case '*':
							switch (lex(ex))
							{
							case 0:
								goto eof;
							case '\n':
								if (error_info.line)
									error_info.line++;
								else error_info.line = 2;
								continue;
							case '*':
								exunlex(ex, '*');
								continue;
							case '/':
								break;
							default:
								continue;
							}
							break;
						}
						if (!b++)
							goto eof;
						agxbputc(&ex->tmp, ' ');
						break;
					}
					break;
				case '/':
					while ((c = lex(ex)) != '\n')
						if (!c)
							goto eof;
					if (error_info.line)
						error_info.line++;
					else error_info.line = 2;
					b = 1;
					agxbputc(&ex->tmp, '\n');
					break;
				default:
					b = 0;
					agxbputc(&ex->tmp, (char)c);
					agxbputc(&ex->tmp, (char)q);
					break;
				}
				continue;
			case '\n':
				if (error_info.line)
					error_info.line++;
				else error_info.line = 2;
				b = 1;
				agxbputc(&ex->tmp, '\n');
				continue;
			case ' ':
			case '\t':
				if (!b++)
					goto eof;
				agxbputc(&ex->tmp, ' ');
				continue;
			case '(':
			case '{':
			case '[':
				b = 0;
				if (!po)
				{
					switch (po = c)
					{
					case '(':
						pc = ')';
						break;
					case '{':
						pc = '}';
						break;
					case '[':
						pc = ']';
						break;
					}
					n++;
				}
				else if (c == po)
					n++;
				agxbputc(&ex->tmp, (char)c);
				continue;
			case ')':
			case '}':
			case ']':
				b = 0;
				if (!po)
				{
					exunlex(ex, c);
					break;
				}
				agxbputc(&ex->tmp, (char)c);
				if (c == pc && --n <= 0)
				{
					if (t == po)
						break;
					po = 0;
				}
				continue;
			case ';':
				b = 0;
				if (!n)
					break;
				agxbputc(&ex->tmp, (char)c);
				continue;
			case '\'':
			case '"':
				b = 0;
				agxbputc(&ex->tmp, (char)c);
				ex->input->nesting++;
				q = c;
				while ((c = lex(ex)) != q)
				{
					if (c == '\\')
					{
						agxbputc(&ex->tmp, '\\');
						c = lex(ex);
					}
					if (!c)
					{
						exerror("unterminated %c string", q);
						goto eof;
					}
					if (c == '\n')
					{
						if (error_info.line)
							error_info.line++;
						else error_info.line = 2;
					}
					agxbputc(&ex->tmp, (char)c);
				}
				ex->input->nesting--;
				continue;
			default:
				b = 0;
				agxbputc(&ex->tmp, (char)c);
				continue;
			}
			break;
		}
		ex->disc->reff(ex, NULL, ex_lval.id, NULL);

						/*..INDENT*/
					}
					goto again;
				}
				return ex_lval.id->lex;
			}
			return ex_lval.op = c;
		}
 eof:
	ex->eof = 1;
	return ex_lval.op = ';';
}
