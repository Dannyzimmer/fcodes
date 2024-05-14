## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/common \
	-I$(top_srcdir)/lib/pathplan \
	-I$(top_srcdir)/lib/gvc \
	-I$(top_srcdir)/lib/cgraph \
	-I$(top_srcdir)/lib/cdt

if WITH_GDIPLUS

noinst_LTLIBRARIES = libgvplugin_gdiplus_C.la
if WITH_WIN32
lib_LTLIBRARIES = libgvplugin_gdiplus.la
else
pkglib_LTLIBRARIES = libgvplugin_gdiplus.la
endif
endif

noinst_HEADERS = FileStream.h gvplugin_gdiplus.h

nodist_libgvplugin_gdiplus_C_la_SOURCES = GdiPlus*.h
libgvplugin_gdiplus_C_la_SOURCES = \
	FileStream.cpp \
	gvdevice_gdiplus.cpp \
	gvloadimage_gdiplus.cpp \
	gvplugin_gdiplus.cpp \
	gvrender_gdiplus.cpp \
	gvtextlayout_gdiplus.cpp

# libtool doesn't want to run with actual Windows import libs, so we force GdiPlus.lib through to the linker
libgvplugin_gdiplus_la_LDFLAGS = -version-info $(GVPLUGIN_VERSION_INFO) -Wl,"$(PLATFORMSDKLIB)\GdiPlus.lib"
nodist_libgvplugin_gdiplus_la_SOURCES = GdiPlus*.h
libgvplugin_gdiplus_la_SOURCES = $(libgvplugin_gdiplus_C_la_SOURCES)
libgvplugin_gdiplus_la_LIBADD = -lgdi32 -lole32 -luuid $(top_builddir)/lib/gvc/libgvc.la

if WITH_WIN32
libgvplugin_gdiplus_la_LDFLAGS += -no-undefined
endif

if WITH_DARWIN9
libgvplugin_gdiplus_la_LDFLAGS += -Wl,-exported_symbol,_gvplugin_gdiplus_LTX_library
endif

EXTRA_DIST = gvplugin_gdiplus.vcxproj*
