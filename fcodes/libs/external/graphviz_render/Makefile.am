## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

export TAR_OPTIONS = --owner=0 --group=0 --exclude=windows/dependencies/*/*

dist_doc_DATA = AUTHORS COPYING CHANGELOG.md NEWS cpl1.0.txt

pkginclude_HEADERS = $(top_builddir)/graphviz_version.h

dist_man_MANS = graphviz.7

# $(subdirs) contains the list from: AC_CONFIG_SUBDIRS
SUBDIRS = $(subdirs) lib plugin cmd tclpkg doc contrib share graphs tests

.PHONY: doxygen
doxygen:
	doxygen

dist-hook:
	echo "$(VERSION)" >GRAPHVIZ_VERSION

BUILT_SOURCES = $(top_builddir)/graphviz_version.h

$(top_builddir)/graphviz_version.h: config.h
	printf '#pragma once\n' > $(top_builddir)/graphviz_version.h
	$(EGREP) 'PACKAGE|VERSION|GVPLUGIN' config.h >> $(top_builddir)/graphviz_version.h

EXTRA_DIST = graphviz.spec \
	config/depcomp config/config.rpath \
	builddate.h \
	m4/README m4/lib-ld.m4 m4/lib-link.m4 m4/lib-prefix.m4 \
	Doxyfile.in Doxyfile \
	config/config_perl.pl \
	config/config_ruby.rb config/config_tcl.tcl dot.demo plugin.demo \
	macosx windows debian redhat graphviz.sln

.PHONY: rpm
rpm: dist
	mkdir -p $(HOME)/rpmbuild/$(HOSTNAME)/SPECS $(HOME)/rpmbuild/$(HOSTNAME)/SOURCES
	cp graphviz-$(VERSION).tar.gz $(HOME)/rpmbuild/$(HOSTNAME)/SOURCES/
	case `cut -d' ' -f1 /etc/system-release` in \
	Fedora) cp redhat/graphviz.spec.fedora $(HOME)/rpmbuild/$(HOSTNAME)/SPECS/graphviz.spec;; \
	CentOS) cp redhat/graphviz.spec.rhel $(HOME)/rpmbuild/$(HOSTNAME)/SPECS/graphviz.spec;; \
	Red) cp redhat/graphviz.spec.rhel $(HOME)/rpmbuild/$(HOSTNAME)/SPECS/graphviz.spec;; \
	esac
	rpmbuild -ba $(HOME)/rpmbuild/$(HOSTNAME)/SPECS/graphviz.spec

.PHONY: deb
deb: dist
	rm -rf graphviz-$(VERSION)
	zcat graphviz-$(VERSION).tar.gz | tar xf -
	(cd graphviz-$(VERSION); fakeroot make -f debian/rules binary)

.PHONY: pkg
pkg: dist
	tar xzf graphviz-$(VERSION).tar.gz && make -C graphviz-$(VERSION)/macosx/build && mv graphviz-$(VERSION)/macosx/build/graphviz-*.pkg .

DISTCLEANFILES = config.cache builddate.h FEATURE/* graphviz-$(VERSION)*
