## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/common \
	-I$(top_srcdir)/lib/cgraph \
	-I$(top_srcdir)/lib/cdt

if WITH_SFDP
bin_PROGRAMS = gvmap cluster
dist_bin_SCRIPTS = gvmap.sh
man_MANS = gvmap.1 cluster.1 gvmap.sh.1
if ENABLE_MAN_PDFS
pdf_DATA = gvmap.1.pdf cluster.1.pdf gvmap.sh.1.pdf
endif
endif

noinst_HEADERS = make_map.h power.h country_graph_coloring.h

gvmap_SOURCES = gvmap.c make_map.c power.c country_graph_coloring.c

cluster_SOURCES = cluster.c make_map.c power.c country_graph_coloring.c

gvmap_LDADD = \
	$(top_builddir)/lib/edgepaint/libedgepaint_C.la \
	$(top_builddir)/lib/sfdpgen/libsfdpgen_C.la \
	$(top_builddir)/lib/neatogen/libneatogen_C.la \
	$(top_builddir)/lib/sparse/libsparse_C.la \
	$(top_builddir)/lib/rbtree/librbtree_C.la \
	$(top_builddir)/lib/ingraphs/libingraphs_C.la \
	$(top_builddir)/lib/edgepaint/liblab_gamut.la \
	$(top_builddir)/lib/cgraph/libcgraph.la \
	$(top_builddir)/lib/gvc/libgvc.la \
	$(GTS_LIBS) -lm

cluster_LDADD = \
	$(top_builddir)/lib/edgepaint/libedgepaint_C.la \
	$(top_builddir)/lib/sfdpgen/libsfdpgen_C.la \
	$(top_builddir)/lib/neatogen/libneatogen_C.la \
	$(top_builddir)/lib/sparse/libsparse_C.la \
	$(top_builddir)/lib/rbtree/librbtree_C.la \
	$(top_builddir)/lib/ingraphs/libingraphs_C.la \
	$(top_builddir)/lib/edgepaint/liblab_gamut.la \
	$(top_builddir)/lib/cgraph/libcgraph.la \
	$(top_builddir)/lib/gvc/libgvc.la \
	$(GTS_LIBS) -lm

gvmap.sh :

.1.1.pdf:
	rm -f $@; pdffile=$@; psfile=$${pdffile%pdf}ps; \
	$(GROFF) -Tps -man $< > $$psfile || { rm -f $$psfile; exit 1; }; \
	$(PS2PDF) $$psfile && rm -f $$psfile || { rm -f $$psfile; exit 1; }
SUFFIXES = .1 .1.pdf

EXTRA_DIST = gvmap.1 cluster.1 gvmap.sh.1 gvmap.vcxproj*

DISTCLEANFILES = $(pdf_DATA)
