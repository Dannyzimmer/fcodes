## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_srcdir)/lib

noinst_HEADERS = vmalloc.h
noinst_LTLIBRARIES = libvmalloc_C.la

libvmalloc_C_la_SOURCES = vmalloc.c vmclear.c vmclose.c \
	vmopen.c \
	vmstrdup.c

EXTRA_DIST = vmalloc.vcxproj*
