"""
Graphviz miscellaneous test cases
"""

import itertools
import json
import os
import platform
import subprocess
import sys
import tempfile
from pathlib import Path

import pytest

sys.path.append(os.path.dirname(__file__))
from gvtest import ROOT, compile_c, dot  # pylint: disable=wrong-import-position


def test_json_node_order():
    """
    test that nodes appear in JSON output in the same order as they were input
    """

    # a simple graph with some nodes
    input = (
        "digraph G {\n"
        "  ordering=out;\n"
        '  {rank=same;"1";"2"};\n'
        '  "1"->"2";\n'
        '  {rank=same;"4";"5";};\n'
        '  "4"->"5";\n'
        '  "7"->"5";\n'
        '  "7"->"4";\n'
        '  "6"->"1";\n'
        '  "3"->"6";\n'
        '  "6"->"4";\n'
        '  "3"->"8";\n'
        "}"
    )

    # turn it into JSON
    output = dot("json", source=input)

    # parse this into a data structure we can inspect
    data = json.loads(output)

    # extract the nodes, filtering out subgraphs
    nodes = [n["name"] for n in data["objects"] if "nodes" not in n]

    # the nodes should appear in the order in which they were seen in the input
    assert nodes == ["1", "2", "4", "5", "7", "6", "3", "8"]


def test_json_edge_order():
    """
    test that edges appear in JSON output in the same order as they were input
    """

    # a simple graph with some edges
    input = (
        "digraph G {\n"
        "  ordering=out;\n"
        '  {rank=same;"1";"2"};\n'
        '  "1"->"2";\n'
        '  {rank=same;"4";"5";};\n'
        '  "4"->"5";\n'
        '  "7"->"5";\n'
        '  "7"->"4";\n'
        '  "6"->"1";\n'
        '  "3"->"6";\n'
        '  "6"->"4";\n'
        '  "3"->"8";\n'
        "}"
    )

    # turn it into JSON
    output = dot("json", source=input)

    # parse this into a data structure we can inspect
    data = json.loads(output)

    # the edges should appear in the order in which they were seen in the input
    expected = [
        ("1", "2"),
        ("4", "5"),
        ("7", "5"),
        ("7", "4"),
        ("6", "1"),
        ("3", "6"),
        ("6", "4"),
        ("3", "8"),
    ]
    edges = [
        (data["objects"][e["tail"]]["name"], data["objects"][e["head"]]["name"])
        for e in data["edges"]
    ]
    assert edges == expected


@pytest.mark.skipif(
    platform.system() != "Linux", reason="TODO: make this test case portable"
)
def test_xml_escape():
    """
    Check the functionality of ../lib/common/xml.c:xml_escape.
    """

    # locate our test program
    xml_c = Path(__file__).parent / "../lib/common/xml.c"
    assert xml_c.exists(), "missing xml.c"

    with tempfile.TemporaryDirectory() as tmp:

        # write a dummy config.h to allow standalone compilation
        (Path(tmp) / "config.h").write_text("", encoding="utf-8")

        # compile the stub to something we can run
        xml_exe = Path(tmp) / "xml.exe"
        cflags = [
            "-std=c99",
            "-DTEST_XML",
            "-I",
            tmp,
            "-I",
            ROOT / "lib",
            "-I",
            ROOT / "lib/gvc",
            "-I",
            ROOT / "lib/pathplan",
            "-I",
            ROOT / "lib/cgraph",
            "-I",
            ROOT / "lib/cdt",
            "-Wall",
            "-Wextra",
        ]
        compile_c(xml_c, cflags, dst=xml_exe)

        def escape(dash: bool, nbsp: bool, raw: bool, utf8: bool, s: str) -> str:
            args = [xml_exe]
            if dash:
                args += ["--dash"]
            if nbsp:
                args += ["--nbsp"]
            if raw:
                args += ["--raw"]
            if utf8:
                args += ["--utf8"]
            args += [s]

            # We would like to pass `encoding="utf-8"`, or even better
            # `universal_newlines=True`. However, neither of these seem to work as
            # described in Python == 3.6. Observable using Ubuntu 18.04 in CI.
            # Instead, we encode and decode manually.
            args = [str(a).encode("utf-8") for a in args]
            out = subprocess.check_output(args)
            decoded = out.decode("utf-8")

            return decoded

        for dash, nbsp, raw, utf8 in itertools.product((False, True), repeat=4):

            # something basic with nothing escapable
            plain = "the quick brown fox"
            plain_escaped = escape(dash, nbsp, raw, utf8, plain)
            assert plain == plain_escaped, "text incorrectly modified"

            # basic tag escaping
            tag = "template <typename T> void foo(T t);"
            tag_escaped = escape(dash, nbsp, raw, utf8, tag)
            assert (
                tag_escaped == "template &lt;typename T&gt; void foo(T t);"
            ), "incorrect < or > escaping"

            # something with an embedded escape
            embedded = "salt &amp; pepper"
            embedded_escaped = escape(dash, nbsp, raw, utf8, embedded)
            if raw:
                assert embedded_escaped == "salt &amp;amp; pepper", "missing & escape"
            else:
                assert embedded_escaped == embedded, "text incorrectly modified"

            # hyphen escaping
            hyphen = "UTF-8"
            hyphen_escaped = escape(dash, nbsp, raw, utf8, hyphen)
            if dash:
                assert hyphen_escaped == "UTF&#45;8", "incorrect dash escape"
            else:
                assert hyphen_escaped == hyphen, "text incorrectly modified"

            # line endings
            nl = "the quick\nbrown\rfox"
            nl_escaped = escape(dash, nbsp, raw, utf8, nl)
            if raw:
                assert (
                    nl_escaped == "the quick&#10;brown&#13;fox"
                ), "incorrect new line escape"
            else:
                # allow benign modification of the \r
                assert nl_escaped in (
                    nl,
                    "the quick\nbrown\nfox",
                ), "text incorrectly modified"

            # non-breaking space escaping
            two = "the quick  brown fox"
            two_escaped = escape(dash, nbsp, raw, utf8, two)
            if nbsp:
                assert (
                    two_escaped == "the quick &#160;brown fox"
                ), "incorrect nbsp escape"
            else:
                assert two_escaped == two, "text incorrectly modified"

            # cases from table in https://en.wikipedia.org/wiki/UTF-8
            for c, expected in (
                ("$", "$"),
                ("¢", "&#xa2;"),
                ("ह", "&#x939;"),
                ("€", "&#x20ac;"),
                ("한", "&#xd55c;"),
                ("𐍈", "&#x10348;"),
            ):
                unescaped = f"character |{c}|"
                escaped = escape(dash, nbsp, raw, utf8, unescaped)
                if utf8:
                    assert escaped == f"character |{expected}|", "bad UTF-8 escaping"
                else:
                    assert escaped == unescaped, "bad UTF-8 passthrough"
