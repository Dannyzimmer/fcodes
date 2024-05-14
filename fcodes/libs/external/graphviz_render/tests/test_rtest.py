#!/usr/bin/env python3

"""
Older Graphviz regression test suite that has been encapsulated

TODO:
 Report differences with shared version and with new output.
"""

import hashlib
import io
import os
import platform
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import List

import pytest

# Test specifications
GRAPHDIR = Path(__file__).parent / "graphs"
# Directory of input graphs and data
OUTDIR = Path("ndata")  # Directory for test output
OUTHTML = Path("nhtml")  # Directory for html test report


class Case:
    """
    test case struct
    """

    def __init__(
        self,
        name: str,
        input: Path,
        algorithm: str,
        format: str,
        flags: List[str],
    ):  # pylint: disable=too-many-arguments
        self.name = name
        self.input = input
        self.algorithm = algorithm
        self.format = format
        self.flags = flags[:]


TESTS: List[Case] = [
    Case("shapes", Path("shapes.gv"), "dot", "gv", []),
    Case("shapes", Path("shapes.gv"), "dot", "ps", []),
    Case("crazy", Path("crazy.gv"), "dot", "png", []),
    Case("crazy", Path("crazy.gv"), "dot", "ps", []),
    Case("arrows", Path("arrows.gv"), "dot", "gv", []),
    Case("arrows", Path("arrows.gv"), "dot", "ps", []),
    Case("arrowsize", Path("arrowsize.gv"), "dot", "png", []),
    Case("center", Path("center.gv"), "dot", "ps", []),
    Case("center", Path("center.gv"), "dot", "png", ["-Gmargin=1"]),
    # color encodings
    # multiple edge colors
    Case("color", Path("color.gv"), "dot", "png", []),
    Case("color", Path("color.gv"), "dot", "png", ["-Gbgcolor=lightblue"]),
    Case("decorate", Path("decorate.gv"), "dot", "png", []),
    Case("record", Path("record.gv"), "dot", "gv", []),
    Case("record", Path("record.gv"), "dot", "ps", []),
    Case("html", Path("html.gv"), "dot", "gv", []),
    Case("html", Path("html.gv"), "dot", "ps", []),
    Case("html2", Path("html2.gv"), "dot", "gv", []),
    Case("html2", Path("html2.gv"), "dot", "ps", []),
    Case("html2", Path("html2.gv"), "dot", "svg", []),
    Case("pslib", Path("pslib.gv"), "dot", "ps", ["-lgraphs/sdl.ps"]),
    Case("user_shapes", Path("user_shapes.gv"), "dot", "ps", []),
    # dot png - doesn't work: Warning: No loadimage plugin for "gif:cairo"
    Case("user_shapes", Path("user_shapes.gv"), "dot", "png:gd", []),
    # bug - the epsf version has problems
    Case(
        "ps_user_shapes",
        Path("ps_user_shapes.gv"),
        "dot",
        "ps",
        ["-Nshapefile=graphs/dice.ps"],
    ),
    Case("colorscheme", Path("colorscheme.gv"), "dot", "ps", []),
    Case("colorscheme", Path("colorscheme.gv"), "dot", "png", []),
    Case("compound", Path("compound.gv"), "dot", "gv", []),
    Case("dir", Path("dir.gv"), "dot", "ps", []),
    Case("clusters", Path("clusters.gv"), "dot", "ps", []),
    Case("clusters", Path("clusters.gv"), "dot", "png", []),
    Case(
        "clustlabel",
        Path("clustlabel.gv"),
        "dot",
        "ps",
        ["-Glabelloc=t", "-Glabeljust=r"],
    ),
    Case(
        "clustlabel",
        Path("clustlabel.gv"),
        "dot",
        "ps",
        ["-Glabelloc=b", "-Glabeljust=r"],
    ),
    Case(
        "clustlabel",
        Path("clustlabel.gv"),
        "dot",
        "ps",
        ["-Glabelloc=t", "-Glabeljust=l"],
    ),
    Case(
        "clustlabel",
        Path("clustlabel.gv"),
        "dot",
        "ps",
        ["-Glabelloc=b", "-Glabeljust=l"],
    ),
    Case(
        "clustlabel",
        Path("clustlabel.gv"),
        "dot",
        "ps",
        ["-Glabelloc=t", "-Glabeljust=c"],
    ),
    Case(
        "clustlabel",
        Path("clustlabel.gv"),
        "dot",
        "ps",
        ["-Glabelloc=b", "-Glabeljust=c"],
    ),
    Case("clustlabel", Path("clustlabel.gv"), "dot", "ps", ["-Glabelloc=t"]),
    Case("clustlabel", Path("clustlabel.gv"), "dot", "ps", ["-Glabelloc=b"]),
    Case(
        "rootlabel",
        Path("rootlabel.gv"),
        "dot",
        "ps",
        ["-Glabelloc=t", "-Glabeljust=r"],
    ),
    Case(
        "rootlabel",
        Path("rootlabel.gv"),
        "dot",
        "ps",
        ["-Glabelloc=b", "-Glabeljust=r"],
    ),
    Case(
        "rootlabel",
        Path("rootlabel.gv"),
        "dot",
        "ps",
        ["-Glabelloc=t", "-Glabeljust=l"],
    ),
    Case(
        "rootlabel",
        Path("rootlabel.gv"),
        "dot",
        "ps",
        ["-Glabelloc=b", "-Glabeljust=l"],
    ),
    Case(
        "rootlabel",
        Path("rootlabel.gv"),
        "dot",
        "ps",
        ["-Glabelloc=t", "-Glabeljust=c"],
    ),
    Case(
        "rootlabel",
        Path("rootlabel.gv"),
        "dot",
        "ps",
        ["-Glabelloc=b", "-Glabeljust=c"],
    ),
    Case("rootlabel", Path("rootlabel.gv"), "dot", "ps", ["-Glabelloc=t"]),
    Case("rootlabel", Path("rootlabel.gv"), "dot", "ps", ["-Glabelloc=b"]),
    Case("layers", Path("layers.gv"), "dot", "ps", []),
    # check mode=hier
    Case("mode", Path("mode.gv"), "neato", "ps", ["-Gmode=KK"]),
    Case("mode", Path("mode.gv"), "neato", "ps", ["-Gmode=hier"]),
    Case("mode", Path("mode.gv"), "neato", "ps", ["-Gmode=hier", "-Glevelsgap=1"]),
    Case("model", Path("mode.gv"), "neato", "ps", ["-Gmodel=circuit"]),
    Case(
        "model", Path("mode.gv"), "neato", "ps", ["-Goverlap=false", "-Gmodel=subset"]
    ),
    # cairo versions have problems
    Case("nojustify", Path("nojustify.gv"), "dot", "png", []),
    Case("nojustify", Path("nojustify.gv"), "dot", "png:gd", []),
    Case("nojustify", Path("nojustify.gv"), "dot", "ps", []),
    Case("nojustify", Path("nojustify.gv"), "dot", "ps:cairo", []),
    # bug
    Case("ordering", Path("ordering.gv"), "dot", "gv", ["-Gordering=in"]),
    Case("ordering", Path("ordering.gv"), "dot", "gv", ["-Gordering=out"]),
    Case("overlap", Path("overlap.gv"), "neato", "gv", ["-Goverlap=false"]),
    Case("overlap", Path("overlap.gv"), "neato", "gv", ["-Goverlap=scale"]),
    Case("pack", Path("pack.gv"), "neato", "gv", []),
    Case("pack", Path("pack.gv"), "neato", "gv", ["-Gpack=20"]),
    Case("pack", Path("pack.gv"), "neato", "gv", ["-Gpackmode=graph"]),
    Case("page", Path("mode.gv"), "neato", "ps", ["-Gpage=8.5,11"]),
    Case("page", Path("mode.gv"), "neato", "ps", ["-Gpage=8.5,11", "-Gpagedir=TL"]),
    Case("page", Path("mode.gv"), "neato", "ps", ["-Gpage=8.5,11", "-Gpagedir=TR"]),
    # pencolor, fontcolor, fillcolor
    Case("colors", Path("colors.gv"), "dot", "ps", []),
    Case("polypoly", Path("polypoly.gv"), "dot", "ps", []),
    Case("polypoly", Path("polypoly.gv"), "dot", "png", []),
    Case("ports", Path("ports.gv"), "dot", "gv", []),
    Case("rotate", Path("crazy.gv"), "dot", "png", ["-Glandscape"]),
    Case("rotate", Path("crazy.gv"), "dot", "ps", ["-Glandscape"]),
    Case("rotate", Path("crazy.gv"), "dot", "png", ["-Grotate=90"]),
    Case("rotate", Path("crazy.gv"), "dot", "ps", ["-Grotate=90"]),
    Case("rankdir", Path("crazy.gv"), "dot", "gv", ["-Grankdir=LR"]),
    Case("rankdir", Path("crazy.gv"), "dot", "gv", ["-Grankdir=BT"]),
    Case("rankdir", Path("crazy.gv"), "dot", "gv", ["-Grankdir=RL"]),
    Case("url", Path("url.gv"), "dot", "ps2", []),
    Case("url", Path("url.gv"), "dot", "svg", ["-Gstylesheet=stylesheet"]),
    Case("url", Path("url.gv"), "dot", "imap", []),
    Case("url", Path("url.gv"), "dot", "cmapx", []),
    Case("url", Path("url.gv"), "dot", "imap_np", []),
    Case("url", Path("url.gv"), "dot", "cmapx_np", []),
    Case(
        "viewport", Path("viewport.gv"), "neato", "png", ["-Gviewport=300,300", "-n2"]
    ),
    Case("viewport", Path("viewport.gv"), "neato", "ps", ["-Gviewport=300,300", "-n2"]),
    Case(
        "viewport",
        Path("viewport.gv"),
        "neato",
        "png",
        ["-Gviewport=300,300,1,200,620", "-n2"],
    ),
    Case(
        "viewport",
        Path("viewport.gv"),
        "neato",
        "ps",
        ["-Gviewport=300,300,1,200,620", "-n2"],
    ),
    Case(
        "viewport",
        Path("viewport.gv"),
        "neato",
        "png",
        ["-Gviewport=300,300,2,200,620", "-n2"],
    ),
    Case(
        "viewport",
        Path("viewport.gv"),
        "neato",
        "ps",
        ["-Gviewport=300,300,2,200,620", "-n2"],
    ),
    Case("rowcolsep", Path("rowcolsep.gv"), "dot", "gv", ["-Gnodesep=0.5"]),
    Case("rowcolsep", Path("rowcolsep.gv"), "dot", "gv", ["-Granksep=1.5"]),
    Case("size", Path("mode.gv"), "neato", "ps", ["-Gsize=5,5"]),
    Case("size", Path("mode.gv"), "neato", "png", ["-Gsize=5,5"]),
    # size with !
    Case("size_ex", Path("root.gv"), "dot", "ps", ["-Gsize=6,6!"]),
    Case("size_ex", Path("root.gv"), "dot", "png", ["-Gsize=6,6!"]),
    Case("dotsplines", Path("size.gv"), "dot", "gv", ["-Gsplines=line"]),
    Case("dotsplines", Path("size.gv"), "dot", "gv", ["-Gsplines=polyline"]),
    Case(
        "neatosplines",
        Path("overlap.gv"),
        "neato",
        "gv",
        ["-Goverlap=false", "-Gsplines"],
    ),
    Case(
        "neatosplines",
        Path("overlap.gv"),
        "neato",
        "gv",
        ["-Goverlap=false", "-Gsplines=polyline"],
    ),
    Case("style", Path("style.gv"), "dot", "ps", []),
    Case("style", Path("style.gv"), "dot", "png", []),
    # edge clipping
    Case("edgeclip", Path("edgeclip.gv"), "dot", "gv", []),
    # edge weight
    Case("weight", Path("weight.gv"), "dot", "gv", []),
    Case("root", Path("root.gv"), "twopi", "gv", []),
    Case("cairo", Path("cairo.gv"), "dot", "ps:cairo", []),
    Case("cairo", Path("cairo.gv"), "dot", "png:cairo", []),
    Case("cairo", Path("cairo.gv"), "dot", "svg:cairo", []),
    Case("flatedge", Path("flatedge.gv"), "dot", "gv", []),
    Case("nestedclust", Path("nestedclust"), "dot", "gv", []),
    Case("rd_rules", Path("rd_rules.gv"), "dot", "png", []),
    Case("sq_rules", Path("sq_rules.gv"), "dot", "png", []),
    # FIXME: Re-enable when https://gitlab.com/graphviz/graphviz/-/issues/1690 is fixed
    # Case("fdp_clus", Path("fdp.gv"), "fdp", "png", []),
    Case("japanese", Path("japanese.gv"), "dot", "png", []),
    Case("russian", Path("russian.gv"), "dot", "png", []),
    Case("AvantGarde", Path("AvantGarde.gv"), "dot", "png", []),
    Case("AvantGarde", Path("AvantGarde.gv"), "dot", "ps", []),
    Case("Bookman", Path("Bookman.gv"), "dot", "png", []),
    Case("Bookman", Path("Bookman.gv"), "dot", "ps", []),
    Case("Helvetica", Path("Helvetica.gv"), "dot", "png", []),
    Case("Helvetica", Path("Helvetica.gv"), "dot", "ps", []),
    Case("NewCenturySchlbk", Path("NewCenturySchlbk.gv"), "dot", "png", []),
    Case("NewCenturySchlbk", Path("NewCenturySchlbk.gv"), "dot", "ps", []),
    Case("Palatino", Path("Palatino.gv"), "dot", "png", []),
    Case("Palatino", Path("Palatino.gv"), "dot", "ps", []),
    Case("Times", Path("Times.gv"), "dot", "png", []),
    Case("Times", Path("Times.gv"), "dot", "ps", []),
    Case("ZapfChancery", Path("ZapfChancery.gv"), "dot", "png", []),
    Case("ZapfChancery", Path("ZapfChancery.gv"), "dot", "ps", []),
    Case("ZapfDingbats", Path("ZapfDingbats.gv"), "dot", "png", []),
    Case("ZapfDingbats", Path("ZapfDingbats.gv"), "dot", "ps", []),
    Case("xlabels", Path("xlabels.gv"), "dot", "png", []),
    Case("xlabels", Path("xlabels.gv"), "neato", "png", []),
    Case("sides", Path("sides.gv"), "dot", "ps", []),
]


def doDiff(OUTFILE, testname, fmt):
    """
    Compare old and new output and report if different.
     Args: testname index fmt
    """
    FILE1 = OUTDIR / OUTFILE
    FILE2 = REFDIR / OUTFILE
    F = fmt.split(":")[0]
    # FIXME: Remove when https://gitlab.com/graphviz/graphviz/-/issues/1789 is
    # fixed
    if (
        platform.system() == "Windows"
        and F in ["ps", "gv"]
        and testname in ["clusters", "compound", "rootlabel"]
    ):
        pytest.skip(
            f"Warning: Skipping {F} output comparison for test "
            f"{testname}: format {fmt} because the order of "
            "clusters in gv or ps output is not stable on Windows (#1789)"
        )
    if F in ["ps", "ps2"]:
        with open(FILE1, "rt", encoding="latin-1") as src:
            dst1 = io.StringIO()
            done_setup = False
            for line in src:
                if done_setup:
                    dst1.write(line)
                else:
                    done_setup = re.match(r"%%End.*Setup", line) is not None

        with open(FILE2, "rt", encoding="latin-1") as src:
            dst2 = io.StringIO()
            done_setup = False
            for line in src:
                if done_setup:
                    dst2.write(line)
                else:
                    done_setup = re.match(r"%%End.*Setup", line) is not None

        returncode = 0 if dst1.getvalue() == dst2.getvalue() else -1
    elif F == "svg":
        with open(FILE1, "rt", encoding="utf-8") as f:
            a = re.sub(r"^<!--.*-->$", "", f.read(), flags=re.MULTILINE)
        with open(FILE2, "rt", encoding="utf-8") as f:
            b = re.sub(r"^<!--.*-->$", "", f.read(), flags=re.MULTILINE)
        returncode = 0 if a.strip() == b.strip() else -1
    elif F == "png":
        # FIXME: remove when https://gitlab.com/graphviz/graphviz/-/issues/1788 is fixed
        if os.environ.get("build_system") == "cmake" and platform.system() == "Windows":
            pytest.skip(
                f"Warning: Skipping PNG image comparison for test {testname}:"
                f" format: {fmt} because CMake builds on Windows "
                "do not contain the diffimg utility (#1788)"
            )
        OUTHTML.mkdir(exist_ok=True)
        returncode = subprocess.call(
            ["diffimg", FILE1, FILE2, OUTHTML / f"dif_{OUTFILE}"],
        )
        if returncode != 0:
            with open(OUTHTML / "index.html", "at", encoding="utf-8") as fd:
                fd.write("<p>\n")
                shutil.copyfile(FILE2, OUTHTML / f"old_{OUTFILE}")
                fd.write(f'<img src="old_{OUTFILE}" width="192" height="192">\n')
                shutil.copyfile(FILE1, OUTHTML / f"new_{OUTFILE}")
                fd.write(f'<img src="new_{OUTFILE}" width="192" height="192">\n')
                fd.write(f'<img src="dif_{OUTFILE}" width="192" height="192">\n')
        else:
            (OUTHTML / f"dif_{OUTFILE}").unlink()
    else:
        with open(FILE2, "rt", encoding="utf-8") as a:
            with open(FILE1, "rt", encoding="utf-8") as b:
                returncode = 0 if a.read().strip() == b.read().strip() else -1
    if returncode != 0:
        # FIXME: pytest.fail when all tests pass on all platforms
        print(f"Test {testname}: == Failed == {OUTFILE}", file=sys.stderr)


def genOutname(name, alg, fmt, flags: List[str]):
    """
    Generate output file name given 4 parameters.
      testname layout format flags
    If format ends in :*, remove this, change the colons to underscores,
    and append to basename
    """
    fmt_split = fmt.split(":")
    if len(fmt_split) >= 2:
        F = fmt_split[0]
        XFMT = f'_{"_".join(fmt_split[1:])}'
    else:
        F = fmt
        XFMT = ""

    suffix = hashlib.sha256()
    for flag in flags:
        suffix.update(f"{flag} ".encode("utf-8"))

    OUTFILE = f"{name}_{alg}{XFMT}_{suffix.hexdigest()}.{F}"
    return OUTFILE


@pytest.mark.parametrize(
    "name,input,algorithm,format,flags",
    ((c.name, c.input, c.algorithm, c.format, c.flags) for c in TESTS),
)
def test_graph(name: str, input: Path, algorithm: str, format: str, flags: List[str]):
    """
    Run a single test.
    """
    if input.suffix == ".gv":
        INFILE = GRAPHDIR / input
    else:
        pytest.skip(f"Unknown graph spec, test {name} - ignoring")

    OUTFILE = genOutname(name, algorithm, format, flags)
    OUTDIR.mkdir(exist_ok=True)
    OUTPATH = OUTDIR / OUTFILE
    testcmd = ["dot", f"-K{algorithm}", f"-T{format}"] + flags + ["-o", OUTPATH, INFILE]
    # FIXME: Remove when https://gitlab.com/graphviz/graphviz/-/issues/1786 is
    # fixed
    if os.environ.get("build_system") == "cmake" and format == "png:gd":
        pytest.skip(
            f"Skipping test {name}: format {format} because "
            "CMake builds does not support format png:gd (#1786)"
        )
    # FIXME: Remove when https://gitlab.com/graphviz/graphviz/-/issues/1269 is
    # fixed
    if (
        platform.system() == "Windows"
        and os.environ.get("build_system") == "msbuild"
        and "-Goverlap=false" in flags
    ):
        pytest.skip(
            f"Skipping test {name}: with flag -Goverlap=false because "
            "it fails with Windows MSBuild builds which are not built with "
            "triangulation library (#1269)"
        )
    # FIXME: Remove when https://gitlab.com/graphviz/graphviz/-/issues/1787 is
    # fixed
    if (
        platform.system() == "Windows"
        and os.environ.get("build_system") == "msbuild"
        and os.environ.get("configuration") == "Debug"
        and name == "user_shapes"
    ):
        pytest.skip(
            f"Skipping test {name}: using shapefile because it fails "
            "with Windows MSBuild Debug builds (#1787)"
        )
    # FIXME: Remove when https://gitlab.com/graphviz/graphviz/-/issues/1790 is
    # fixed
    if platform.system() == "Windows" and name == "ps_user_shapes":
        pytest.skip(
            f"Skipping test {name}: using PostScript shapefile "
            "because it fails with Windows builds (#1790)"
        )

    RVAL = subprocess.call(testcmd, stderr=subprocess.STDOUT)

    if RVAL != 0 or not OUTPATH.exists():
        pytest.fail(
            f'Test {name}: == Layout failed ==\n  {" ".join(str(a) for a in testcmd)}'
        )
    elif (REFDIR / OUTFILE).exists():
        doDiff(OUTFILE, name, format)
    else:
        sys.stderr.write(
            f"Test {name}: == No file {REFDIR}/{OUTFILE} for comparison ==\n"
        )


# Set REFDIR
if platform.system() == "Linux":
    REFDIR = Path("linux.x86")
elif platform.system() == "Darwin":
    REFDIR = Path("macosx")
elif platform.system() == "Windows":
    REFDIR = Path("nshare")
else:
    print(f'Unrecognized system "{platform.system()}"', file=sys.stderr)
    REFDIR = Path("nshare")
