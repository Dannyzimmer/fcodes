"""tests that examples provided with Graphviz compile correctly"""

import os
import platform
import subprocess
import sys
from pathlib import Path

import pytest

sys.path.append(os.path.dirname(__file__))
from gvtest import run_c, which  # pylint: disable=wrong-import-position


@pytest.mark.parametrize(
    "src", ["demo.c", "dot.c", "example.c", "neatopack.c", "simple.c"]
)
# FIXME: Remove skip when
# https://gitlab.com/graphviz/graphviz/-/issues/1777 is fixed
@pytest.mark.skipif(
    os.getenv("build_system") == "msbuild",
    reason="Windows MSBuild release does not contain any header files (#1777)",
)
def test_compile_example(src):
    """try to compile the example"""

    # construct an absolute path to the example
    filepath = Path(__file__).parent.resolve() / ".." / "dot.demo" / src

    libs = ["cgraph", "gvc"]

    # run the example
    args = ["-Kneato"] if src in ["demo.c", "dot.c"] else []

    if platform.system() == "Windows":
        cflags = ["-DGVDLL"]
    else:
        cflags = None

    _, _ = run_c(filepath, args, "graph {a -- b}", cflags=cflags, link=libs)


@pytest.mark.parametrize(
    "src",
    [
        "addrings",
        "attr",
        "bbox",
        "bipart",
        "chkedges",
        "clustg",
        "collapse",
        "cycle",
        "deghist",
        "delmulti",
        "depath",
        "flatten",
        "group",
        "indent",
        "path",
        "scale",
        "span",
        "treetoclust",
        "addranks",
        "anon",
        "bb",
        "chkclusters",
        "cliptree",
        "col",
        "color",
        "dechain",
        "deledges",
        "delnodes",
        "dijkstra",
        "get-layers-list",
        "knbhd",
        "maxdeg",
        "rotate",
        "scalexy",
        "topon",
    ],
)
@pytest.mark.skipif(which("gvpr") is None, reason="GVPR not available")
def test_gvpr_example(src):
    """check GVPR can parse the given example"""

    # FIXME: remove when https://gitlab.com/graphviz/graphviz/-/issues/1784 is fixed
    if (
        (
            os.environ.get("build_system") == "msbuild"
            and os.environ.get("configuration") == "Debug"
        )
        or (
            platform.system() == "Windows" and os.environ.get("build_system") == "cmake"
        )
    ) and src in ["bbox", "col"]:
        pytest.skip(
            'GVPR tests "bbox" and "col" hangs on Windows MSBuild Debug '
            "builds and Windows CMake builds (#1784)"
        )

    # construct a relative path to the example because gvpr on Windows does not
    # support absolute paths (#1780)
    path = Path("cmd/gvpr/lib") / src
    wd = Path(__file__).parent.parent.resolve()

    # run GVPR with the given script
    subprocess.check_call(["gvpr", "-f", path], stdin=subprocess.DEVNULL, cwd=wd)


@pytest.mark.skipif(which("gvpr") is None, reason="GVPR not available")
# FIXME: Remove skip when
# https://gitlab.com/graphviz/graphviz/-/issues/1882 is fixed
@pytest.mark.skipif(
    platform.system() == "Windows"
    and os.getenv("build_system") == "cmake"
    and platform.machine() in ("AMD64", "x86_64"),
    reason="test_gvpr_clustg fails with 64-bit gvpr on Windows (#1882)",
)
def test_gvpr_clustg():
    """check cmd/gvpr/lib/clustg works"""

    # construct a relative path to clustg because gvpr on Windows does not
    # support absolute paths (#1780)
    path = Path("cmd/gvpr/lib/clustg")
    wd = Path(__file__).parent.parent.resolve()

    # some sample input
    input = "digraph { N1; N2; N1 -> N2; N3; }"

    # run GVPR on this input
    output = subprocess.check_output(
        ["gvpr", "-f", path], input=input, cwd=wd, universal_newlines=True
    )

    assert (
        output.strip() == 'strict digraph "clust%1" {\n'
        "\tnode [_cnt=0];\n"
        "\tedge [_cnt=0];\n"
        "\tN1 -> N2\t[_cnt=1];\n"
        "\tN3;\n"
        "}"
    ), "unexpected output"
