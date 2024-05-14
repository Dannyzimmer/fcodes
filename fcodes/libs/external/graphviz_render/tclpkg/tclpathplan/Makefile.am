## Process this file with automake to produce Makefile.in

SUBDIRS = demo

pkgtcldir = $(pkglibdir)/tcl

noinst_HEADERS = makecw.h simple.h
if WITH_TCL
pkgtcl_LTLIBRARIES = libtclplan.la
endif

libtclplan_la_SOURCES = find_ints.c intersect.c makecw.c tclpathplan.c wrapper.c
libtclplan_la_CPPFLAGS = \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/tclpkg/tclhandle \
	-I$(top_srcdir)/lib/pathplan $(TCL_INCLUDES)
libtclplan_la_CFLAGS = $(TCL_CFLAGS)
libtclplan_la_LDFLAGS = -no-undefined -module -avoid-version
libtclplan_la_LIBADD = \
	$(top_builddir)/tclpkg/tclhandle/libtclhandle_C.la \
	$(top_builddir)/tclpkg/tclstubs/libtclstubs_C.la \
	$(top_builddir)/lib/pathplan/libpathplan.la $(MATH_LIBS) \
	-lc $(TCL_STUB_LIB_SPEC)

if WITH_TCL
all-local: pkgIndex.tcl

pkgIndex.tcl: libtclplan.la
	sh $(top_srcdir)/tclpkg/mkpkgindex.sh libtclplan.la Tclpathplan $(VERSION)
endif

CLEANFILES = pkgIndex.tcl

EXTRA_DIST = $(libtclplan_la_SOURCES)
