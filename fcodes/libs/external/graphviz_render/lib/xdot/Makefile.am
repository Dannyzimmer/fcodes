# ## Process this file with automake to produce Makefile.in

XDOT_VERSION = "4:0:0"

pkginclude_HEADERS = xdot.h

AM_CPPFLAGS = -I$(top_srcdir)/lib

noinst_LTLIBRARIES = libxdot_C.la
lib_LTLIBRARIES = libxdot.la
pkgconfig_DATA = libxdot.pc

dist_man_MANS = xdot.3
if ENABLE_MAN_PDFS
pdf_DATA = xdot.3.pdf
endif

libxdot_C_la_SOURCES = xdot.c
libxdot_la_LDFLAGS = -version-info $(XDOT_VERSION) -no-undefined
libxdot_la_SOURCES = $(libxdot_C_la_SOURCES)

.3.3.pdf:
	rm -f $@; pdffile=$@; psfile=$${pdffile%pdf}ps; \
	$(GROFF) -Tps -man $< > $$psfile || { rm -f $$psfile; exit 1; }; \
	$(PS2PDF) $$psfile && rm -f $$psfile || { rm -f $$psfile; exit 1; }
SUFFIXES = .3 .3.pdf

EXTRA_DIST = xdot.vcxproj*

DISTCLEANFILES = $(pdf_DATA)
