## Process this file with automake to produce Makefile.in

demosharpdir = $(demodir)
demoguiledir = $(demodir)
demoiodir = $(demodir)
demojavadir = $(demodir)
demoluadir = $(demodir)
demoocamldir = $(demodir)
demoperldir = $(demodir)
demophpdir = $(demodir)
demopython3dir = $(demodir)
demoRdir = $(demodir)
demorubydir = $(demodir)
demotcldir = $(demodir)

if INSTALL_DEMOS
if WITH_SHARP
demosharp_SCRIPTS =
endif
if WITH_GUILE
demoguile_SCRIPTS =
endif
if WITH_JAVA
demojava_SCRIPTS =
endif
if WITH_LUA
demolua_SCRIPTS = modgraph.lua
endif
if WITH_OCAML
demoocaml_SCRIPTS =
endif
if WITH_PERL
demoperl_SCRIPTS = modgraph.pl
endif
if WITH_PHP
demophp_SCRIPTS = modgraph.php
endif
if WITH_PYTHON3
demopython3_SCRIPTS = modgraph.py
endif
if WITH_R
demoR_SCRIPTS =
endif
if WITH_RUBY
demoruby_SCRIPTS = modgraph.rb
endif
if WITH_TCL
demotcl_SCRIPTS = modgraph.tcl
endif
endif

EXTRA_DIST = modgraph.*
