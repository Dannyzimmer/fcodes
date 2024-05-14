## Process this file with automake to produce Makefile.in

VERSION = 0.2

AM_CPPFLAGS = \
	$(GDLIB_CFLAGS) \
	-I$(top_srcdir)/lib

if WITH_LIBGD
bin_PROGRAMS = diffimg
man_MANS = diffimg.1
if ENABLE_MAN_PDFS
pdf_DATA = diffimg.1.pdf
endif
endif

diffimg_SOURCES = diffimg.c

diffimg_LDADD = $(GDLIB_LIBS) $(MATH_LIBS)

.1.1.pdf:
	rm -f $@; pdffile=$@; psfile=$${pdffile%pdf}ps; \
	$(GROFF) -Tps -man $< > $$psfile || { rm -f $$psfile; exit 1; }; \
	$(PS2PDF) $$psfile && rm -f $$psfile || { rm -f $$psfile; exit 1; }
SUFFIXES = .1 .1.pdf

GRAPH = "digraph G { hello -> world }"

test: diffimg
	echo $(GRAPH) | dot -Tpng >hello1.png
	echo $(GRAPH) | dot -Tpng >hello2.png
	if `./diffimg hello1.png hello2.png >test1.png`;then echo same;else echo different;fi
	echo $(GRAPH) | dot -Grankdir=LR -Tpng >hello2.png
	if `./diffimg hello1.png hello2.png >test2.png`;then echo same;else echo different;fi

CLEANFILES = *.png
EXTRA_DIST = diffimg.vcxproj* diffimg.1
DISTCLEANFILES = $(pdf_DATA)
