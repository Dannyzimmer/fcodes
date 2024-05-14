## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/cgraph \
	-I$(top_srcdir)/lib/cdt \
	-I$(top_srcdir)/lib/gvc \
	-I$(top_srcdir)/lib/xdot \
	-I$(top_srcdir)/lib/gvpr \
	-I$(top_srcdir)/lib/common \
	-I$(top_srcdir)/lib/pathplan \
	-I$(top_srcdir)/cmd/smyrna/gui \
	$(GTK_CFLAGS:-I%=-isystem%) \
	$(GLUT_CFLAGS) $(GTKGLEXT_CFLAGS) $(GLADE_CFLAGS) $(FREETYPE2_CFLAGS) $(GTS_CFLAGS) $(XRENDER_CFLAGS)

bin_PROGRAMS =
if ENABLE_SHARED
bin_PROGRAMS += smyrna
endif
if ENABLE_STATIC
bin_PROGRAMS += smyrna_static
endif

dist_man_MANS = smyrna.1
if ENABLE_MAN_PDFS
pdf_DATA = smyrna.1.pdf
endif

noinst_HEADERS = arcball.h draw.h glexpose.h \
	glmotion.h gltemplate.h gui/appmouse.h gui/callbacks.h \
	hotkeymap.h polytess.h selectionfuncs.h \
	smyrna_utils.h smyrnadefs.h topfisheyeview.h \
	topviewfuncs.h tvnodes.h \
	viewport.h viewportcamera.h support.h \
	gui/datalistcallbacks.h gui/frmobjectui.h \
	gui/glcompui.h gui/gui.h gui/menucallbacks.h \
	gui/toolboxcallbacks.h gui/topviewsettings.h gvprpipe.h hier.h glutrender.h

smyrna_SOURCES = arcball.c draw.c glexpose.c \
	glmotion.c gltemplate.c gui/appmouse.c gui/callbacks.c \
	gvprpipe.c hier.c hotkeymap.c main.c polytess.c \
	selectionfuncs.c smyrna_utils.c topfisheyeview.c \
	topviewfuncs.c tvnodes.c \
	viewport.c viewportcamera.c \
	gui/datalistcallbacks.c gui/frmobjectui.c \
	gui/glcompui.c gui/gui.c gui/menucallbacks.c \
	gui/toolboxcallbacks.c gui/topviewsettings.c glutrender.c

smyrna_LDADD = $(top_builddir)/lib/cgraph/libcgraph_C.la \
	$(top_builddir)/lib/cdt/libcdt_C.la \
	$(top_builddir)/lib/xdot/libxdot_C.la \
	$(top_builddir)/lib/glcomp/libglcomp_C.la \
	$(top_builddir)/lib/topfish/libtopfish_C.la \
	$(top_builddir)/lib/common/libcommon_C.la \
	$(top_builddir)/lib/gvc/libgvc_C.la \
	$(top_builddir)/lib/gvpr/libgvpr_C.la \
	$(top_builddir)/lib/expr/libexpr_C.la \
	$(top_builddir)/lib/ingraphs/libingraphs_C.la \
	$(top_builddir)/lib/neatogen/libneatogen_C.la \
	$(top_builddir)/lib/pathplan/libpathplan_C.la \
	$(GTK_LIBS) $(GLUT_LIBS) $(GTKGLEXT_LIBS) $(GLADE_LIBS) $(X_LIBS) $(EXPAT_LIBS) $(GTS_LIBS) $(MATH_LIBS) $(EXTRA_SMYRNA_LDFLAGS)

smyrna_static_SOURCES = $(smyrna_SOURCES)
smyrna_static_LDADD = $(top_builddir)/lib/cgraph/libcgraph_C.la \
	$(top_builddir)/lib/cdt/libcdt_C.la \
	$(top_builddir)/lib/xdot/libxdot_C.la \
	$(top_builddir)/lib/glcomp/libglcomp_C.la \
	$(top_builddir)/lib/topfish/libtopfish_C.la \
	$(top_builddir)/lib/common/libcommon_C.la \
	$(top_builddir)/lib/gvc/libgvc_C.la \
	$(top_builddir)/lib/gvpr/libgvpr_C.la \
	$(top_builddir)/lib/expr/libexpr_C.la \
	$(top_builddir)/lib/ingraphs/libingraphs_C.la \
	$(top_builddir)/lib/neatogen/libneatogen_C.la \
	$(top_builddir)/lib/pathplan/libpathplan_C.la \
	$(GTK_LIBS) $(GLUT_LIBS) $(GTKGLEXT_LIBS) $(GLADE_LIBS) $(X_LIBS) $(EXPAT_LIBS) $(GTS_LIBS) $(MATH_LIBS)

.1.1.pdf:
	rm -f $@; pdffile=$@; psfile=$${pdffile%pdf}ps; \
	$(GROFF) -Tps -man $< > $$psfile || { rm -f $$psfile; exit 1; }; \
	$(PS2PDF) $$psfile && rm -f $$psfile || { rm -f $$psfile; exit 1; }
SUFFIXES = .1 .1.pdf

EXTRA_DIST = smyrna.vcxproj*

DISTCLEANFILES = $(pdf_DATA)
