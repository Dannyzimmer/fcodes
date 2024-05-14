## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_srcdir)/lib

noinst_HEADERS = tclhandle.h
if WITH_TCL
noinst_LTLIBRARIES = libtclhandle_C.la
endif

libtclhandle_C_la_SOURCES = tclhandle.c
libtclhandle_C_la_LDFLAGS = -module -avoid-version
