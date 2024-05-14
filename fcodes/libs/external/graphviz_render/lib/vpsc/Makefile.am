## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_srcdir)/lib

if WITH_IPSEPCOLA
noinst_LTLIBRARIES = libvpsc_C.la
endif

noinst_HEADERS = \
	block.h blocks.h variable.h constraint.h \
	generate-constraints.h \
	solve_VPSC.h csolve_VPSC.h pairingheap/PairingHeap.h \
	pairingheap/dsexceptions.h

libvpsc_C_la_SOURCES = \
	block.cpp blocks.cpp constraint.cpp generate-constraints.cpp \
	pairingheap/PairingHeap.cpp \
	solve_VPSC.cpp csolve_VPSC.cpp variable.cpp

EXTRA_DIST = vpsc.vcxproj*

