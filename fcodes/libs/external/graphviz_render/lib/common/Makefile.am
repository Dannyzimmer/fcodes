## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-I$(top_srcdir)/lib \
	-I$(top_builddir)/lib \
	-I$(top_srcdir)/libltdl \
	-I$(top_srcdir)/lib/gvc \
	-I$(top_srcdir)/lib/pack \
	-I$(top_srcdir)/lib/xdot \
	-I$(top_srcdir)/lib/pathplan \
	-I$(top_srcdir)/lib/cgraph \
	-I$(top_srcdir)/lib/cdt $(GD_INCLUDES) $(Z_INCLUDES)

if WITH_WIN32
AM_CFLAGS = -DGVC_EXPORTS
endif

BUILT_SOURCES = colortbl.h htmlparse.h

pkginclude_HEADERS = arith.h geom.h color.h types.h textspan.h usershape.h
noinst_HEADERS = boxes.h render.h utils.h memory.h \
	geomprocs.h colorprocs.h colortbl.h entities.h globals.h \
	const.h macros.h htmllex.h htmltable.h pointset.h intset.h \
	textspan_lut.h ps_font_equiv.h
noinst_LTLIBRARIES = libcommon_C.la

libcommon_C_la_SOURCES = arrows.c colxlate.c ellipse.c textspan.c textspan_lut.c \
	args.c memory.c globals.c htmllex.c htmlparse.y htmltable.c input.c \
	pointset.c intset.c postproc.c routespl.c splines.c psusershape.c \
	timing.c labels.c ns.c shapes.c utils.c geom.c taper.c \
	output.c emit.c xml.c \
	color_names
libcommon_C_la_CPPFLAGS = $(AM_CPPFLAGS) $(EXPAT_CFLAGS)
libcommon_C_la_LIBADD = \
	$(top_builddir)/lib/xdot/libxdot.la $(EXPAT_LIBS)

colxlate.o colxlate.lo : colortbl.h

colortbl.h: brewer_lib svgcolor_lib $(top_srcdir)/lib/common/color_names \
            $(top_srcdir)/lib/common/make_colortbl.py
	python3 $(top_srcdir)/lib/common/make_colortbl.py brewer_lib svgcolor_lib \
	  $(top_srcdir)/lib/common/color_names colortbl.h

brewer_lib: $(top_srcdir)/lib/common/brewer_colors \
            $(top_srcdir)/lib/common/make_brewer_lib.py
	python3 $(top_srcdir)/lib/common/make_brewer_lib.py \
	  "$(top_srcdir)/lib/common/brewer_colors" "$@"

svgcolor_lib: $(top_srcdir)/lib/common/svgcolor_names \
              $(top_srcdir)/lib/common/make_svgcolor_lib.py
	python3 $(top_srcdir)/lib/common/make_svgcolor_lib.py \
	  "$(top_srcdir)/lib/common/svgcolor_names" "$@"

htmllex.o htmllex.lo: htmllex.c htmllex.h htmlparse.h
htmlparse.o htmlparse.lo: htmlparse.c htmlparse.h

htmlparse.c htmlparse.h: $(top_srcdir)/lib/common/htmlparse.y
	$(YACC) -Wno-yacc -dv $(top_srcdir)/lib/common/htmlparse.y -o htmlparse.c

DISTCLEANFILES = brewer_lib colortbl.h \
	htmlparse.[ch] svgcolor_lib

EXTRA_DIST = README.imap \
	htmlparse.c htmlparse.h \
	entities.html entities.tcl \
	brewer_colors brewer_lib svgcolor_names svgcolor_lib \
	color_names colortbl.h \
	make_brewer_lib.py make_colortbl.py make_svgcolor_lib.py
