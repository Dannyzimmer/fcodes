#!/usr/bin/env bash

set -e
set -o pipefail
set -u
set -x

pacman -S --noconfirm --needed \
    autoconf \
    automake-wrapper \
    bison \
    ${MINGW_PACKAGE_PREFIX}-cairo \
    ${MINGW_PACKAGE_PREFIX}-cmake \
    ${MINGW_PACKAGE_PREFIX}-devil \
    ${MINGW_PACKAGE_PREFIX}-diffutils \
    ${MINGW_PACKAGE_PREFIX}-expat \
    flex \
    ${MINGW_PACKAGE_PREFIX}-gcc \
    ${MINGW_PACKAGE_PREFIX}-ghostscript \
    ${MINGW_PACKAGE_PREFIX}-gts \
    ${MINGW_PACKAGE_PREFIX}-libgd \
    make \
    ${MINGW_PACKAGE_PREFIX}-libtool \
    ${MINGW_PACKAGE_PREFIX}-librsvg \
    ${MINGW_PACKAGE_PREFIX}-libwebp \
    ${MINGW_PACKAGE_PREFIX}-pango \
    ${MINGW_PACKAGE_PREFIX}-pkgconf \
    ${MINGW_PACKAGE_PREFIX}-poppler \
    ${MINGW_PACKAGE_PREFIX}-python \
    ${MINGW_PACKAGE_PREFIX}-python-pip \
    ${MINGW_PACKAGE_PREFIX}-qt5-base \
    ${MINGW_PACKAGE_PREFIX}-mesa \
    ${MINGW_PACKAGE_PREFIX}-ninja \
    ${MINGW_PACKAGE_PREFIX}-nsis \
    ${MINGW_PACKAGE_PREFIX}-tcl \
    ${MINGW_PACKAGE_PREFIX}-zlib \
