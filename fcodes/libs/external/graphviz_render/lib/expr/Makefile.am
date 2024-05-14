## Process this file with automake to produce Makefile.in

EXPR_VERSION = "4:0:0"

AM_CPPFLAGS = \
	-I$(top_srcdir)/lib \
	-I$(top_builddir)/lib \
	-I$(top_srcdir)/lib/cdt

noinst_HEADERS = exgram.h exlib.h exop.h exparse.h expr.h
noinst_LTLIBRARIES = libexpr_C.la

dist_man_MANS = expr.3
if ENABLE_MAN_PDFS
pdf_DATA = expr.3.pdf
endif

libexpr_C_la_SOURCES = excc.c excontext.c exdata.c exerror.c \
	exeval.c exexpr.c exopen.c extoken.c \
	extype.c exzero.c exparse.y exnospace.c
libexpr_C_la_LIBADD = \
	$(top_builddir)/lib/ast/libast_C.la \
	$(top_builddir)/lib/vmalloc/libvmalloc_C.la \
	$(top_builddir)/lib/sfio/libsfio_C.la \
	$(top_builddir)/lib/sfio/Sfio_f/libsfiof_C.la \
	$(top_builddir)/lib/cdt/libcdt.la

$(libexpr_C_la_OBJECTS) $(libexpr_la_OBJECTS): \
	exparse.h exparse.c exgram.h exop.h

exparse.c exparse.h: $(top_srcdir)/lib/expr/exparse.y
	$(YACC) -Wno-yacc -dtv $(top_srcdir)/lib/expr/exparse.y -o exparse.c

.3.3.pdf:
	rm -f $@; pdffile=$@; psfile=$${pdffile%pdf}ps; \
	$(GROFF) -Tps -man $< > $$psfile || { rm -f $$psfile; exit 1; }; \
	$(PS2PDF) $$psfile && rm -f $$psfile || { rm -f $$psfile; exit 1; }
SUFFIXES = .3 .3.pdf

EXTRA_DIST = exparse.c exparse.h expr.vcxproj*

DISTCLEANFILES = exparse.c exparse.h $(pdf_DATA)
