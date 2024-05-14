FROM fedora:38

COPY requirements.txt ./requirements.txt

# Development tools
RUN yum -y groupinstall 'Development Tools' \
&& yum -y install rpm-build \
bison \
byacc \
flex \
# Centos build utilities
libtool-ltdl-devel \
libedit-devel \
ksh \
cmake \
# Option xpat
expat-devel \
# Option glut
freeglut-devel \
# Option ann
ann-devel \
# Option gts
gts-devel \
# Option swig
swig \
# Command smyra
# Command gvedit
qt5-qtbase-devel \
ncurses-devel \
# Plugin library devil
DevIL-devel \
# Plugin library gd
gd-devel \
# Documentation and Plugin library ghostscipt
ghostscript ghostscript-devel \
# Plugin library gtk
gtkglext-devel \
# Plugin library lasi
lasi-devel \
# Plugin library poppler
poppler-glib-devel \
poppler-qt5 \
# Plugin library rsvg
librsvg2-devel \
# Language extension gv_go
golang \
gcc-go \
# Language extension gv_guile
guile30-devel \
# Language extension gv_java
java-devel \
# Language extension gv_lua
lua-devel \
# Language extension gv_ocaml
ocaml \
# Language extension gv_perl
perl \
perl-ExtUtils-Embed \
# Language extension gv_php
php-devel \
# Language extension gv_python3
python3-devel \
# Language extension gv_R
R \
R-devel \
# Language extension gv_ruby
ruby-devel \
# Language extension gv_tcl
# Required in redhat/graphviz.spec.fedora.in
libXaw-devel \
libglade2-devel \
mono-core \
# Test utilities
python3-pip \
python3-setuptools \
# install Python dependencies
&& python3 -m pip install --user --upgrade pip \
&& python3 -m pip install --user --requirement requirements.txt \
# Clean up
&& yum clean all && rm -rf /var/cache/yum
