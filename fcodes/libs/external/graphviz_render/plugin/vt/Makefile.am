AM_CPPFLAGS = \
  -I$(top_srcdir)/lib \
  -I$(top_srcdir)/lib/cdt \
  -I$(top_srcdir)/lib/cgraph \
  -I$(top_srcdir)/lib/common \
  -I$(top_srcdir)/lib/gvc \
  -I$(top_srcdir)/lib/pathplan

noinst_LTLIBRARIES = libgvplugin_vt_C.la
if WITH_WIN32
lib_LTLIBRARIES = libgvplugin_vt.la
else
pkglib_LTLIBRARIES = libgvplugin_vt.la
endif

libgvplugin_vt_C_la_SOURCES = gvplugin_vt.c

libgvplugin_vt_la_LDFLAGS = -version-info $(GVPLUGIN_VERSION_INFO)
libgvplugin_vt_la_SOURCES = $(libgvplugin_vt_C_la_SOURCES)
libgvplugin_vt_la_LIBADD = \
  $(top_builddir)/lib/gvc/libgvc.la \
  $(top_builddir)/lib/cgraph/libcgraph.la \
  $(top_builddir)/lib/cdt/libcdt.la

if WITH_WIN32
libgvplugin_vt_la_LDFLAGS += -no-undefined
endif

if WITH_DARWIN9
libgvplugin_vt_la_LDFLAGS += -Wl,-exported_symbol,_gvplugin_vt_LTX_library
endif
