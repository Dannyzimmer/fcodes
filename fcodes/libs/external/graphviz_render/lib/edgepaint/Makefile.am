## Process this file with automake to produce Makefile.in

LAB_GAMUT_VERSION = "1:0:0"

AM_CPPFLAGS = \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/common \
	-I$(top_srcdir)/lib/gvc \
	-I$(top_srcdir)/lib/pathplan \
	-I$(top_srcdir)/lib/cgraph \
	-I$(top_srcdir)/lib/cdt

noinst_HEADERS = edge_distinct_coloring.h furtherest_point.h intersection.h lab_gamut.h lab.h node_distinct_coloring.h

noinst_LTLIBRARIES = libedgepaint_C.la
lib_LTLIBRARIES = liblab_gamut.la
pkgconfig_DATA = liblab_gamut.pc

libedgepaint_C_la_SOURCES = edge_distinct_coloring.c furtherest_point.c intersection.c lab.c node_distinct_coloring.c

liblab_gamut_la_LDFLAGS = -version-info $(LAB_GAMUT_VERSION) -no-undefined
liblab_gamut_la_SOURCES = lab_gamut.c

EXTRA_DIST = edgepaintlib.vcxproj* lab_gamut.vcxproj*
