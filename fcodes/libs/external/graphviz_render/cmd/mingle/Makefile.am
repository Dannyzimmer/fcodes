## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/common \
	-I$(top_srcdir)/lib/cgraph \
	-I$(top_srcdir)/lib/cdt

if WITH_ANN
bin_PROGRAMS = mingle
man_MANS = mingle.1
if ENABLE_MAN_PDFS
pdf_DATA = mingle.1.pdf
endif
endif

mingle_SOURCES = minglemain.cpp
mingle_CPPFLAGS = $(AM_CPPFLAGS)
mingle_LDADD = \
	$(top_builddir)/lib/mingle/libmingle_C.la \
	$(top_builddir)/lib/ingraphs/libingraphs_C.la \
	$(top_builddir)/lib/sfdpgen/libsfdpgen_C.la \
	$(top_builddir)/lib/neatogen/libneatogen_C.la \
	$(top_builddir)/lib/sparse/libsparse_C.la \
	$(top_builddir)/lib/rbtree/librbtree_C.la \
	$(top_builddir)/lib/common/libcommon_C.la \
	$(top_builddir)/lib/cgraph/libcgraph.la \
	$(top_builddir)/lib/cdt/libcdt.la \
	$(ANN_LIBS) -lm

.1.1.pdf:
	rm -f $@; pdffile=$@; psfile=$${pdffile%pdf}ps; \
	$(GROFF) -Tps -man $< > $$psfile || { rm -f $$psfile; exit 1; }; \
	$(PS2PDF) $$psfile && rm -f $$psfile || { rm -f $$psfile; exit 1; }
SUFFIXES = .1 .1.pdf

EXTRA_DIST = mingle.1 mingle.vcxproj*

DISTCLEANFILES = $(pdf_DATA)
