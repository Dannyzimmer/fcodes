"""test ../lib/cgraph/ internal generic utility code"""

import os
import platform
import sys
from pathlib import Path

import pytest

sys.path.append(os.path.dirname(__file__))
from gvtest import run_c  # pylint: disable=wrong-import-position


@pytest.mark.parametrize("utility", ("bitarray", "list", "stack", "tokenize"))
def test_utility(utility: str):
    """run the given utility’s unit tests"""

    # locate the unit tests
    src = Path(__file__).parent.resolve() / f"../lib/cgraph/test_{utility}.c"
    assert src.exists()

    # locate lib directory that needs to be in the include path
    lib = Path(__file__).parent.resolve() / "../lib"

    # extra C flags this compilation needs
    cflags = ["-I", lib]
    if platform.system() != "Windows":
        cflags += ["-std=gnu99", "-Wall", "-Wextra", "-Werror"]

    _, _ = run_c(src, cflags=cflags)
