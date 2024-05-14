## Process this file with automake to produce Makefile.in

GVPR_VERSION = "2:0:0"

if WITH_MINGW
# Add an extra forward slash in front of $(pkgdatadir) to avoid the
# path being translated to use backslashes under MSYS2/MinGW. This
# trick is needed since DFLT_GVPRPATH is a string which is compiled
# and backslashes in strings are interpreted as escape sequences. See
# https://stackoverflow.com/a/14189687
DFLT_GVPRPATH = .$(PATH_SEPARATOR)/$(pkgdatadir)/gvpr
else
DFLT_GVPRPATH = .$(PATH_SEPARATOR)$(pkgdatadir)/gvpr
endif

AM_CPPFLAGS = \
	-I$(top_srcdir)/lib \
	-I$(top_builddir)/lib \
	-I$(top_srcdir)/lib/common \
	-I$(top_builddir)/lib/common \
	-I$(top_srcdir)/lib/cgraph \
	-I$(top_srcdir)/lib/cdt \
	-DDFLT_GVPRPATH='"$(DFLT_GVPRPATH)"'

pkginclude_HEADERS = gvpr.h
noinst_HEADERS = actions.h compile.h gdefs.h gprdata.inc gprstate.h parse.h \
	queue.h trieFA.h trie.c
noinst_LTLIBRARIES = libgvpr_C.la
lib_LTLIBRARIES = libgvpr.la
pkgconfig_DATA = libgvpr.pc
dist_man_MANS = gvpr.3
if ENABLE_MAN_PDFS
pdf_DATA = gvpr.3.pdf
endif

libgvpr_C_la_SOURCES = actions.c compile.c gvpr.c gprstate.c parse.c queue.c
libgvpr_C_la_DEPENDENCIES = \
	$(top_builddir)/lib/expr/libexpr_C.la \
	$(top_builddir)/lib/common/libcommon_C.la \
	$(top_builddir)/lib/ingraphs/libingraphs_C.la

.3.3.pdf:
	rm -f $@; pdffile=$@; psfile=$${pdffile%pdf}ps; \
	$(GROFF) -Tps -man $< > $$psfile || { rm -f $$psfile; exit 1; }; \
	$(PS2PDF) $$psfile && rm -f $$psfile || { rm -f $$psfile; exit 1; }
SUFFIXES = .3 .3.pdf

libgvpr_la_LDFLAGS = -version-info $(GVPR_VERSION) -no-undefined
libgvpr_la_SOURCES = $(libgvpr_C_la_SOURCES)
libgvpr_la_LIBADD = $(libgvpr_C_la_LIBADD) \
	$(top_builddir)/lib/ingraphs/libingraphs_C.la \
	$(top_builddir)/lib/expr/libexpr_C.la \
	$(top_builddir)/lib/cdt/libcdt.la \
	$(top_builddir)/lib/cgraph/libcgraph.la \
	$(MATH_LIBS)
libgvpr_la_DEPENDENCIES = $(libgvpr_C_la_DEPENDENCIES)

if WITH_WIN32
libgvpr_la_LDFLAGS += -export-symbols-regex gvpr
endif

EXTRA_DIST = trie.c gvprlib.vcxproj*

DISTCLEANFILES = $(pdf_DATA)
