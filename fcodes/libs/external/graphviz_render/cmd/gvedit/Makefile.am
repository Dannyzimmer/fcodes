## Process this file with automake to produce Makefile.in

SUBDIRS = ui images

gveditdir = $(pkgdatadir)/gvedit

mans = gvedit.1

if WITH_QT
bin_PROGRAMS = gvedit
gvedit_DATA = attrs.txt
man_MANS = $(mans)
if ENABLE_MAN_PDFS
pdf_DATA = gvedit.1.pdf
endif
endif

AM_CPPFLAGS = \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/common \
	-I$(top_srcdir)/lib/gvc \
	-I$(top_srcdir)/lib/pathplan \
	-I$(top_srcdir)/lib/cgraph \
	-I$(top_srcdir)/lib/cdt

gvedit_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	$(QTGUI_CFLAGS) \
	$(QTCORE_CFLAGS) \
	-DDEMAND_LOADING=1

gvedit_LDADD = \
	$(top_builddir)/lib/gvc/libgvc.la \
	$(top_builddir)/lib/cgraph/libcgraph.la \
	$(top_builddir)/lib/cdt/libcdt.la \
	$(QTGUI_LIBS) \
	$(QTCORE_LIBS) \
	$(EXPAT_LIBS) $(Z_LIBS)

noinst_HEADERS = csettings.h imageviewer.h mainwindow.h mdichild.h ui_settings.h

MOCABLES = moc_csettings.cpp moc_imageviewer.cpp moc_mainwindow.cpp moc_mdichild.cpp qrc_mdi.cpp

gvedit_SOURCES = main.cpp csettings.cpp imageviewer.cpp mainwindow.cpp mdichild.cpp $(MOCABLES)

$(MOCABLES): mocables

.PHONY: mocables
mocables: qMakefile
	$(MAKE) -f qMakefile prefix=${prefix} exec_prefix=${exec_prefix} mocables compiler_rcc_make_all

# in case the user has both Qt4 and Qt5 installed, set QT_SELECT to force Qt5
qMakefile: gvedit.pro
	QT_SELECT=5 $(QMAKE) -o qMakefile gvedit.pro

attrs.txt: $(top_srcdir)/doc/infosrc/attrs
	cp $(top_srcdir)/doc/infosrc/attrs attrs.txt

.1.1.pdf:
	rm -f $@; pdffile=$@; psfile=$${pdffile%pdf}ps; \
	$(GROFF) -Tps -man $< > $$psfile || { rm -f $$psfile; exit 1; }; \
	$(PS2PDF) $$psfile && rm -f $$psfile || { rm -f $$psfile; exit 1; }
SUFFIXES = .1 .1.pdf

EXTRA_DIST = gvedit.vcxproj* gvedit.pro.in mdi.qrc $(mans)

CLEANFILES = moc_csettings.cpp moc_imageviewer.cpp moc_mainwindow.cpp moc_mdichild.cpp qrc_mdi.cpp
DISTCLEANFILES = attrs.txt qMakefile $(pdf_DATA)

clean-local:
	-$(MAKE) -f qMakefile clean

distclean-local:
	-$(MAKE) -f qMakefile distclean

