"""
Tests of Graphviz output against expected reference output.
"""

import difflib
import subprocess
import sys
import tempfile
from pathlib import Path

import pytest


# pylint: disable=line-too-long
@pytest.mark.xfail
@pytest.mark.parametrize(
    "src,format,reference",
    (
        ('graph G {a [label="" shape=none]}', "gv", "001.001.gv"),
        ('graph G {a [label="" shape=none]}', "xdot", "001.002.xdot"),
        ('graph G {a [label="" shape=none]}', "png", "001.003.png"),
        ('graph G {a [label="" shape=none]}', "svg", "001.004.svg"),
        ('graph G {a [label="" shape=none]}', "ps", "001.005.ps"),
        ('graph G {a [label="" shape=none]}', "cmapx", "001.006.cmapx"),
        ('graph G {a [label="" shape=plaintext]}', "xdot", "002.001.xdot"),
        ('graph G {a [label="" shape=point]}', "xdot", "003.001.xdot"),
        ('graph G {a [label="" shape=box]}', "xdot", "004.001.xdot"),
        ('graph G {a [label="" shape=rect]}', "xdot", "005.001.xdot"),
        ('graph G {a [label="" shape=rectangle]}', "xdot", "006.001.xdot"),
        ('graph G {a [label="" shape=ellipse]}', "xdot", "007.001.xdot"),
        ('graph G {a [label="" shape=circle]}', "xdot", "008.001.xdot"),
        ('graph G {a [label="" shape=triangle]}', "xdot", "009.001.xdot"),
        ('graph G {a [label="" shape=invtriangle]}', "xdot", "010.001.xdot"),
        ('graph G {a [label="" shape=diamond]}', "xdot", "011.001.xdot"),
        ('graph G {a [label="" shape=trapezium]}', "xdot", "012.001.xdot"),
        ('graph G {a [label="" shape=invtrapezium]}', "xdot", "013.001.xdot"),
        ('graph G {a [label="" shape=parallelogram]}', "xdot", "014.001.xdot"),
        ('graph G {a [label="" shape=house]}', "xdot", "015.001.xdot"),
        ('graph G {a [label="" shape=invhouse]}', "xdot", "016.001.xdot"),
        ('graph G {a [label="" shape=pentagon]}', "xdot", "017.001.xdot"),
        ('graph G {a [label="" shape=hexagon]}', "xdot", "018.001.xdot"),
        ('graph G {a [label="" shape=septagon]}', "xdot", "019.001.xdot"),
        ('graph G {a [label="" shape=octagon]}', "xdot", "020.001.xdot"),
        ('graph G {a [label="" shape=doubleoctagon]}', "xdot", "021.001.xdot"),
        ('graph G {a [label="" shape=doublecircle]}', "xdot", "022.001.xdot"),
        ('graph G {a [label="" shape=tripleoctagon]}', "xdot", "023.001.xdot"),
        ('graph G {a [label="" shape=Mdiamond]}', "xdot", "024.001.xdot"),
        ('graph G {a [label="" shape=Msquare]}', "xdot", "025.001.xdot"),
        ('graph G {a [label="" shape=Mcircle]}', "xdot", "026.001.xdot"),
        ('graph G {a [label="" shape=none]}', "xdot", "027.001.xdot"),
        ("graph G {a [label=b shape=none]}", "xdot", "028.001.xdot"),
        ("graph G {a [shape=none]}", "xdot", "029.001.xdot"),
        ('graph G {a [label="\\N" shape=none]}', "xdot", "030.001.xdot"),
        ('graph G {a [label="n_of_\\G" shape=none]}', "xdot", "031.001.xdot"),
        ('graph G {a [label="\\E\\H\\T" shape=none]}', "xdot", "032.001.xdot"),
        (
            'digraph G {node [label="" shape=none]; a->b [label="\\E"]}',
            "xdot",
            "033.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [label="\\T"]}',
            "xdot",
            "034.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [label="\\H"]}',
            "xdot",
            "035.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [label="e_of_\\G"]}',
            "xdot",
            "036.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [label="\\N"]}',
            "xdot",
            "037.001.xdot",
        ),
        (
            'graph G {a [label="center\\naligned\\ntext" shape=none]}',
            "xdot",
            "038.001.xdot",
        ),
        (
            'graph G {a [label="left\\laligned\\ltext" shape=none]}',
            "xdot",
            "039.001.xdot",
        ),
        (
            'graph G {a [label="right\\raligned\\rtext" shape=none]}',
            "xdot",
            "040.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=none]}',
            "xdot",
            "041.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=""]}',
            "xdot",
            "042.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=normal]}',
            "xdot",
            "043.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=box]}',
            "xdot",
            "044.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=crow]}',
            "xdot",
            "045.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=diamond]}',
            "xdot",
            "046.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=dot]}',
            "xdot",
            "047.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=inv]}',
            "xdot",
            "048.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=tee]}',
            "xdot",
            "049.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=vee]}',
            "xdot",
            "050.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=lnormal]}',
            "xdot",
            "051.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=lbox]}',
            "xdot",
            "052.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=lcrow]}',
            "xdot",
            "053.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=ldiamond]}',
            "xdot",
            "054.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=ldot]}',
            "xdot",
            "055.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=linv]}',
            "xdot",
            "056.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=ltee]}',
            "xdot",
            "057.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=lvee]}',
            "xdot",
            "058.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=rnormal]}',
            "xdot",
            "059.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=rbox]}',
            "xdot",
            "060.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=rcrow]}',
            "xdot",
            "061.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=rdiamond]}',
            "xdot",
            "062.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=rdot]}',
            "xdot",
            "063.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=rinv]}',
            "xdot",
            "064.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=rtee]}',
            "xdot",
            "065.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=rvee]}',
            "xdot",
            "066.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=onormal]}',
            "xdot",
            "067.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=obox]}',
            "xdot",
            "068.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=ocrow]}',
            "xdot",
            "069.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=odiamond]}',
            "xdot",
            "070.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=odot]}',
            "xdot",
            "071.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=oinv]}',
            "xdot",
            "072.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=otee]}',
            "xdot",
            "073.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=ovee]}',
            "xdot",
            "074.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=onormalnormal]}',
            "xdot",
            "075.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=oboxbox]}',
            "xdot",
            "076.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=ocrowcrow]}',
            "xdot",
            "077.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=odiamonddiamond]}',
            "xdot",
            "078.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=odotdot]}',
            "xdot",
            "079.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=oinvinv]}',
            "xdot",
            "080.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=oteetee]}',
            "xdot",
            "081.001.xdot",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [arrowhead=oveevee]}',
            "xdot",
            "082.001.xdot",
        ),
        (
            'graph G {a [label="" shape=box style=filled color=black]}',
            "xdot",
            "083.001.xdot",
        ),
        (
            'graph G {a [label="" shape=box style=filled color=black]}',
            "png",
            "083.002.png",
        ),
        (
            'graph G {a [label="" shape=box style=filled color=red]}',
            "xdot",
            "084.001.xdot",
        ),
        (
            'graph G {a [label="" shape=box style=filled color=red]}',
            "png",
            "084.002.png",
        ),
        (
            'graph G {a [label="" shape=box style=filled color=hotpink]}',
            "xdot",
            "085.001.xdot",
        ),
        (
            'graph G {a [label="" shape=box style=filled color=hotpink]}',
            "png",
            "085.002.png",
        ),
        (
            'graph G {a [label="" shape=box style=filled color=transparent]}',
            "xdot",
            "086.001.xdot",
        ),
        (
            'graph G {a [label="" shape=box style=filled color=transparent]}',
            "png",
            "086.002.png",
        ),
        ('graph G {a [URL="" label="" shape=none]}', "cmapx", "087.001.cmapx"),
        ('graph G {a [URL="" label="" shape=none]}', "ps", "087.002.ps"),
        ('graph G {a [URL="" label="" shape=none]}', "svg", "087.003.svg"),
        ('graph G {graph [URL="\\G.html"]}', "cmapx", "088.001.cmapx"),
        ('graph G {graph [URL="\\G.html"]}', "ps", "088.002.ps"),
        ('graph G {graph [URL="\\G.html"]}', "svg", "088.003.svg"),
        ('graph G {a [URL="\\N.html" label="" shape=none]}', "cmapx", "089.001.cmapx"),
        ('graph G {a [URL="\\N.html" label="" shape=none]}', "ps", "089.002.ps"),
        ('graph G {a [URL="\\N.html" label="" shape=none]}', "svg", "089.003.svg"),
        (
            'digraph G {node [label="" shape=none]; a->b [URL="\\E.html"]}',
            "cmapx",
            "090.001.cmapx",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [URL="\\E.html"]}',
            "ps",
            "090.002.ps",
        ),
        (
            'digraph G {node [label="" shape=none]; a->b [URL="\\E.html"]}',
            "svg",
            "090.003.svg",
        ),
    ),
)
def test_reference(src: str, format: str, reference: str):
    """test some Graphviz input against a reference source"""

    # locate the reference output
    ref = Path(__file__).resolve().parent / "test_reference" / reference
    assert ref.exists()

    # make a scratch space
    with tempfile.TemporaryDirectory() as tmp:
        t = Path(tmp)

        # process our input with Graphviz
        output = t / reference
        subprocess.run(
            ["dot", f"-T{format}", "-o", output],
            input=src,
            check=True,
            universal_newlines=True,
        )

        # compare against the reference output
        if output.suffix == ".png":
            subprocess.check_call(["diffimg", ref, output])
        else:
            fail = False
            a = ref.read_text(encoding="utf-8")
            b = output.read_text(encoding="utf-8")
            for line in difflib.unified_diff(
                a, b, fromfile=str(ref), tofile=str(output)
            ):
                sys.stderr.write(f"{line}\n")
                fail = True
            if fail:
                raise RuntimeError(f"diff {ref} {output} failed")
