"""test ../lib/vmalloc"""

import os
import platform
import sys
from pathlib import Path

sys.path.append(os.path.dirname(__file__))
from gvtest import run_c  # pylint: disable=wrong-import-position


def test_vmalloc():
    """run the vmalloc unit tests"""

    # locate the vmalloc unit tests
    src = Path(__file__).parent.resolve() / "../lib/vmalloc/test.c"
    assert src.exists()

    # locate lib directory that needs to be in the include path
    lib = Path(__file__).parent.resolve() / "../lib"

    # extra C flags this compilation needs
    cflags = ["-I", lib]
    if platform.system() != "Windows":
        cflags += ["-std=gnu99", "-Wall", "-Wextra", "-Werror"]

    _, _ = run_c(src, cflags=cflags)
