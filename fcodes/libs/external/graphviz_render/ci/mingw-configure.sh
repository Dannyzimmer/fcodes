#!/usr/bin/env bash

set -e
set -o pipefail
set -u
set -x

ci/mingw-install.sh

ci/out-of-source-configure.sh
