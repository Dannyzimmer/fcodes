FROM centos:7

COPY requirements.txt ./requirements.txt

# Development tools
RUN yum -y groupinstall 'Development Tools' \
&& yum install -y \
# Install epel-release rpm (Needed for cmake3. Must be a separate step)
https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm \
# Install ius-release rpm (Needed for git version 2. Must be a separate step)
https://repo.ius.io/ius-release-el7.rpm \
&& yum clean all && rm -rf /var/cache/yum \
# Remove old version of git
&& yum -y remove git \
# Centos build utilities
&& yum -y install \
libtool-ltdl-devel \
libedit-devel \
ksh \
cmake3 \
# Git version 2 needed to get the option --date=format-local:... used in gen_version.py
https://repo.ius.io/7/x86_64/packages/g/git236-2.36.4-1.el7.ius.x86_64.rpm \
# Option glut
freeglut-devel \
# Option ann
ann-devel \
# Option gts
# Option swig
swig \
# Command smyra
libglade2-devel \
# Command gvedit
qt5-qtbase-devel \
ncurses-devel \
# Plugin library devil
# Plugin library gd
gd-devel \
# Documentation
ghostscript \
# Plugin library
ghostscript-devel \
# Plugin library gtk
gtk2-devel \
# Plugin library lasi
# Plugin library poppler
poppler-glib-devel \
poppler-qt \
# Plugin library rsvg
librsvg2-devel \
# Language extension gv_guile
guile-devel \
# Language extension gv_java
java-devel \
# Language extension gv_lua
lua-devel \
# Language extension gv_ocaml
ocaml \
# Language extension gv_perl
perl-ExtUtils-Embed \
# Language extension gv_php
php-devel \
bzip2-devel \
openssl-devel \
libxml2-devel \
# Language extension gv_python3
python3-devel \
# Language extension gv_ruby
ruby-devel \
# Language extension gv_tcl
tcl-devel \
# Requred in redhat/graphviz.spec.fedora.in
libICE-devel \
libSM-devel \
libXaw-devel \
tk-devel \
# Test utilities
python3-pip \
python3-setuptools \
&& alternatives --install /usr/local/bin/cmake cmake /usr/bin/cmake3 20 \
--slave /usr/local/bin/ctest ctest /usr/bin/ctest3 \
--slave /usr/local/bin/cpack cpack /usr/bin/cpack3 \
--slave /usr/local/bin/ccmake ccmake /usr/bin/ccmake3 \
--family cmake \
# install Python dependencies
&& python3 -m pip install --user --upgrade pip \
&& python3 -m pip install --user --requirement requirements.txt \
# Clean up
&& yum clean all && rm -rf /var/cache/yum
