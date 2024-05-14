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
 * D. G. Korn
 * G. S. Fowler
 * AT&T Research
 *
 * match shell file patterns -- derived from Bourne and Korn shell gmatch()
 *
 *	sh pattern	egrep RE	description
 *	----------	--------	-----------
 *	*		.*		0 or more chars
 *	?		.		any single char
 *	[.]		[.]		char class
 *	[!.]		[^.]		negated char class
 *	[[:.:]]		[[:.:]]		ctype class
 *	[[=.=]]		[[=.=]]		equivalence class
 *	[[...]]		[[...]]		collation element
 *	*(.)		(.)*		0 or more of
 *	+(.)		(.)+		1 or more of
 *	?(.)		(.)?		0 or 1 of
 *	(.)		(.)		1 of
 *	@(.)		(.)		1 of
 *	a|b		a|b		a or b
 *	\#				() subgroup back reference [1-9]
 *	a&b				a and b
 *	!(.)				none of
 *
 * \ used to escape metacharacters
 *
 *	*, ?, (, |, &, ), [, \ must be \'d outside of [...]
 *	only ] must be \'d inside [...]
 *
 * BUG: unbalanced ) terminates top level pattern
 */

#include <ast/ast.h>
#include <cgraph/strview.h>
#include <ctype.h>
#include <stddef.h>
#include <string.h>

#ifndef	isblank
#define	isblank(x)	((x)==' '||(x)=='\t')
#endif

#ifndef isgraph
#define	isgraph(x)	(isprint(x)&&!isblank(x))
#endif

#ifdef _DEBUG_MATCH
#include <ast/error.h>
#endif

#define MAXGROUP	10

typedef struct {
    char *beg[MAXGROUP];
    char *end[MAXGROUP];
    char *next_s;
    int groups;
} Group_t;

typedef struct {
    Group_t current;
    Group_t best;
    char *last_s;
    char *next_p;
} Match_t;

#if defined(_lib_mbtowc) && _lib_mbtowc && MB_LEN_MAX > 1
#define mbgetchar(p)	((ast.locale.set&AST_LC_multibyte)?((ast.tmp_int=mbtowc(&ast.tmp_wchar,p,MB_CUR_MAX))>=0?((p+=ast.tmp_int),ast.tmp_wchar):0):(*p++))
#else
#define mbgetchar(p)	(*p++)
#endif

#define getsource(s,e)	(((s)>=(e))?0:mbgetchar(s))

/*
 * gobble chars up to <sub> or ) keeping track of (...) and [...]
 * sub must be one of { '|', '&', 0 }
 * 0 returned if s runs out
 */

static char *gobble(Match_t * mp, char *s, int sub,
		    int *g, int clear)
{
    int p = 0;
    char *b = 0;
    int c = 0;
    int n;

    for (;;)
	switch (mbgetchar(s)) {
	case '\\':
	    if (mbgetchar(s))
		break;
	 /*FALLTHROUGH*/ case 0:
	    return 0;
	case '[':
	    if (!b) {
		if (*s == '!')
		    (void)mbgetchar(s);
		b = s;
	    } else if (*s == '.' || *s == '=' || *s == ':')
		c = *s;
	    break;
	case ']':
	    if (b) {
		if (*(s - 2) == c)
		    c = 0;
		else if (b != (s - 1))
		    b = 0;
	    }
	    break;
	case '(':
	    if (!b) {
		p++;
		n = (*g)++;
		if (clear) {
		    if (!sub)
			n++;
		    if (n < MAXGROUP)
			mp->current.beg[n] = mp->current.end[n] = 0;
		}
	    }
	    break;
	case ')':
	    if (!b && p-- <= 0)
		return sub ? 0 : s;
	    break;
	case '|':
	    if (!b && !p && sub == '|')
		return s;
	    break;
	default: // nothing required
	    break;
	}
}

static int grpmatch(Match_t *, int, char *, char *, char *, int);

#ifdef _DEBUG_MATCH
#define RETURN(v)	{error_info.indent--;return (v);}
#else
#define RETURN(v)	{return (v);}
#endif

/*
 * match a single pattern
 * e is the end (0) of the substring in s
 * r marks the start of a repeated subgroup pattern
 */

static int
onematch(Match_t * mp, int g, char *s, char *p, char *e, char *r,
	 int flags)
{
    int pc;
    int sc;
    int n;
    int icase;
    char *olds;
    char *oldp;

#ifdef _DEBUG_MATCH
    error_info.indent++;
    error(-1, "onematch g=%d s=%-.*s p=%s r=%p flags=%o", g, e - s, s, p,
	  r, flags);
#endif
    icase = flags & STR_ICASE;
    do {
	olds = s;
	sc = getsource(s, e);
	if (icase && isupper(sc))
	    sc = tolower(sc);
	oldp = p;
	switch (pc = mbgetchar(p)) {
	case '(':
	case '*':
	case '?':
	case '+':
	case '@':
	case '!':
	    if (pc == '(' || *p == '(') {
		char *subp;
		int oldg;

		s = olds;
		subp = p + (pc != '(');
		oldg = g;
		n = ++g;
		if (g < MAXGROUP && (!r || g > mp->current.groups))
		    mp->current.beg[g] = mp->current.end[g] = 0;
		if (!(p = gobble(mp, subp, 0, &g, !r)))
		    RETURN(0);
		if (pc == '*' || pc == '?' || (pc == '+' && oldp == r)) {
		    if (onematch(mp, g, s, p, e, NULL, flags))
			RETURN(1);
		    if (!sc || !getsource(s, e)) {
			mp->current.groups = oldg;
			RETURN(0);
		    }
		}
		if (pc == '*' || pc == '+') {
		    p = oldp;
		    sc = n - 1;
		} else
		    sc = g;
		pc = (pc != '!');
		do {
		    if (grpmatch(mp, n, olds, subp, s, flags) == pc) {
			if (n < MAXGROUP) {
			    if (!mp->current.beg[n]
				|| mp->current.beg[n] > olds)
				mp->current.beg[n] = olds;
			    if (s > mp->current.end[n])
				mp->current.end[n] = s;
#ifdef _DEBUG_MATCH
			    error(-4,
				  "subgroup#%d n=%d beg=%p end=%p len=%d",
				  __LINE__, n, mp->current.beg[n],
				  mp->current.end[n],
				  mp->current.end[n] - mp->current.beg[n]);
#endif
			}
			if (onematch(mp, sc, s, p, e, oldp, flags)) {
			    if (p == oldp && n < MAXGROUP) {
				if (!mp->current.beg[n]
				    || mp->current.beg[n] > olds)
				    mp->current.beg[n] = olds;
				if (s > mp->current.end[n])
				    mp->current.end[n] = s;
#ifdef _DEBUG_MATCH
				error(-4,
				      "subgroup#%d n=%d beg=%p end=%p len=%d",
				      __LINE__, n, mp->current.beg[n],
				      mp->current.end[n],
				      mp->current.end[n] -
				      mp->current.beg[n]);
#endif
			    }
			    RETURN(1);
			}
		    }
		} while (s < e && mbgetchar(s));
		mp->current.groups = oldg;
		RETURN(0);
	    } else if (pc == '*') {
		/*
		 * several stars are the same as one
		 */

		while (*p == '*' && *(p + 1) != '(')
		    p++;
		oldp = p;
		switch (pc = mbgetchar(p)) {
		case '@':
		case '!':
		case '+':
		    n = *p == '(';
		    break;
		case '(':
		case '[':
		case '?':
		case '*':
		    n = 1;
		    break;
		case 0:
		case '|':
		case '&':
		case ')':
		    mp->current.next_s = (flags & STR_MAXIMAL) ? e : olds;
		    mp->next_p = oldp;
		    mp->current.groups = g;
		    if (!pc
			&& (!mp->best.next_s
			    || ((flags & STR_MAXIMAL)
				&& mp->current.next_s > mp->best.next_s)
			    || (!(flags & STR_MAXIMAL)
				&& mp->current.next_s <
				mp->best.next_s))) {
			mp->best = mp->current;
#ifdef _DEBUG_MATCH
			error(-3, "best#%d groups=%d next=\"%s\"",
			      __LINE__, mp->best.groups, mp->best.next_s);
#endif
		    }
		    RETURN(1);
		case '\\':
		    if (!(pc = mbgetchar(p)))
			RETURN(0);
		    if (pc >= '0' && pc <= '9') {
			n = pc - '0';
#ifdef _DEBUG_MATCH
			error(-2,
			      "backref#%d n=%d g=%d beg=%p end=%p len=%d",
			      __LINE__, n, g, mp->current.beg[n],
			      mp->current.end[n],
			      mp->current.end[n] - mp->current.beg[n]);
#endif
			if (n <= g && mp->current.beg[n])
			    pc = *mp->current.beg[n];
		    }
		/*FALLTHROUGH*/ default:
		    if (icase && isupper(pc))
			pc = tolower(pc);
		    n = 0;
		    break;
		}
		p = oldp;
		for (;;) {
		    if ((n || pc == sc)
			&& onematch(mp, g, olds, p, e, NULL, flags))
			RETURN(1);
		    if (!sc)
			RETURN(0);
		    olds = s;
		    sc = getsource(s, e);
		    if ((flags & STR_ICASE) && isupper(sc))
			sc = tolower(sc);
		}
	    } else if (pc != '?' && pc != sc)
		RETURN(0);
	    break;
	case 0:
	    if (!(flags & STR_MAXIMAL))
		sc = 0;
	 /*FALLTHROUGH*/ case '|':
	case '&':
	case ')':
	    if (!sc) {
		mp->current.next_s = olds;
		mp->next_p = oldp;
		mp->current.groups = g;
	    }
	    if (!pc
		&& (!mp->best.next_s
		    || ((flags & STR_MAXIMAL) && olds > mp->best.next_s)
		    || (!(flags & STR_MAXIMAL)
			&& olds < mp->best.next_s))) {
		mp->best = mp->current;
		mp->best.next_s = olds;
		mp->best.groups = g;
#ifdef _DEBUG_MATCH
		error(-3, "best#%d groups=%d next=\"%s\"", __LINE__,
		      mp->best.groups, mp->best.next_s);
#endif
	    }
	    RETURN(!sc);
	case '[':
	    {
		/*UNDENT... */

		int invert;
		int x;
		int ok = 0;
		char *range;

		if (!sc)
		    RETURN(0);
		range = 0;
		n = 0;
		if ((invert = (*p == '!')))
		    p++;
		for (;;) {
		    oldp = p;
		    if (!(pc = mbgetchar(p))) {
			RETURN(0);
		    } else if (pc == '['
			       && (*p == ':' || *p == '=' || *p == '.')) {
			n = mbgetchar(p);
			oldp = p;
			strview_t callee = {.data = oldp};
			for (;;) {
			    if (!(pc = mbgetchar(p)))
				RETURN(0);
			    if (pc == n && *p == ']')
				break;
			    ++callee.size;
			}
			(void)mbgetchar(p);
			if (ok)
			    /*NOP*/;
			else if (n == ':') {
			    if (strview_str_eq(callee, "alnum")) {
				if (isalnum(sc))
				    ok = 1;
			    } else if (strview_str_eq(callee, "alpha")) {
				if (isalpha(sc))
				    ok = 1;
			    } else if (strview_str_eq(callee, "blank")) {
				if (isblank(sc))
				    ok = 1;
			    } else if (strview_str_eq(callee, "cntrl")) {
				if (iscntrl(sc))
				    ok = 1;
			    } else if (strview_str_eq(callee, "digit")) {
				if (isdigit(sc))
				    ok = 1;
			    } else if (strview_str_eq(callee, "graph")) {
				if (isgraph(sc))
				    ok = 1;
			    } else if (strview_str_eq(callee, "lower")) {
				if (islower(sc))
				    ok = 1;
			    } else if (strview_str_eq(callee, "print")) {
				if (isprint(sc))
				    ok = 1;
			    } else if (strview_str_eq(callee, "punct")) {
				if (ispunct(sc))
				    ok = 1;
			    } else if (strview_str_eq(callee, "space")) {
				if (isspace(sc))
				    ok = 1;
			    } else if (strview_str_eq(callee, "upper")) {
				if (icase ? islower(sc) : isupper(sc))
				    ok = 1;
			    } else if (strview_str_eq(callee, "xdigit")) {
				if (isxdigit(sc))
				    ok = 1;
			    }
			}
			else if (range)
			    goto getrange;
			else if (*p == '-' && *(p + 1) != ']') {
			    (void)mbgetchar(p);
			    range = oldp;
			} else
			    if ((isalpha((int)*oldp) && isalpha((int)*olds)
				 && tolower(*oldp) == tolower(*olds))
				|| sc == mbgetchar(oldp))
			    ok = 1;
			n = 1;
		    } else if (pc == ']' && n) {
			if (ok != invert)
			    break;
			RETURN(0);
		    } else if (pc == '\\'
			       && (oldp = p, !(pc = mbgetchar(p)))) {
			RETURN(0);
		    } else if (ok)
			/*NOP*/;
		    else if (range)
		    {
		      getrange:
#if defined(_lib_mbtowc) && _lib_mbtowc
			if (ast.locale.set & AST_LC_multibyte) {
			    wchar_t sw;
			    wchar_t bw;
			    wchar_t ew;
			    int sz;
			    int bz;
			    int ez;

			    sz = mbtowc(&sw, olds, MB_CUR_MAX);
			    bz = mbtowc(&bw, range, MB_CUR_MAX);
			    ez = mbtowc(&ew, oldp, MB_CUR_MAX);
			    if (sw == bw || sw == ew)
				ok = 1;
			    else if (sz > 1 || bz > 1 || ez > 1) {
				if (sz == bz && sz == ez && sw > bw
				    && sw < ew)
				    ok = 1;
				else
				    RETURN(0);
			    }
			}
			if (!ok)
#endif
			    if (icase && isupper(pc))
				pc = tolower(pc);
			x = mbgetchar(range);
			if (icase && isupper(x))
			    x = tolower(x);
			if (sc == x || sc == pc || (sc > x && sc < pc))
			    ok = 1;
			if (*p == '-' && *(p + 1) != ']') {
			    (void)mbgetchar(p);
			    range = oldp;
			} else
			    range = 0;
			n = 1;
		    } else if (*p == '-' && *(p + 1) != ']') {
			(void)mbgetchar(p);
			range = oldp;
			n = 1;
		    } else {
			if (icase && isupper(pc))
			    pc = tolower(pc);
			if (sc == pc)
			    ok = 1;
			n = pc;
		    }
		}

		/*...INDENT */
	    }
	    break;
	case '\\':
	    if (!(pc = mbgetchar(p)))
		RETURN(0);
	    if (pc >= '0' && pc <= '9') {
		n = pc - '0';
#ifdef _DEBUG_MATCH
		error(-2, "backref#%d n=%d g=%d beg=%p end=%p len=%d",
		      __LINE__, n, g, mp->current.beg[n],
		      mp->current.end[n],
		      mp->current.end[n] - mp->current.beg[n]);
#endif
		if (n <= g && (oldp = mp->current.beg[n])) {
		    while (oldp < mp->current.end[n])
			if (!*olds || *olds++ != *oldp++)
			    RETURN(0);
		    s = olds;
		    break;
		}
	    }
	/*FALLTHROUGH*/ default:
	    if (icase && isupper(pc))
		pc = tolower(pc);
	    if (pc != sc)
		RETURN(0);
	    break;
	}
    } while (sc);
    RETURN(0);
}

/*
 * match any pattern in a group
 * | and & subgroups are parsed here
 */

static int
grpmatch(Match_t * mp, int g, char *s, char *p, char *e,
	 int flags)
{
    char *a;

#ifdef _DEBUG_MATCH
    error_info.indent++;
    error(-1, "grpmatch g=%d s=%-.*s p=%s flags=%o", g, e - s, s, p,
	  flags);
#endif
    do {
	for (a = p; onematch(mp, g, s, a, e, NULL, flags); a++)
	    if (*(a = mp->next_p) != '&')
		RETURN(1);
    } while ((p = gobble(mp, p, '|', &g, 1)));
    RETURN(0);
}

/*
 * subgroup match
 * 0 returned if no match
 * otherwise number of subgroups matched returned
 * match group begin offsets are even elements of sub
 * match group end offsets are odd elements of sub
 * the matched string is from s+sub[0] up to but not
 * including s+sub[1]
 */

int strgrpmatch(const char *b, const char *p, int *sub, int n, int flags)
{
    int i;
    char *s;
    char *e;
    Match_t match;

    s = (char *) b;
    match.last_s = e = s + strlen(s);
    for (;;) {
	match.best.next_s = 0;
	match.current.groups = 0;
	match.current.beg[0] = 0;
	if ((i = grpmatch(&match, 0, s, (char *) p, e, flags)) || match.best.next_s) {
	    if (!(flags & STR_RIGHT) || (match.current.next_s == e)) {
		if (!i)
		    match.current = match.best;
		match.current.groups++;
		match.current.end[0] = match.current.next_s;
#ifdef _DEBUG_MATCH
		error(-1, "match i=%d s=\"%s\" p=\"%s\" flags=%o groups=%d next=\"%s\"",
		      i, s, p, flags, match.current.groups, match.current.next_s);
#endif
		break;
	    }
	}
	if ((flags & STR_LEFT) || s >= e)
	    return 0;
	s++;
    }
    if ((flags & STR_RIGHT) && match.current.next_s != e)
	return 0;
    if (!sub)
	return 1;
    match.current.beg[0] = s;
    s = (char *) b;
    if (n > match.current.groups)
	n = match.current.groups;
    for (i = 0; i < n; i++) {
	sub[i * 2] = match.current.end[i] ? match.current.beg[i] - s : 0;
	sub[i * 2 + 1] = match.current.end[i] ? match.current.end[i] - s : 0;
    }
    return n;
}

/*
 * compare the string s with the shell pattern p
 * returns 1 for match 0 otherwise
 */

int strmatch(const char *s, const char *p)
{
    return strgrpmatch(s, p, NULL, 0, STR_MAXIMAL | STR_LEFT | STR_RIGHT);
}
