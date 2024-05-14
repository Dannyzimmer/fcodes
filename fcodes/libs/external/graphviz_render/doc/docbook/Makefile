SRCS = \
       src/book.xml \
       src/bookinfo.xml \
       src/graphviz_plugins.xml \
       src/graphviz_scripting.xml

.PHONY: all
all: html pdf

dvi fo javahelp man pdf ps txt: $(SRCS)
	xmlto $@ src/book.xml


xhtml xhtml-nochunks html html-nochunks htmlhelp: $(SRCS)
	xmlto $@ -o $@ src/book.xml
	cp -rp src/imgs $@/

.PHONY: clean
clean:
	rm -rf xhtml xhtml-nochunks html html-nochunks htmlhelp book.*
