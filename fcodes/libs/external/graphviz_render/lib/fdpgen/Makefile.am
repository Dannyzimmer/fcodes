## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/common \
	-I$(top_srcdir)/lib/gvc \
	-I$(top_srcdir)/lib/pack \
	-I$(top_srcdir)/lib/pathplan \
	-I$(top_srcdir)/lib/cgraph \
	-I$(top_srcdir)/lib/cdt

if WITH_WIN32
AM_CFLAGS = -DNEATOGEN_EXPORTS=1
endif

noinst_HEADERS = comp.h dbg.h fdp.h grid.h tlayout.h xlayout.h \
	clusteredges.h
noinst_LTLIBRARIES = libfdpgen_C.la

libfdpgen_C_la_SOURCES = comp.c dbg.c grid.c fdpinit.c layout.c \
	tlayout.c xlayout.c clusteredges.c

EXTRA_DIST = gvfdpgen.vcxproj*
