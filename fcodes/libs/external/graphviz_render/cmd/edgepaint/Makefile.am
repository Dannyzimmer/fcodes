## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/common \
	-I$(top_srcdir)/lib/cgraph \
	-I$(top_srcdir)/lib/cdt

bin_PROGRAMS = edgepaint

dist_man_MANS = edgepaint.1
if ENABLE_MAN_PDFS
pdf_DATA = edgepaint.1.pdf
endif

edgepaint_SOURCES = edgepaintmain.c
edgepaint_CPPFLAGS = $(AM_CPPFLAGS)
edgepaint_LDADD = \
	$(top_builddir)/lib/edgepaint/libedgepaint_C.la \
	$(top_builddir)/lib/ingraphs/libingraphs_C.la \
	$(top_builddir)/lib/sfdpgen/libsfdpgen_C.la \
	$(top_builddir)/lib/neatogen/libneatogen_C.la \
	$(top_builddir)/lib/sparse/libsparse_C.la \
	$(top_builddir)/lib/rbtree/librbtree_C.la \
	$(top_builddir)/lib/edgepaint/liblab_gamut.la \
	$(top_builddir)/lib/cgraph/libcgraph.la \
	$(top_builddir)/lib/cdt/libcdt.la \
	$(top_builddir)/lib/gvc/libgvc.la \
	-lm

# add a non-existent C++ source to force the C++ compiler to be used for
# linking, so the C++ standard library is included for our C++ dependencies
nodist_EXTRA_edgepaint_SOURCES = fake.cxx

.1.1.pdf:
	rm -f $@; pdffile=$@; psfile=$${pdffile%pdf}ps; \
	$(GROFF) -Tps -man $< > $$psfile || { rm -f $$psfile; exit 1; }; \
	$(PS2PDF) $$psfile && rm -f $$psfile || { rm -f $$psfile; exit 1; }
SUFFIXES = .1 .1.pdf

EXTRA_DIST = edgepaint.vcxproj*

DISTCLEANFILES = $(pdf_DATA)
