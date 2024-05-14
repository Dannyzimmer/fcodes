  /* By default, Flex emits a lexer using symbols prefixed with "yy". Graphviz
   * contains multiple Flex-generated lexers, so we alter this prefix to avoid
   * symbol clashes.
   */
%option prefix="gml"

  /* Avoid generating an unused input function. See
     https://westes.github.io/flex/manual/Scanner-Options.html
   */
%option noinput

%{
#include <assert.h>
#include <cgraph/alloc.h>
#include <stdlib.h>
#include <string.h>
#include <gml2gv.h>
#include <gmlparse.h>
#include "config.h"

#define GRAPH_EOF_TOKEN     '@'     /* lex class must be defined below */

static int line_num = 1;
static int errors;
static FILE* Ifile;

void initgmlscan(FILE *ifile) 
{ 
    if (ifile) {
	Ifile = ifile; 
	line_num = 1;
    }
    errors = 0;
}

#ifndef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
        if ((result = fread(buf, 1, max_size, Ifile)) < 0) \
                YY_FATAL_ERROR( "input in flex scanner failed" )
#endif

/* buffer for arbitrary length strings (longer than BUFSIZ) */
static char *Sbuf;

static void beginstr(void) {
  assert(Sbuf == NULL && "leaking memory");
  Sbuf = gv_strdup("");
}

static void addstr(const char *src) {
  assert(Sbuf != NULL && "missing beginstr()");

  // enlarge the buffer to make room for the suffix
  {
    size_t old_size = strlen(Sbuf) + 1;
    size_t new_size = old_size + strlen(src);
    Sbuf = gv_realloc(Sbuf, old_size, new_size);
  }

  strcat(Sbuf, src);
}

static void endstr(void) {
  assert(Sbuf != NULL && "missing beginstr()");

  // take ownership of the Sbuf backing memory
  gmllval.str = Sbuf;
  Sbuf = NULL;
}

%}
GRAPH_EOF_TOKEN                         [@]
ASCII     [ !#$%\047-\177]
DIGIT     [0-9]
L_INT     [-+]?{DIGIT}+
MANTISSA  E[-+]?{DIGIT}
L_REAL    [-+]?{DIGIT}*\.{DIGIT}*{MANTISSA}?
L_ID      [a-zA-Z_][_a-zA-Z0-9]*
%x qstring
%%
{GRAPH_EOF_TOKEN}           return(EOF);
<INITIAL,qstring>\n         line_num++;
^"#".*                      /* ignore # line */
[ \t\r]                     /* ignore whitespace */

"graph"	                    return (GRAPH);
"node"                      return (NODE);
"edge"                      return (EDGE);
"directed"                  return (DIRECTED);
"id"                        return (ID);
"source"                    return (SOURCE);
"target"                    return (TARGET);
"x"                         return (XVAL);
"y"                         return (YVAL);
"w"                         return (WVAL);
"h"                         return (HVAL);
"label"                     return (LABEL);
"graphics"                  return (GRAPHICS);;
"LabelGraphics"             return (LABELGRAPHICS);
"type"                      return (TYPE);
"fill"                      return (FILL);
"outline"                   return (OUTLINE);
"outlineStyle"              return (OUTLINESTYLE);
"outlineWidth"              return (OUTLINEWIDTH);
"width"                     return (WIDTH);
"style"                     return (STYLE);
"Line"                      return (LINE);
"point"                     return (POINT);
"text"                      return (TEXT);
"fontSize"                  return (FONTSIZE);
"fontName"                  return (FONTNAME);
"color"                     return (COLOR);
{L_INT}                     { gmllval.str = gv_strdup(yytext); return (INTEGER); }
{L_REAL}                    { gmllval.str = gv_strdup(yytext); return (REAL); }
{L_ID}                      { gmllval.str = gv_strdup(yytext); return (NAME); }
["]                         BEGIN(qstring); beginstr();

<qstring>["]                BEGIN(INITIAL); endstr(); return (STRING);
<qstring>([^"]+)            addstr(yytext);

.                           return (yytext[0]);

%%

void gmlerror(const char *str)
{
    if (errors)
	return;
    errors = 1;
    agerr(AGWARN, " %s in line %d near '%s'\n", str,line_num,yytext);
}

int gmlerrors(void)
{
    return errors;
}
 
void gmllexeof(void) { unput(GRAPH_EOF_TOKEN); }

#ifndef YY_CALL_ONLY_ARG
# define YY_CALL_ONLY_ARG void
#endif

int yywrap(YY_CALL_ONLY_ARG)
{
    return 1;
}

