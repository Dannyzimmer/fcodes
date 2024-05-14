## Process this file with automake to produce Makefile.in

SUBDIRS = Sfio_f

AM_CPPFLAGS = -I$(top_srcdir)/lib

noinst_HEADERS = sfhdr.h sfio.h
noinst_LTLIBRARIES = libsfio_C.la

libsfio_C_la_SOURCES = sfcvt.c \
	sfextern.c \
	sfscanf.c \
	sftable.c sfprint.c \
	sfvscanf.c

libsfio_C_la_DEPENDENCIES = \
	Sfio_f/libsfiof_C.la

EXTRA_DIST = README sfio.vcxproj*
