## Process this file with automake to produce Makefile.in

CDT_VERSION = "5:0:0"

pkgextraincludedir = $(pkgincludedir)

AM_CPPFLAGS = -I$(top_srcdir)/lib

if WITH_WIN32
AM_CFLAGS = -DEXPORT_CDT=1
endif

pkginclude_HEADERS = cdt.h
noinst_HEADERS = dthdr.h
noinst_LTLIBRARIES = libcdt_C.la
lib_LTLIBRARIES = libcdt.la
dist_man_MANS = cdt.3
if ENABLE_MAN_PDFS
pdf_DATA = cdt.3.pdf
endif
pkgconfig_DATA = libcdt.pc

libcdt_C_la_SOURCES = dtclose.c dtdisc.c dtextract.c dtflatten.c \
	dthash.c dtlist.c dtmethod.c dtopen.c dtrenew.c dtrestore.c dtsize.c \
	dtstat.c dtstrhash.c dttree.c dtview.c dtwalk.c

libcdt_la_LDFLAGS = -version-info $(CDT_VERSION) -no-undefined
libcdt_la_SOURCES = $(libcdt_C_la_SOURCES)

.3.3.pdf:
	rm -f $@; pdffile=$@; psfile=$${pdffile%pdf}ps; \
	$(GROFF) -Tps -man $< > $$psfile || { rm -f $$psfile; exit 1; }; \
	$(PS2PDF) $$psfile && rm -f $$psfile || { rm -f $$psfile; exit 1; }
SUFFIXES = .3 .3.pdf

EXTRA_DIST = cdt.vcxproj*

DISTCLEANFILES = $(pdf_DATA)
