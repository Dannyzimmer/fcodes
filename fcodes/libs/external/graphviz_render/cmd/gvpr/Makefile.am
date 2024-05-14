## Process this file with automake to produce Makefile.in

SUBDIRS = lib

AM_CPPFLAGS = \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/gvpr \
	-I$(top_srcdir)/lib/cgraph \
	-I$(top_srcdir)/lib/cdt

if ENABLE_STATIC
bin_PROGRAMS = gvpr gvpr_static
else
bin_PROGRAMS = gvpr
endif

dist_man_MANS = gvpr.1
if ENABLE_MAN_PDFS
pdf_DATA = gvpr.1.pdf
endif

gvpr_SOURCES = gvprmain.c
gvpr_CPPFLAGS = $(AM_CPPFLAGS)
gvpr_LDADD = $(top_builddir)/lib/gvpr/libgvpr.la \
	$(top_builddir)/lib/cgraph/libcgraph.la \
	$(top_builddir)/lib/cdt/libcdt.la

.1.1.pdf:
	rm -f $@; pdffile=$@; psfile=$${pdffile%pdf}ps; \
	$(GROFF) -Tps -man $< > $$psfile || { rm -f $$psfile; exit 1; }; \
	$(PS2PDF) $$psfile && rm -f $$psfile || { rm -f $$psfile; exit 1; }
SUFFIXES = .1 .1.pdf

gvpr_static_SOURCES = gvprmain.c
gvpr_static_CPPFLAGS = $(AM_CPPFLAGS)
gvpr_static_LDADD = \
	$(top_builddir)/lib/gvpr/libgvpr_C.la \
	$(top_builddir)/lib/expr/libexpr_C.la \
	$(top_builddir)/lib/sfio/libsfio_C.la \
	$(top_builddir)/lib/sfio/Sfio_f/libsfiof_C.la \
	$(top_builddir)/lib/vmalloc/libvmalloc_C.la \
	$(top_builddir)/lib/ingraphs/libingraphs_C.la \
	$(top_builddir)/lib/ast/libast_C.la \
	$(top_builddir)/lib/cgraph/libcgraph_C.la \
	$(top_builddir)/lib/cdt/libcdt_C.la \
	$(MATH_LIBS)

EXTRA_DIST = lib gvpr.vcxproj*

DISTCLEANFILES = $(pdf_DATA)
