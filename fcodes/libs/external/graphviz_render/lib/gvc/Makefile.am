## Process this file with automake to produce Makefile.in

GVC_VERSION = "6:0:0"

AM_CPPFLAGS = \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/common \
	-I$(top_srcdir)/lib/pathplan \
	-I$(top_srcdir)/lib/cgraph \
	-I$(top_srcdir)/lib/cdt \
	$(INCLTDL) -DGVLIBDIR='"$(pkglibdir)"'

if WITH_WIN32
AM_CFLAGS = -DGVC_EXPORTS=1
endif

LIBS = $(Z_LIBS) $(MATH_LIBS)

pkginclude_HEADERS = gvc.h gvcext.h gvplugin.h gvcjob.h \
	gvcommon.h gvplugin_render.h gvplugin_layout.h gvconfig.h \
	gvplugin_textlayout.h gvplugin_device.h gvplugin_loadimage.h
noinst_HEADERS = gvcint.h gvcproc.h gvio.h
noinst_LTLIBRARIES = libgvc_C.la
lib_LTLIBRARIES = libgvc.la
pkgconfig_DATA = libgvc.pc
dist_man_MANS = gvc.3
if ENABLE_MAN_PDFS
pdf_DATA = gvc.3.pdf
endif

libgvc_C_la_SOURCES = gvrender.c gvlayout.c gvdevice.c gvloadimage.c \
	gvcontext.c gvjobs.c gvevent.c gvplugin.c gvconfig.c \
	gvtool_tred.c gvtextlayout.c gvusershape.c gvc.c

libgvc_C_la_LIBADD = \
	$(top_builddir)/lib/pack/libpack_C.la \
	$(top_builddir)/lib/label/liblabel_C.la \
	$(top_builddir)/lib/common/libcommon_C.la
libgvc_C_la_DEPENDENCIES = \
	$(top_builddir)/lib/pack/libpack_C.la \
	$(top_builddir)/lib/label/liblabel_C.la \
	$(top_builddir)/lib/common/libcommon_C.la
if WITH_ORTHO
libgvc_C_la_LIBADD += $(top_builddir)/lib/ortho/libortho_C.la
libgvc_C_la_DEPENDENCIES += $(top_builddir)/lib/ortho/libortho_C.la
endif
if ENABLE_LTDL
libgvc_C_la_LIBADD += $(LIBLTDL) $(LIBLTDL_LDFLAGS)
endif

libgvc_la_LDFLAGS = -version-info $(GVC_VERSION) -no-undefined
libgvc_la_SOURCES = $(libgvc_C_la_SOURCES)
libgvc_la_LIBADD = $(libgvc_C_la_LIBADD) \
	$(top_builddir)/lib/cdt/libcdt.la \
	$(top_builddir)/lib/cgraph/libcgraph.la \
	$(top_builddir)/lib/pathplan/libpathplan.la \
	$(EXPAT_LIBS) $(Z_LIBS) $(MATH_LIBS)
libgvc_la_DEPENDENCIES = $(libgvc_C_la_DEPENDENCIES)

.3.3.pdf:
	rm -f $@; pdffile=$@; psfile=$${pdffile%pdf}ps; \
	$(GROFF) -Tps -man $< > $$psfile || { rm -f $$psfile; exit 1; }; \
	$(PS2PDF) $$psfile && rm -f $$psfile || { rm -f $$psfile; exit 1; }
SUFFIXES = .3 .3.pdf

DISTCLEANFILES = $(pdf_DATA)
