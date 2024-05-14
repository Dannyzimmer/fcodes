#!/usr/bin/env bash

# this script does something close to the work flow end users may follow when
# building Graphviz

set -e
set -o pipefail
set -u
set -x

# output some info for debugging
uname -rms
cat /etc/os-release

GV_VERSION=$(cat GRAPHVIZ_VERSION)

# unpack the portable source tarball
tar xfz graphviz-${GV_VERSION}.tar.gz

# setup a directory for building in
mkdir build
cd build
# ensure that MinGW tcl shell is used in order to find tcl functions
../graphviz-${GV_VERSION}/configure ${CONFIGURE_OPTIONS:-} --with-tclsh=${MSYSTEM_PREFIX}/bin/tclsh86
