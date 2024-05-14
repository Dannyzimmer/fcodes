## Process this file with automake to produce Makefile.in

PATHPLAN_VERSION = "4:0:0"

AM_CPPFLAGS = -I$(top_srcdir)/lib

if WITH_WIN32
AM_CFLAGS = -DPATHPLAN_EXPORTS
endif

pkginclude_HEADERS = pathgeom.h pathplan.h
noinst_HEADERS = pathutil.h solvers.h tri.h vis.h vispath.h

noinst_LTLIBRARIES = libpathplan_C.la
lib_LTLIBRARIES = libpathplan.la
pkgconfig_DATA = libpathplan.pc
dist_man_MANS = pathplan.3
if ENABLE_MAN_PDFS
pdf_DATA = pathplan.3.pdf
endif

libpathplan_C_la_SOURCES = cvt.c inpoly.c route.c shortest.c \
	shortestpth.c solvers.c triang.c util.c visibility.c

libpathplan_la_LDFLAGS = -version-info $(PATHPLAN_VERSION) -no-undefined
libpathplan_la_SOURCES = $(libpathplan_C_la_SOURCES)
libpathplan_la_LIBADD = $(MATH_LIBS)

.3.3.pdf:
	rm -f $@; pdffile=$@; psfile=$${pdffile%pdf}ps; \
	$(GROFF) -Tps -man $< > $$psfile || { rm -f $$psfile; exit 1; }; \
	$(PS2PDF) $$psfile && rm -f $$psfile || { rm -f $$psfile; exit 1; }
SUFFIXES = .3 .3.pdf

EXTRA_DIST = Pathplan.vcxproj*

DISTCLEANFILES = $(pdf_DATA)
