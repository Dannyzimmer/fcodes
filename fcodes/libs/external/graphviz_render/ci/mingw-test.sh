#!/usr/bin/env bash

set -e
set -o pipefail
set -u
set -x

ci/mingw-install.sh
python3 -m pip install --requirement requirements.txt

export PATH=$PATH:/c/Git/cmd

if [ "${build_system}" = "cmake" ]; then
    DIR_REL="$(echo build/_CPack_Packages/win64/NSIS/Graphviz-*-win[36][24])"
else
    echo "Error: ${build_system} is not yet supported" >&2
    exit 1
fi

# we need the absolete path since pytest cd somewhere else

# we need the Win32 value of the physical directory since somehow the
# symbolic one is not understood in the -L flag. In the -I flag the
# path gets mysteriously translated and works anyway
DIR_WABS="C:/Graphviz"

# we need the logical value of the directory for the PATH
DIR_LABS="/c/Graphviz"

# needed to find headers and libs at compile time. Must use absolute
# Windows path for libs (why?)
export CFLAGS="-I$DIR_LABS/include"
export LDFLAGS="-L$DIR_LABS/lib"

# needed to find e.g. libgvc.dll at run time. Windows does not use
# LD_LIBRARY_PATH. Must be the logical directory
export PATH="${PATH}:$DIR_LABS/bin"

python gen_version.py --output GRAPHVIZ_VERSION
export GV_VERSION=$( cat GRAPHVIZ_VERSION )

python3 -m pytest --verbose ci/tests.py tests
