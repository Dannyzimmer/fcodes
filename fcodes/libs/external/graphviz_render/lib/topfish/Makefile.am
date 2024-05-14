# ## Process this file with automake to produce Makefile.in
#
AM_CPPFLAGS = \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/common

if WITH_SMYRNA
noinst_HEADERS = hierarchy.h
noinst_LTLIBRARIES = libtopfish_C.la
endif

libtopfish_C_la_SOURCES = hierarchy.c rescale_layout.c

EXTRA_DIST = topfish.vcxproj*
