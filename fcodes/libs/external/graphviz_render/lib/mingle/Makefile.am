## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/common \
	-I$(top_srcdir)/lib/gvc \
	-I$(top_srcdir)/lib/pathplan \
	-I$(top_srcdir)/lib/cgraph \
	-I$(top_srcdir)/lib/cdt $(ANN_CFLAGS)

noinst_HEADERS = edge_bundling.h ink.h agglomerative_bundling.h nearest_neighbor_graph.h nearest_neighbor_graph_ann.h

if WITH_ANN
noinst_LTLIBRARIES = libmingle_C.la
endif

libmingle_C_la_SOURCES = edge_bundling.cpp ink.cpp agglomerative_bundling.cpp \
	nearest_neighbor_graph.cpp nearest_neighbor_graph_ann.cpp

EXTRA_DIST = minglelib.vcxproj*
