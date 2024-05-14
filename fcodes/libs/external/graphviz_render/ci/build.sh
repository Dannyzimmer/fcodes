#!/usr/bin/env bash

set -e
set -o pipefail
set -u
set -x

if [ -f /etc/os-release ]; then
    cat /etc/os-release
    . /etc/os-release
    if [ "${OSTYPE}" = "msys" ]; then
        # MSYS2/MinGW doesn't have VERSION_ID in /etc/os-release
        VERSION_ID=$( uname -r )
    fi
else
    ID=$( uname -s )
    # remove trailing text after actual version
    VERSION_ID=$( uname -r | sed "s/\([0-9\.]*\).*/\1/")
fi

# FIXME: the build system sets redundant RPATHs which trigger rpmbuild errors on
# Fedora ≥ 35, so suppress this rpmbuild check
# https://gitlab.com/graphviz/graphviz/-/issues/2163
if [ "${ID}" = "fedora" ]; then
  export QA_RPATHS=$(( 0x0001 ))
fi

META_DATA_DIR=Metadata/${ID}/${VERSION_ID}
mkdir -p ${META_DATA_DIR}
DIR=$(pwd)/Packages/${ID}/${VERSION_ID}
mkdir -p ${DIR}
ARCH=$( uname -m )
build_system=${build_system:-autotools}
if [ "${build_system}" = "cmake" ]; then
    cmake --version
    mkdir build
    pushd build
    cmake --log-level=VERBOSE --warn-uninitialized -Werror=dev \
      ${CMAKE_OPTIONS:-} ..
    cmake --build .
    cpack
    popd
    if [ "${OSTYPE}" = "linux-gnu" ]; then
        GV_VERSION=$(python3 gen_version.py)
        if [ "${ID_LIKE:-}" = "debian" ]; then
            mv build/Graphviz-${GV_VERSION}-Linux.deb ${DIR}/graphviz-${GV_VERSION}-cmake.deb
        else
            mv build/Graphviz-${GV_VERSION}-Linux.rpm ${DIR}/graphviz-${GV_VERSION}-cmake.rpm
        fi
    elif [[ "${OSTYPE}" =~ "darwin" ]]; then
        mv build/*.zip ${DIR}/
    elif [ "${OSTYPE}" = "msys" ]; then
        mv build/*.zip ${DIR}/
        mv build/*.exe ${DIR}/
    elif [[ "${OSTYPE}" =~ "cygwin" ]]; then
        mv build/*.zip ${DIR}/
        mv build/*.tar.bz2 ${DIR}/
    else
        echo "Error: OSTYPE=${OSTYPE} is unknown" >&2
        exit 1
    fi
elif [[ "${CONFIGURE_OPTIONS:-}" =~ "--enable-static" ]]; then
    GV_VERSION=$( cat GRAPHVIZ_VERSION )
    if [ "${use_autogen:-no}" = "yes" ]; then
        ./autogen.sh
        ./configure ${CONFIGURE_OPTIONS:-} --prefix=$( pwd )/build | tee >(./ci/extract-configure-log.sh >${META_DATA_DIR}/configure.log)
        make
        make install
        tar cf - -C build . | xz -9 -c - > ${DIR}/graphviz-${GV_VERSION}-${ARCH}.tar.xz
    else
        tar xfz graphviz-${GV_VERSION}.tar.gz
        pushd graphviz-${GV_VERSION}
        ./configure $CONFIGURE_OPTIONS --prefix=$( pwd )/build | tee >(../ci/extract-configure-log.sh >../${META_DATA_DIR}/configure.log)
        make
        make install
        popd
    fi
else
    GV_VERSION=$( cat GRAPHVIZ_VERSION )
    if [ "$OSTYPE" = "linux-gnu" ]; then
        if [ "${ID_LIKE:-}" = "debian" ]; then
            tar xfz graphviz-${GV_VERSION}.tar.gz
            (cd graphviz-${GV_VERSION}; fakeroot make -f debian/rules binary) | tee >(ci/extract-configure-log.sh >${META_DATA_DIR}/configure.log)
            tar cf - *.deb *.ddeb | xz -9 -c - >${DIR}/graphviz-${GV_VERSION}-debs.tar.xz
        else
            rm -rf ${HOME}/rpmbuild
            rpmbuild -ta graphviz-${GV_VERSION}.tar.gz | tee >(ci/extract-configure-log.sh >${META_DATA_DIR}/configure.log)
            pushd ${HOME}/rpmbuild/RPMS
            mv */*.rpm ./
            tar cf - *.rpm | xz -9 -c - >${DIR}/graphviz-${GV_VERSION}-rpms.tar.xz
            popd
        fi
    elif [[ "${OSTYPE}" =~ "darwin" ]]; then
        ./autogen.sh
        ./configure --prefix=$( pwd )/build --with-quartz=yes
        make
        make install
        tar cfz ${DIR}/graphviz-${GV_VERSION}-${ARCH}.tar.gz --options gzip:compression-level=9 build
    elif [ "${OSTYPE}" = "cygwin" -o "${OSTYPE}" = "msys" ]; then
        if [ "${OSTYPE}" = "msys" ]; then
            # ensure that MinGW tcl shell is used in order to find tcl functions
            CONFIGURE_OPTIONS="${CONFIGURE_OPTIONS:-} --with-tclsh=${MSYSTEM_PREFIX}/bin/tclsh86"
        else # Cygwin
            # avoid platform detection problems
            CONFIGURE_OPTIONS="${CONFIGURE_OPTIONS:-} --build=x86_64-pc-cygwin"
        fi
        if [ "${use_autogen:-no}" = "yes" ]; then
            ./autogen.sh
            ./configure ${CONFIGURE_OPTIONS:-} --prefix=$( pwd )/build | tee >(./ci/extract-configure-log.sh >${META_DATA_DIR}/configure.log)
            make
            make install
            tar cf - -C build . | xz -9 -c - > ${DIR}/graphviz-${GV_VERSION}-${ARCH}.tar.xz
        else
            tar xfz graphviz-${GV_VERSION}.tar.gz
            pushd graphviz-${GV_VERSION}
            ./configure ${CONFIGURE_OPTIONS:-} --prefix=$( pwd )/build | tee >(../ci/extract-configure-log.sh >../${META_DATA_DIR}/configure.log)
            make
            make install
            popd
            tar cf - -C graphviz-${GV_VERSION}/build . | xz -9 -c - > ${DIR}/graphviz-${GV_VERSION}-${ARCH}.tar.xz
        fi
    else
        echo "Error: OSTYPE=${OSTYPE} is unknown" >&2
        exit 1
    fi
fi
