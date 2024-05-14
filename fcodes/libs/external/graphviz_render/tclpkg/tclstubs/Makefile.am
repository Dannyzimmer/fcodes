## Process this file with automake to produce Makefile.in

if WITH_TCL
noinst_LTLIBRARIES = libtclstubs_C.la
noinst_HEADERS = tclInt.h
endif

# tclStubLib.c is a dummy file just to fool automake
libtclstubs_C_la_SOURCES = tclStubLib.c
libtclstubs_C_la_CPPFLAGS = $(TCL_CFLAGS) $(TCL_INCLUDES)
libtclstubs_C_la_LIBADD = $(TCL_STUB_LIB_SPEC)
libtclstubs_C_la_LDFLAGS = -module -avoid-version
