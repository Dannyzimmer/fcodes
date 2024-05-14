## Process this file with automake to produce Makefile.in

SUBDIRS = info schema

dist_pdf_DATA = dotguide.pdf neatoguide.pdf libgraph/Agraph.pdf libguide/libguide.pdf

if WITH_SMYRNA
dist_pdf_DATA += smyrna/smyrna.pdf
endif

dist_html_DATA = build.html FAQ.html index.html winbuild.html pspdf.png char.html gdtclft.entities.example.png todo.html internal_todo.html
dist_doc_DATA = Dot.ref latex_suggestions.txt

EXTRA_DIST = schema/*.xml schema/*.xslt infosrc/*
