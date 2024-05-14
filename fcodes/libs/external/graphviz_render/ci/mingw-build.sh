#!/usr/bin/env bash

set -e
set -o pipefail
set -u
set -x

ci/mingw-install.sh

export PATH=$PATH:/c/Git/cmd

if [ "${build_system}" = "cmake" ]; then
    # Use the libs installed with pacman instead of those in
    # https://gitlab.com/graphviz/graphviz-windows-dependencies.
    export CMAKE_OPTIONS="-Duse_win_pre_inst_libs=OFF"
fi

ci/build.sh
