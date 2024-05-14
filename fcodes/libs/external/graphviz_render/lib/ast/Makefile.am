## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_srcdir)/lib

noinst_HEADERS = ast.h error.h
noinst_LTLIBRARIES = libast_C.la

libast_C_la_SOURCES = pathpath.c chresc.c chrtoi.c error.c \
	fmtbuf.c fmtesc.c pathaccess.c pathcanon.c pathcat.c \
	pathfind.c \
	strcopy.c stresc.c strmatch.c

EXTRA_DIST = ast.vcxproj*
