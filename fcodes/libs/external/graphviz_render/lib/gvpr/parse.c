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
 * Top-level parsing of gpr code into blocks
 *
 */

#include <ast/ast.h>
#include <ast/error.h>
#include <cgraph/agxbuf.h>
#include <cgraph/alloc.h>
#include <cgraph/unreachable.h>
#include <gvpr/parse.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static int lineno = 1;		/* current line number */
static int col0 = 1;		/* true if char ptr is at column 0 */
static int startLine = 1;	/* set to start line of bracketd content */
static int kwLine = 1;		/* set to line of keyword */

static char *case_str[] = {
    "BEGIN",
    "END",
    "BEG_G",
    "END_G",
    "N",
    "E",
    "EOF",
    "ERROR",
};

/* caseStr:
 * Convert case_t to string.
 */
static char *caseStr(case_t cs)
{
    return case_str[(int) cs];
}

/* eol:
 * Eat characters until eol.
 */
static int eol(FILE *str) {
    int c;
    while ((c = getc(str)) != '\n') {
	if (c < 0)
	    return c;
    }
    lineno++;
    col0 = 1;
    return c;
}

/* readc:
 * return character from input stream
 * while keeping track of line number.
 * Strip out comments, and return space or newline.
 * If a newline is seen in comment and ostr
 * is non-null, add newline to ostr.
 */
static int readc(FILE *str, agxbuf *ostr) {
    int c;
    int cc;

    switch (c = getc(str)) {
    case '\n':
	lineno++;
        col0 = 1;
	break;
    case '#':
	if (col0) { /* shell comment */
	    c = eol (str);
	}
	else col0 = 0;
	break;
    case '/':
	cc = getc(str);
	switch (cc) {
	case '*':		/* in C comment   */
	    while (1) {
		switch (c = getc(str)) {
		case '\n':
		    lineno++;
		    if (ostr)
			agxbputc(ostr, (char)c);
		    break;
		case '*':
		    switch (cc = getc(str)) {
		    case -1:
			return cc;
			break;
		    case '\n':
			lineno++;
			if (ostr)
			    agxbputc(ostr, (char)cc);
			break;
		    case '*':
			ungetc(cc, str);
			break;
		    case '/':
			col0 = 0;
			return ' ';
			break;
		    }
		}
	    }
	    break;
	case '/':		/* in C++ comment */
	    c = eol (str);
	    break;
	default:		/* not a comment  */
	    if (cc >= '\0')
		ungetc(cc, str);
	    break;
	}
	break;
    default:
        col0 = 0;
	break;
    }
    return c;
}

/* unreadc;
 * push character back onto stream;
 * if newline, reduce lineno.
 */
static void unreadc(FILE *str, int c) {
    ungetc(c, str);
    if (c == '\n')
	lineno--;
}

/* skipWS:
 */
static int skipWS(FILE *str) {
    int c;

    while (true) {
	c = readc(str, 0);
	if (!isspace(c)) {
	    return c;
	}
    }
}

/* parseID:
 * Put initial alpha in buffer;
 * add additional alphas, up to buffer size.
 */
static void parseID(FILE *str, int c, char *buf, size_t bsize) {
    char *ptr = buf;
    char *eptr = buf + (bsize - 1);

    *ptr++ = (char)c;
    while (true) {
	c = readc(str, 0);
	if (c < 0)
	    break;
	if (isalpha(c) || c == '_') {
	    if (ptr == eptr)
		break;
	    *ptr++ = (char)c;
	} else {
	    unreadc(str, c);
	    break;
	}
    }
    *ptr = '\0';

}

#define BSIZE 8

/* parseKind:
 * Look for keywords: BEGIN, END, BEG_G, END_G, N, E
 * As side-effect, sets kwLine to line of keyword.
 */
static case_t parseKind(FILE *str) {
    int c;
    char buf[BSIZE];
    case_t cs = Error;

    c = skipWS(str);
    if (c < 0)
	return Eof;
    if (!isalpha(c)) {
	error(ERROR_ERROR,
	      "expected keyword BEGIN/END/N/E...; found '%c', line %d", c,
	      lineno);
	return Error;
    }

    kwLine = lineno;
    parseID(str, c, buf, BSIZE);
    if (strcmp(buf, "BEGIN") == 0) {
	cs = Begin;
    } else if (strcmp(buf, "BEG_G") == 0) {
	cs = BeginG;
    } else if (strcmp(buf, "E") == 0) {
	cs = Edge;
    } else if (strcmp(buf, "END") == 0) {
	cs = End;
    } else if (strcmp(buf, "END_G") == 0) {
	cs = EndG;
    } else if (strcmp(buf, "N") == 0) {
	cs = Node;
    }
    if (cs == Error)
	error(ERROR_ERROR, "unexpected keyword \"%s\", line %d", buf, kwLine);
    return cs;
}

/* endString:
 * eat characters from ins, putting them into outs,
 * up to and including a terminating character ec
 * that is not escaped with a back quote.
 */
static int endString(FILE *ins, agxbuf *outs, char ec) {
    int sline = lineno;
    int c;

    while ((c = getc(ins)) != ec) {
	if (c == '\\') {
	    agxbputc(outs, (char)c);
	    c = getc(ins);
	}
	if (c < 0) {
	    error(ERROR_ERROR, "unclosed string, start line %d", sline);
	    return c;
	}
	if (c == '\n')
	    lineno++;
	agxbputc(outs, (char) c);
    }
    agxbputc(outs, (char)c);
    return 0;
}

/* endBracket:
 * eat characters from ins, putting them into outs,
 * up to a terminating character ec.
 * Strings are treated as atomic units: any ec in them
 * is ignored. Since matching bc-ec pairs might nest,
 * the function is called recursively.
 */
static int endBracket(FILE *ins, agxbuf *outs, char bc, char ec) {
    int c;

    while (true) {
	c = readc(ins, outs);
	if (c < 0 || c == ec)
	    return c;
	else if (c == bc) {
	    agxbputc(outs, (char) c);
	    c = endBracket(ins, outs, bc, ec);
	    if (c < 0)
		return c;
	    else
		agxbputc(outs, (char) c);
	} else if (c == '\'' || c == '"') {
	    agxbputc(outs, (char) c);
	    if (endString(ins, outs, (char)c)) return -1;
	} else
	    agxbputc(outs, (char) c);
    }
}

/* parseBracket:
 *  parse paired expression : bc <string> ec
 *  returning <string>
 * As a side-effect, set startLine to beginning of content.
 */
static char *parseBracket(FILE *str, agxbuf *buf, int bc, int ec) {
    int c;

    c = skipWS(str);
    if (c < 0)
	return 0;
    if (c != bc) {
	unreadc(str, c);
	return 0;
    }
    startLine = lineno;
    c = endBracket(str, buf, bc, (char)ec);
    if (c < 0) {
	if (!getErrorErrors())
	    error(ERROR_ERROR,
	      "unclosed bracket %c%c expression, start line %d", bc, ec,
	      startLine);
	return 0;
    }
    else
	return agxbdisown(buf);
}

/* parseAction:
 */
static char *parseAction(FILE *str, agxbuf *buf) {
    return parseBracket(str, buf, '{', '}');
}

/* parseGuard:
 */
static char *parseGuard(FILE *str, agxbuf *buf) {
    return parseBracket(str, buf, '[', ']');
}

/* parseCase:
 * Recognize
 *   BEGIN <optional action>
 *   END <optional action>
 *   BEG_G <optional action>
 *   END_G <optional action>
 *   N <optional guard> <optional action>
 *   E <optional guard> <optional action>
 * where
 *   guard = '[' <expr> ']'
 *   action = '{' <expr> '}'
 */
static case_t parseCase(FILE *str, char **guard, int *gline, char **action,
	  int *aline)
{
    case_t kind;

    agxbuf buf = {0};

    kind = parseKind(str);
    switch (kind) {
    case Begin:
    case BeginG:
    case End:
    case EndG:
	*action = parseAction(str, &buf);
	*aline = startLine;
	if (getErrorErrors ())
	    kind = Error;
	break;
    case Edge:
    case Node:
	*guard = parseGuard(str, &buf);
	*gline = startLine;
	if (!getErrorErrors ()) {
	    *action = parseAction(str, &buf);
	    *aline = startLine;
	}
	if (getErrorErrors ())
	    kind = Error;
	break;
    case Eof:
    case Error:		/* to silence warnings */
	break;
    default:
	UNREACHABLE();
    }

    agxbfree(&buf);
    return kind;
}

/* addBlock:
 * create new block and append to list;
 * return new item as tail
 */
static parse_block *addBlock (parse_block * last, char *stmt, int line,
	int n_nstmts, case_info *nodelist, int n_estmts, case_info *edgelist)
{
    parse_block* item = gv_alloc(sizeof(parse_block));

    item->l_beging = line;
    item->begg_stmt = stmt;
    item->n_nstmts = n_nstmts;
    item->n_estmts = n_estmts;
    item->node_stmts = nodelist;
    item->edge_stmts = edgelist;
    if (last)
	last->next = item;

    return item;
}

/* addCase:
 * create new case_info and append to list;
 * return new item as tail
 */
static case_info *addCase(case_info * last, char *guard, int gline,
			  char *action, int line, int *cnt)
{
    if (!guard && !action) {
	error(ERROR_WARNING,
	      "Case with neither guard nor action, line %d - ignored", kwLine);
	return last;
    }

    ++(*cnt);
    case_info *item = gv_alloc(sizeof(case_info));
    item->guard = guard;
    item->action = action;
    item->next = NULL;
    if (guard)
	item->gstart = gline;
    if (action)
	item->astart = line;

    if (last)
	last->next = item;

    return item;
}

/* bindAction:
 *
 */
static void
bindAction(case_t cs, char *action, int aline, char **ap, int *lp)
{
    if (!action)
	error(ERROR_WARNING, "%s with no action, line %d - ignored",
	      caseStr(cs), kwLine);
    else if (*ap)
	error(ERROR_ERROR, "additional %s section, line %d", caseStr(cs),
	      kwLine);
    else {
	*ap = action;
	*lp = aline;
    }
}

/* parseProg:
 * Parses input into gpr sections.
 */
parse_prog *parseProg(char *input, int isFile)
{
    FILE *str;
    char *guard = NULL;
    char *action = NULL;
    bool more;
    parse_block *blocklist = NULL;
    case_info *edgelist = NULL;
    case_info *nodelist = NULL;
    parse_block *blockl = NULL;
    case_info *edgel = NULL;
    case_info *nodel = NULL;
    int n_blocks = 0;
    int n_nstmts = 0;
    int n_estmts = 0;
    int line = 0, gline = 0;
    int l_beging = 0;
    char *begg_stmt;

    
    lineno = col0 = startLine = kwLine = 1;
    parse_prog *prog = calloc(1, sizeof(parse_prog));
    if (!prog) {
	error(ERROR_ERROR, "parseProg: out of memory");
	return NULL;
    }

    if (isFile) {
	str = fopen(input, "r");
	prog->source = input;
	
    } else {
	str = tmpfile();
	if (str != NULL) {
	    fputs(input, str);
	    rewind(str);
	}
	prog->source = NULL;	/* command line */
    }

    if (!str) {
	if (isFile)
	    error(ERROR_ERROR, "could not open %s for reading", input);
	else
	    error(ERROR_ERROR, "parseProg : unable to create sfio stream");
	free (prog);
	return NULL;
    }
    
    begg_stmt = NULL;
    more = true;
    while (more) {
	switch (parseCase(str, &guard, &gline, &action, &line)) {
	case Begin:
	    bindAction(Begin, action, line, &prog->begin_stmt, &prog->l_begin);
	    break;
	case BeginG:
	    if (action && (begg_stmt || nodelist || edgelist)) { /* non-empty block */
		blockl = addBlock(blockl, begg_stmt, l_beging, 
		    n_nstmts, nodelist, n_estmts, edgelist);
	 	if (!blocklist)
		    blocklist = blockl;
		n_blocks++;

		/* reset values */
		n_nstmts = n_estmts = 0;
		edgel = nodel = edgelist = nodelist = NULL;
		begg_stmt = NULL;
	    }
	    bindAction(BeginG, action, line, &begg_stmt, &l_beging);
	    break;
	case End:
	    bindAction(End, action, line, &prog->end_stmt, &prog->l_end);
	    break;
	case EndG:
	    bindAction(EndG, action, line, &prog->endg_stmt, &prog->l_endg);
	    break;
	case Eof:
	    more = false;
	    break;
	case Node:
	    nodel = addCase(nodel, guard, gline, action, line, &n_nstmts);
	    if (!nodelist)
		nodelist = nodel;
	    break;
	case Edge:
	    edgel = addCase(edgel, guard, gline, action, line, &n_estmts);
	    if (!edgelist)
		edgelist = edgel;
	    break;
	case Error:		/* to silence warnings */
	    more = false;
	    break;
	default:
	    UNREACHABLE();
	}
    }

    if (begg_stmt || nodelist || edgelist) { /* non-empty block */
	blockl = addBlock(blockl, begg_stmt, l_beging, 
	    n_nstmts, nodelist, n_estmts, edgelist);
	if (!blocklist)
	    blocklist = blockl;
	n_blocks++;
    }

    prog->n_blocks = n_blocks;
    prog->blocks = blocklist;

    fclose(str);

    if (getErrorErrors ()) {
	freeParseProg (prog);
	prog = NULL;
    }

    return prog;
}

static void
freeCaseList (case_info* ip)
{
    case_info* nxt;
    while (ip) {
	nxt = ip->next;
	free (ip->guard);
	free (ip->action);
	free (ip);
	ip = nxt;
    }
}

static void
freeBlocks (parse_block* ip)
{
    parse_block* nxt;
    while (ip) {
	nxt = ip->next;
	free (ip->begg_stmt);
	freeCaseList (ip->node_stmts);
	freeCaseList (ip->edge_stmts);
	ip = nxt;
    }
}

void 
freeParseProg (parse_prog * prog)
{
    if (!prog) return;
    free (prog->begin_stmt);
    freeBlocks (prog->blocks);
    free (prog->endg_stmt);
    free (prog->end_stmt);
    free (prog);
}

