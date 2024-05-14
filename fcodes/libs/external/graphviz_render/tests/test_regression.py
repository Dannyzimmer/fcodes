"""
Graphviz regression tests

The test cases in this file relate to previously observed bugs. A failure of one
of these indicates that a past bug has been reintroduced.
"""

import io
import json
import math
import os
import platform
import re
import shutil
import signal
import stat
import subprocess
import sys
import tempfile
import textwrap
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import List

import pytest

sys.path.append(os.path.dirname(__file__))
from gvtest import (  # pylint: disable=wrong-import-position
    ROOT,
    dot,
    gvpr,
    is_mingw,
    remove_xtype_warnings,
    run_c,
    which,
)


def is_ndebug_defined() -> bool:
    """
    are assertions disabled in the Graphviz build under test?
    """

    # the Windows release builds set NDEBUG
    if os.environ.get("configuration") == "Release":
        return True

    return False


@pytest.mark.xfail(
    platform.system() == "Windows", reason="#56", strict=not is_ndebug_defined()
)  # FIXME
def test_14():
    """
    using ortho and twopi in combination should not cause an assertion failure
    https://gitlab.com/graphviz/graphviz/-/issues/14
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "14.dot"
    assert input.exists(), "unexpectedly missing test case"

    # process it with Graphviz
    dot("svg", input)


@pytest.mark.skipif(which("neato") is None, reason="neato not available")
def test_42():
    """
    check for a former crash in neatogen
    https://gitlab.com/graphviz/graphviz/-/issues/42
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "42.dot"
    assert input.exists(), "unexpectedly missing test case"

    # process it with Graphviz
    subprocess.check_call(["neato", "-n2", "-Tpng", input], stdout=subprocess.DEVNULL)


def test_56():
    """
    parsing a particular graph should not cause a Trapezoid-table overflow
    assertion failure
    https://gitlab.com/graphviz/graphviz/-/issues/56
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "56.dot"
    assert input.exists(), "unexpectedly missing test case"

    # process it with Graphviz
    dot("svg", input)


def test_121():
    """
    test a graph that previously caused an assertion failure in `merge_chain`
    https://gitlab.com/graphviz/graphviz/-/issues/121
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "121.dot"
    assert input.exists(), "unexpectedly missing test case"

    # process it with Graphviz
    dot("pdf", input)


def test_131():
    """
    PIC back end should produce valid output
    https://gitlab.com/graphviz/graphviz/-/issues/131
    """

    # a basic graph
    src = "digraph { a -> b; c -> d; }"

    # ask Graphviz to process this to PIC
    pic = dot("pic", source=src)

    if which("gpic") is None:
        pytest.skip("GNU PIC not available")

    # ask GNU PIC to process the Graphviz output
    subprocess.run(
        ["gpic"],
        input=pic,
        stdout=subprocess.DEVNULL,
        check=True,
        universal_newlines=True,
    )


@pytest.mark.parametrize("testcase", ("144_no_ortho.dot", "144_ortho.dot"))
def test_144(testcase: str):
    """
    using ortho should not result in head/tail confusion
    https://gitlab.com/graphviz/graphviz/-/issues/144
    """

    # locate our associated test cases in this directory
    input = Path(__file__).parent / testcase
    assert input.exists(), "unexpectedly missing test case"

    # process the non-ortho one into JSON
    out = dot("json", input)
    data = json.loads(out)

    # find the nodes “A”, “B” and “C”
    A = [x for x in data["objects"] if x["name"] == "A"][0]
    B = [x for x in data["objects"] if x["name"] == "B"][0]
    C = [x for x in data["objects"] if x["name"] == "C"][0]

    # find the straight A→B and the angular A→C edges
    straight_edge = [
        x for x in data["edges"] if x["tail"] == A["_gvid"] and x["head"] == B["_gvid"]
    ][0]
    angular_edge = [
        x for x in data["edges"] if x["tail"] == A["_gvid"] and x["head"] == C["_gvid"]
    ][0]

    # the A→B edge should have been routed vertically down
    straight_points = straight_edge["_draw_"][1]["points"]
    xs = [x for x, _ in straight_points]
    ys = [y for _, y in straight_points]
    assert all(x == xs[0] for x in xs), "A->B not routed vertically"
    assert ys == sorted(ys, reverse=True), "A->B is not routed down"

    # determine Graphviz’ idea of head and tail ends
    straight_head_point = straight_edge["_hdraw_"][3]["points"][0]
    straight_tail_point = straight_edge["_tdraw_"][3]["points"][0]
    assert straight_head_point[1] < straight_tail_point[1], "A->B head/tail confusion"

    # the A→C edge should have been routed in zigzag down and right
    angular_points = angular_edge["_draw_"][1]["points"]
    xs = [x for x, _ in angular_points]
    ys = [y for _, y in angular_points]
    assert xs == sorted(xs), "A->B is not routed down"
    assert ys == sorted(ys, reverse=True), "A->B is not routed right"

    # determine Graphviz’ idea of head and tail ends
    angular_head_point = angular_edge["_hdraw_"][3]["points"][0]
    angular_tail_point = angular_edge["_tdraw_"][3]["points"][0]
    assert angular_head_point[0] > angular_tail_point[0], "A->C head/tail confusion"


def test_146():
    """
    dot should respect an alpha channel value of 0 when writing SVG
    https://gitlab.com/graphviz/graphviz/-/issues/146
    """

    # a graph using white text but with 0 alpha
    source = (
        "graph {\n"
        '  n[style="filled", fontcolor="#FFFFFF00", label="hello world"];\n'
        "}"
    )

    # ask Graphviz to process this
    svg = dot("svg", source=source)

    # the SVG should be setting opacity
    opacity = re.search(r'\bfill-opacity="(\d+(\.\d+)?)"', svg)
    assert opacity is not None, "transparency not set for alpha=0 color"

    # it should be zeroed
    assert (
        float(opacity.group(1)) == 0
    ), "alpha=0 color set to something non-transparent"


def test_165():
    """
    dot should be able to produce properly escaped xdot output
    https://gitlab.com/graphviz/graphviz/-/issues/165
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "165.dot"
    assert input.exists(), "unexpectedly missing test case"

    # ask Graphviz to translate it to xdot
    output = dot("xdot", input)

    # find the line containing the _ldraw_ attribute
    ldraw = re.search(r"^\s*_ldraw_\s*=(?P<value>.*?)$", output, re.MULTILINE)
    assert ldraw is not None, "no _ldraw_ attribute in graph"

    # this should contain the label correctly escaped
    assert r"hello \\\" world" in ldraw.group("value"), "unexpected ldraw contents"


def test_165_2():
    """
    variant of test_165() that checks a similar problem for edges
    https://gitlab.com/graphviz/graphviz/-/issues/165
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "165_2.dot"
    assert input.exists(), "unexpectedly missing test case"

    # ask Graphviz to translate it to xdot
    output = dot("xdot", input)

    # find the lines containing _ldraw_ attributes
    ldraw = re.findall(r"^\s*_ldraw_\s*=(.*?)$", output, re.MULTILINE)
    assert ldraw is not None, "no _ldraw_ attributes in graph"

    # one of these should contain the label correctly escaped
    assert any(r"hello \\\" world" in l for l in ldraw), "unexpected ldraw contents"


def test_165_3():
    """
    variant of test_165() that checks a similar problem for graph labels
    https://gitlab.com/graphviz/graphviz/-/issues/165
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "165_3.dot"
    assert input.exists(), "unexpectedly missing test case"

    # ask Graphviz to translate it to xdot
    output = dot("xdot", input)

    # find the lines containing _ldraw_ attributes
    ldraw = re.findall(r"^\s*_ldraw_\s*=(.*?)$", output, re.MULTILINE)
    assert ldraw is not None, "no _ldraw_ attributes in graph"

    # one of these should contain the label correctly escaped
    assert any(r"hello \\\" world" in l for l in ldraw), "unexpected ldraw contents"


def test_167():
    """
    using concentrate=true should not result in a segfault
    https://gitlab.com/graphviz/graphviz/-/issues/167
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "167.dot"
    assert input.exists(), "unexpectedly missing test case"

    # process this with dot
    ret = subprocess.call(["dot", "-Tpdf", "-o", os.devnull, input])

    # Graphviz should not have caused a segfault
    assert ret != -signal.SIGSEGV, "Graphviz segfaulted"


def test_191():
    """
    a comma-separated list without quotes should cause a hard error, not a warning
    https://gitlab.com/graphviz/graphviz/-/issues/191
    """

    source = (
        "graph {\n"
        '  "Trackable" [fontcolor=grey45,labelloc=c,fontname=Vera Sans, '
        "DejaVu Sans, Liberation Sans, Arial, Helvetica, sans,shape=box,"
        'height=0.3,align=center,fontsize=10,style="setlinewidth(0.5)"];\n'
        "}"
    )

    with subprocess.Popen(
        ["dot", "-Tdot"],
        stdin=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True,
    ) as p:
        _, stderr = p.communicate(source)

        assert "syntax error" in stderr, "missing error message for unquoted list"

        assert p.returncode != 0, "syntax error was only a warning, not an error"


def test_358():
    """
    setting xdot version to 1.7 should enable font characteristics
    https://gitlab.com/graphviz/graphviz/-/issues/358
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "358.dot"
    assert input.exists(), "unexpectedly missing test case"

    # process this with dot
    xdot = dot("xdot", input)

    for i in range(6):
        m = re.search(f"\\bt {1 << i}\\b", xdot)
        assert m is not None, f"font characteristic {1 << i} not enabled in xdot 1.7"


@pytest.mark.parametrize("attribute", ("samehead", "sametail"))
def test_452(attribute: str):
    """
    more than 5 unique `samehead` and `sametail` values should be usable
    https://gitlab.com/graphviz/graphviz/-/issues/452
    """

    # a graph using more than 5 of the same attribute with the same node on one
    # side of each edge
    graph = io.StringIO()
    graph.write("digraph {\n")
    for i in range(6):
        if attribute == "samehead":
            graph.write(f"  m{i} -> n1")
        else:
            graph.write(f"  n1 -> m{i}")
        graph.write(f'[{attribute}="foo{i}"];\n')
    graph.write("}\n")

    # process this with dot
    dot("svg", source=graph.getvalue())


def test_510():
    """
    HSV colors should also support an alpha channel
    https://gitlab.com/graphviz/graphviz/-/issues/510
    """

    # a graph with a turquoise, partially transparent node
    source = 'digraph { a [color="0.482 0.714 0.878 0.5"]; }'

    # process this with dot
    svg = dot("svg", source=source)

    # see if we can locate an opacity adjustment
    m = re.search(r'\bstroke-opacity="(?P<opacity>\d*.\d*)"', svg)
    assert m is not None, "no stroke-opacity set; alpha channel ignored?"

    # it should be something in-between transparent and opaque
    opacity = float(m.group("opacity"))
    assert opacity > 0, "node set transparent; misinterpreted alpha channel?"
    assert opacity < 1, "node set opaque; misinterpreted alpha channel?"


@pytest.mark.skipif(
    which("gv2gxl") is None or which("gxl2gv") is None, reason="GXL tools not available"
)
def test_517():
    """
    round tripping a graph through gv2gxl should not lose HTML labels
    https://gitlab.com/graphviz/graphviz/-/issues/517
    """

    # our test case input
    input = (
        "digraph{\n"
        "  A[label=<<TABLE><TR><TD>(</TD><TD>A</TD><TD>)</TD></TR></TABLE>>]\n"
        '  B[label="<TABLE><TR><TD>(</TD><TD>B</TD><TD>)</TD></TR></TABLE>"]\n'
        "}"
    )

    # translate it to GXL
    gv2gxl = which("gv2gxl")
    gxl = subprocess.check_output([gv2gxl], input=input, universal_newlines=True)

    # translate this back to Dot
    gxl2gv = which("gxl2gv")
    dot_output = subprocess.check_output([gxl2gv], input=gxl, universal_newlines=True)

    # the result should have both expected labels somewhere
    assert (
        "label=<<TABLE><TR><TD>(</TD><TD>A</TD><TD>)</TD></TR></TABLE>>" in dot_output
    ), "HTML label missing"
    assert (
        'label="<TABLE><TR><TD>(</TD><TD>B</TD><TD>)</TD></TR></TABLE>"' in dot_output
    ), "regular label missing"


def test_793():
    """
    Graphviz should not crash when using VRML output with a non-writable current
    directory
    https://gitlab.com/graphviz/graphviz/-/issues/793
    """

    # create a non-writable directory
    with tempfile.TemporaryDirectory() as tmp:
        t = Path(tmp)
        t.chmod(t.stat().st_mode & ~stat.S_IWRITE)

        # ask the VRML back end to handle a simple graph, using the above as the
        # current working directory
        with subprocess.Popen(["dot", "-Tvrml", "-o", os.devnull], cwd=t) as p:
            p.communicate("digraph { a -> b; }")

            # Graphviz should not have caused a segfault
            assert p.returncode != -signal.SIGSEGV, "Graphviz segfaulted"


def test_797():
    """
    “&;” should not be considered an XML escape sequence
    https://gitlab.com/graphviz/graphviz/-/issues/797
    """

    # some input containing the invalid escape
    input = 'digraph tree {\n"1" [shape="box", label="&amp; &amp;;", URL="a"];\n}'

    # process this with the client-side imagemap back end
    output = dot("cmapx", source=input)

    # the escape sequences should have been preserved
    assert "&amp; &amp;" in output


def test_827():
    """
    Graphviz should not crash when processing the b15.gv example
    https://gitlab.com/graphviz/graphviz/-/issues/827
    """

    b15gv = Path(__file__).parent / "graphs/b15.gv"
    assert b15gv.exists(), "missing test case file"

    dot("svg", b15gv)


def test_925():
    """
    spaces should be handled correctly in UTF-8-containing labels in record shapes
    https://gitlab.com/graphviz/graphviz/-/issues/925
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "925.dot"
    assert input.exists(), "unexpectedly mising test case"

    # process this with dot
    svg = dot("svg", input)

    # The output should include the correctly spaced UTF-8 label. Note that these
    # are not ASCII capital As in this string, but rather UTF-8 Cyrillic Capital
    # Letter As.
    assert "ААА ААА ААА" in svg, "incorrect spacing in UTF-8 label"


def test_1221():
    """
    assigning a node to two clusters with newrank should not cause a crash
    https://gitlab.com/graphviz/graphviz/-/issues/1221
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "1221.dot"
    assert input.exists(), "unexpectedly missing test case"

    # process this with dot
    dot("svg", input)


@pytest.mark.skipif(which("gv2gml") is None, reason="gv2gml not available")
def test_1276():
    """
    quotes within a label should be escaped in translation to GML
    https://gitlab.com/graphviz/graphviz/-/issues/1276
    """

    # DOT input containing a label with quotes
    src = 'digraph test {\n  x[label=<"Label">];\n}'

    # process this to GML
    gml = subprocess.check_output(["gv2gml"], input=src, universal_newlines=True)

    # the unescaped label should not appear in the output
    assert '""Label""' not in gml, "quotes not escaped in label"

    # the escaped label should appear in the output
    assert (
        '"&quot;Label&quot;"' in gml or '"&#34;Label&#34;"' in gml
    ), "escaped label not found in GML output"


@pytest.mark.xfail(
    strict=True, reason="https://gitlab.com/graphviz/graphviz/-/issues/1308"
)
def test_1308():
    """
    processing a malformed graph found by Google Autofuzz should not crash
    https://gitlab.com/graphviz/graphviz/-/issues/1308
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "1308.dot"
    assert input.exists(), "unexpectedly missing test case"

    # run it through Graphviz
    ret = subprocess.call(["dot", "-Tsvg", "-o", os.devnull, input])

    assert ret in (0, 1), "Graphviz crashed when processing malformed input"
    assert ret == 1, "Graphviz did not reject malformed input"


def test_1314():
    """
    test that a large font size that produces an overflow in Pango is rejected
    https://gitlab.com/graphviz/graphviz/-/issues/1314
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "1314.dot"
    assert input.exists(), "unexpectedly missing test case"

    # ask Graphviz to process it, which should fail
    try:
        dot("svg", input)
    except subprocess.CalledProcessError:
        return

    # the execution did not fail as expected
    pytest.fail("dot incorrectly exited with success")


def test_1318():
    """
    processing a large number in a comment should not trigger integer overflow
    https://gitlab.com/graphviz/graphviz/-/issues/1318
    """

    # sample input consisting of a large number in a comment
    source = "#8828066547613302784"

    # processing this should succeed
    dot("svg", source=source)


def test_1332():
    """
    Triangulation calculation on the associated example should succeed.

    A prior change that was intended to increase accuracy resulted in the
    example in this test now failing some triangulation calculations. It is not
    clear whether the outcome before or after is correct, but this test ensures
    that the older behavior users are accustomed to is preserved.
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "1332.dot"
    assert input.exists(), "unexpectedly missing test case"

    # process it with Graphviz
    warnings = subprocess.check_output(
        ["dot", "-Tpdf", "-o", os.devnull, input],
        stderr=subprocess.STDOUT,
        universal_newlines=True,
    )

    # work around macOS warnings
    warnings = remove_xtype_warnings(warnings).strip()

    # no warnings should have been printed
    assert (
        warnings == ""
    ), "warnings were printed when processing graph involving triangulation"


def test_1408():
    """
    parsing particular ortho layouts should not cause an assertion failure
    https://gitlab.com/graphviz/graphviz/-/issues/1408
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "1408.dot"
    assert input.exists(), "unexpectedly missing test case"

    # process it with Graphviz
    dot("svg", input)


def test_1411():
    """
    parsing strings containing newlines should not disrupt line number tracking
    https://gitlab.com/graphviz/graphviz/-/issues/1411
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "1411.dot"
    assert input.exists(), "unexpectedly missing test case"

    # process it with Graphviz (should fail)
    with subprocess.Popen(
        ["dot", "-Tsvg", "-o", os.devnull, input],
        stderr=subprocess.PIPE,
        universal_newlines=True,
    ) as p:
        _, output = p.communicate()

        assert p.returncode != 0, "Graphviz accepted broken input"

    assert (
        "syntax error in line 17 near '\\'" in output
    ), "error message did not identify correct location"


def test_1436():
    """
    test a segfault from https://gitlab.com/graphviz/graphviz/-/issues/1436 has
    not reappeared
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "1436.dot"
    assert input.exists(), "unexpectedly missing test case"

    # ask Graphviz to process it, which should generate a segfault if this bug
    # has been reintroduced
    dot("svg", input)


def test_1444():
    """
    specifying 'headport' as an edge attribute should work regardless of what
    order attributes appear in
    https://gitlab.com/graphviz/graphviz/-/issues/1444
    """

    # locate the first of our associated tests
    input1 = Path(__file__).parent / "1444.dot"
    assert input1.exists(), "unexpectedly missing test case"

    # ask Graphviz to process it
    with subprocess.Popen(
        ["dot", "-Tsvg", input1],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True,
    ) as p:
        stdout1, stderr = p.communicate()

        assert p.returncode == 0, "failed to process a headport edge"

    stderr = remove_xtype_warnings(stderr).strip()
    assert stderr == "", "emitted an error for a legal graph"

    # now locate our second variant, that simply has the attributes swapped
    input2 = Path(__file__).parent / "1444-2.dot"
    assert input2.exists(), "unexpectedly missing test case"

    # process it identically
    with subprocess.Popen(
        ["dot", "-Tsvg", input2],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True,
    ) as p:
        stdout2, stderr = p.communicate()

        assert p.returncode == 0, "failed to process a headport edge"

    stderr = remove_xtype_warnings(stderr).strip()
    assert stderr == "", "emitted an error for a legal graph"

    assert stdout1 == stdout2, "swapping edge attributes altered the output graph"


def test_1449():
    """
    using the SVG color scheme should not cause warnings
    https://gitlab.com/graphviz/graphviz/-/issues/1449
    """

    # start Graphviz
    with subprocess.Popen(
        ["dot", "-Tsvg", "-o", os.devnull],
        stdin=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True,
    ) as p:

        # pass it some input that uses the SVG color scheme
        _, stderr = p.communicate('graph g { colorscheme="svg"; }')

        assert p.returncode == 0, "Graphviz exited with non-zero status"

    assert stderr.strip() == "", "SVG color scheme use caused warnings"


def test_1554():
    """
    small distances between nodes should not cause a crash in majorization
    https://gitlab.com/graphviz/graphviz/-/issues/1554
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "1554.dot"
    assert input.exists(), "unexpectedly missing test case"

    # run it through Graphviz
    output = dot("svg", input)

    # the output should not have NaN values, indicating out of bounds computation
    assert (
        re.search(r"\bnan\b", output, flags=re.IGNORECASE) is None
    ), "computation exceeded bounds"


@pytest.mark.skipif(which("gvpr") is None, reason="GVPR not available")
def test_1594():
    """
    GVPR should give accurate line numbers in error messages
    https://gitlab.com/graphviz/graphviz/-/issues/1594
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "1594.gvpr"

    # run GVPR with our (malformed) input program
    with subprocess.Popen(
        ["gvpr", "-f", input],
        stdin=subprocess.PIPE,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        universal_newlines=True,
    ) as p:
        _, stderr = p.communicate()

        assert p.returncode != 0, "GVPR did not reject malformed program"

    assert "line 3:" in stderr, "GVPR did not identify correct line of syntax error"


@pytest.mark.parametrize("long,short", (("--help", "-?"), ("--version", "-V")))
def test_1618(long: str, short: str):
    """
    Graphviz should understand `--help` and `--version`
    https://gitlab.com/graphviz/graphviz/-/issues/1618
    """

    # run Graphviz with the short form of the argument
    p1 = subprocess.run(
        ["dot", short], stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True
    )

    # run it with the long form of the argument
    p2 = subprocess.run(
        ["dot", long], stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True
    )

    # output from both should match
    assert (
        p1.stdout == p2.stdout
    ), f"`dot {long}` wrote differing output than `dot {short}`"
    assert (
        p1.stderr == p2.stderr
    ), f"`dot {long}` wrote differing output than `dot {short}`"


@pytest.mark.parametrize(
    "test_case", ("1622_0.dot", "1622_1.dot", "1622_2.dot", "1622_3.dot")
)
def test_1622(test_case: str):
    """
    Narrow HTML table cells should not cause assertion failures
    https://gitlab.com/graphviz/graphviz/-/issues/1622
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / test_case
    assert input.exists(), "unexpectedly missing test case"

    # process it with Graphviz
    dot("png:cairo:cairo", input)


@pytest.mark.xfail(strict=True)
def test_1624():
    """
    record shapes should be usable
    https://gitlab.com/graphviz/graphviz/-/issues/1624
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "1624.dot"
    assert input.exists(), "unexpectedly missing test case"

    # process it with Graphviz
    dot("svg", input)


def test_1658():
    """
    the graph associated with this test case should not crash Graphviz
    https://gitlab.com/graphviz/graphviz/-/issues/1658
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "1658.dot"
    assert input.exists(), "unexpectedly missing test case"

    # process it with Graphviz
    dot("png", input)


def test_1676():
    """
    https://gitlab.com/graphviz/graphviz/-/issues/1676
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "1676.dot"
    assert input.exists(), "unexpectedly missing test case"

    # run Graphviz with this input
    ret = subprocess.call(["dot", "-Tsvg", "-o", os.devnull, input])

    # this malformed input should not have caused Graphviz to crash
    assert ret != -signal.SIGSEGV, "Graphviz segfaulted"


def test_1724():
    """
    passing malformed node and newrank should not cause segfaults
    https://gitlab.com/graphviz/graphviz/-/issues/1724
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "1724.dot"
    assert input.exists(), "unexpectedly missing test case"

    # run Graphviz with this input
    ret = subprocess.call(["dot", "-Tsvg", "-o", os.devnull, input])

    assert ret != -signal.SIGSEGV, "Graphviz segfaulted"


def test_1767():
    """
    using the Pango plugin multiple times should produce consistent results
    https://gitlab.com/graphviz/graphviz/-/issues/1767
    """

    # FIXME: Remove skip when
    # https://gitlab.com/graphviz/graphviz/-/issues/1777 is fixed
    if os.getenv("build_system") == "msbuild":
        pytest.skip("Windows MSBuild release does not contain any header files (#1777)")

    # find co-located test source
    c_src = (Path(__file__).parent / "1767.c").resolve()
    assert c_src.exists(), "missing test case"

    # find our co-located dot input
    src = (Path(__file__).parent / "1767.dot").resolve()
    assert src.exists(), "missing test case"

    _, _ = run_c(c_src, [src], link=["cgraph", "gvc"])

    # FIXME: uncomment this when #1767 is fixed
    # assert stdout == "Loaded graph:clusters\n" \
    #                  "cluster_0 contains 5 nodes\n" \
    #                  "cluster_1 contains 1 nodes\n" \
    #                  "cluster_2 contains 3 nodes\n" \
    #                  "cluster_3 contains 3 nodes\n" \
    #                  "Loaded graph:clusters\n" \
    #                  "cluster_0 contains 5 nodes\n" \
    #                  "cluster_1 contains 1 nodes\n" \
    #                  "cluster_2 contains 3 nodes\n" \
    #                  "cluster_3 contains 3 nodes\n"


@pytest.mark.skipif(which("gvpr") is None, reason="GVPR not available")
@pytest.mark.skipif(platform.system() != "Windows", reason="only relevant on Windows")
def test_1780():
    """
    GVPR should accept programs at absolute paths
    https://gitlab.com/graphviz/graphviz/-/issues/1780
    """

    # get absolute path to an arbitrary GVPR program
    clustg = Path(__file__).resolve().parent.parent / "cmd/gvpr/lib/clustg"

    # GVPR should not fail when given this path
    gvpr(clustg)


def test_1783():
    """
    Graphviz should not segfault when passed large edge weights
    https://gitlab.com/graphviz/graphviz/-/issues/1783
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "1783.dot"
    assert input.exists(), "unexpectedly missing test case"

    # run Graphviz with this input
    ret = subprocess.call(["dot", "-Tsvg", "-o", os.devnull, input])

    assert ret != 0, "Graphviz accepted illegal edge weight"

    assert ret != -signal.SIGSEGV, "Graphviz segfaulted"


@pytest.mark.skipif(which("gvedit") is None, reason="Gvedit not available")
def test_1813():
    """
    gvedit -? should show usage
    https://gitlab.com/graphviz/graphviz/-/issues/1813
    """

    environ_copy = os.environ.copy()
    environ_copy.pop("DISPLAY", None)
    output = subprocess.check_output(
        ["gvedit", "-?"], env=environ_copy, universal_newlines=True
    )

    assert "Usage" in output, "gvedit -? did not show usage"


def test_1845():
    """
    rendering sequential graphs to PS should not segfault
    https://gitlab.com/graphviz/graphviz/-/issues/1845
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "1845.dot"
    assert input.exists(), "unexpectedly missing test case"

    # generate a multipage PS file from this input
    dot("ps", input)


@pytest.mark.xfail(strict=True)  # FIXME
def test_1856():
    """
    headports and tailports should be respected
    https://gitlab.com/graphviz/graphviz/-/issues/1856
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "1856.dot"
    assert input.exists(), "unexpectedly missing test case"

    # process it into JSON
    out = dot("json", input)
    data = json.loads(out)

    # find the two nodes, “3” and “5”
    three = [x for x in data["objects"] if x["name"] == "3"][0]
    five = [x for x in data["objects"] if x["name"] == "5"][0]

    # find the edge from “3” to “5”
    edge = [
        x
        for x in data["edges"]
        if x["tail"] == three["_gvid"] and x["head"] == five["_gvid"]
    ][0]

    # The edge should look something like:
    #
    #        ┌─┐
    #        │3│
    #        └┬┘
    #    ┌────┘
    #   ┌┴┐
    #   │5│
    #   └─┘
    #
    # but a bug causes port constraints to not be respected and the edge comes out
    # more like:
    #
    #        ┌─┐
    #        │3│
    #        └┬┘
    #         │
    #   ┌─┐   │
    #   ├5̶┼───┘
    #   └─┘
    #
    # So validate that the edge’s path does not dip below the top of the “5” node.

    top_of_five = max(y for _, y in five["_draw_"][1]["points"])

    waypoints_y = [y for _, y in edge["_draw_"][1]["points"]]

    assert all(y >= top_of_five for y in waypoints_y), "edge dips below 5"


@pytest.mark.skipif(which("fdp") is None, reason="fdp not available")
def test_1865():
    """
    fdp should not read out of bounds when processing node names
    https://gitlab.com/graphviz/graphviz/-/issues/1865
    Note, the crash this test tries to provoke may only occur when run under
    Address Sanitizer or Valgrind
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "1865.dot"
    assert input.exists(), "unexpectedly missing test case"

    # fdp should not crash when processing this file
    subprocess.check_call(["fdp", "-o", os.devnull, input])


@pytest.mark.skipif(which("gv2gml") is None, reason="gv2gml not available")
@pytest.mark.skipif(which("gml2gv") is None, reason="gml2gv not available")
@pytest.mark.parametrize("penwidth", ("1.0", "1"))
def test_1871(penwidth: str):
    """
    round tripping something with either an integer or real `penwidth` through
    gv2gml→gml2gv should return the correct `penwidth`
    """

    # a trivial graph
    input = f"graph {{ a [penwidth={penwidth}] }}"

    # pass it through gv2gml
    gv = subprocess.check_output(["gv2gml"], input=input, universal_newlines=True)

    # pass this through gml2gv
    gml = subprocess.check_output(["gml2gv"], input=gv, universal_newlines=True)

    # the result should have a `penwidth` of 1
    has_1 = re.search(r"\bpenwidth\s*=\s*1[^\.]", gml) is not None
    has_1_0 = re.search(r"\bpenwidth\s*=\s*1\.0\b", gml) is not None
    assert (
        has_1 or has_1_0
    ), f"incorrect penwidth from round tripping through GML (output {gml})"


@pytest.mark.skipif(which("fdp") is None, reason="fdp not available")
def test_1876():
    """
    fdp should not rename nodes with internal names
    https://gitlab.com/graphviz/graphviz/-/issues/1876
    """

    # a trivial graph to provoke this issue
    input = "graph { a }"

    # process this with fdp
    try:
        output = subprocess.check_output(["fdp"], input=input, universal_newlines=True)
    except subprocess.CalledProcessError as e:
        raise RuntimeError("fdp failed to process trivial graph") from e

    # we should not see any internal names like "%3"
    assert "%" not in output, "internal name in fdp output"


@pytest.mark.skipif(which("fdp") is None, reason="fdp not available")
def test_1877():
    """
    fdp should not fail an assertion when processing cluster edges
    https://gitlab.com/graphviz/graphviz/-/issues/1877
    """

    # simple input with a cluster edge
    input = "graph {subgraph cluster_a {}; cluster_a -- b}"

    # fdp should be able to process this
    subprocess.run(
        ["fdp", "-o", os.devnull], input=input, check=True, universal_newlines=True
    )


def test_1880():
    """
    parsing a particular graph should not cause a Trapezoid-table overflow
    assertion failure
    https://gitlab.com/graphviz/graphviz/-/issues/1880
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "1880.dot"
    assert input.exists(), "unexpectedly missing test case"

    # process it with Graphviz
    dot("png", input)


def test_1898():
    """
    test a segfault from https://gitlab.com/graphviz/graphviz/-/issues/1898 has
    not reappeared
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "1898.dot"
    assert input.exists(), "unexpectedly missing test case"

    # ask Graphviz to process it, which should generate a segfault if this bug
    # has been reintroduced
    dot("svg", input)


def test_1902():
    """
    test a segfault from https://gitlab.com/graphviz/graphviz/-/issues/1902 has
    not reappeared
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "1902.dot"
    assert input.exists(), "unexpectedly missing test case"

    # ask Graphviz to process it, which should generate a segfault if this bug
    # has been reintroduced
    dot("svg", input)


# root directory of this checkout
ROOT = Path(__file__).parent.parent.resolve()


def test_1855():
    """
    SVGs should have a scale with sufficient precision
    https://gitlab.com/graphviz/graphviz/-/issues/1855
    """

    # locate our associated test case in this directory
    src = Path(__file__).parent / "1855.dot"
    assert src.exists(), "unexpectedly missing test case"

    # run it through Graphviz
    svg = dot("svg", src)

    # find the graph element
    root = ET.fromstring(svg)
    graph = root[0]
    assert graph.get("class") == "graph", "could not find graph element"

    # extract its `transform` attribute
    transform = graph.get("transform")

    # this should begin with a scale directive
    m = re.match(r"scale\((?P<x>\d+(\.\d*)?) (?P<y>\d+(\.\d*))\)", transform)
    assert m is not None, f"failed to find 'scale' in '{transform}'"

    x = m.group("x")
    y = m.group("y")

    # the scale should be somewhere in reasonable range of what is expected
    assert float(x) >= 0.32 and float(x) <= 0.34, "inaccurate x scale"
    assert float(y) >= 0.32 and float(y) <= 0.34, "inaccurate y scale"

    # two digits of precision are insufficient for this example, so require a
    # greater number of digits in both scale components
    assert len(x) > 4, "insufficient precision in x scale"
    assert len(y) > 4, "insufficient precision in y scale"


@pytest.mark.parametrize("variant", [1, 2])
@pytest.mark.skipif(which("gml2gv") is None, reason="gml2gv not available")
def test_1869(variant: int):
    """
    gml2gv should be able to parse the style, outlineStyle, width and
    outlineWidth GML attributes and map them to the DOT attributes
    style and penwidth respectively
    https://gitlab.com/graphviz/graphviz/-/issues/1869
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / f"1869-{variant}.gml"
    assert input.exists(), "unexpectedly missing test case"

    # ask gml2gv to translate it to DOT
    output = subprocess.check_output(["gml2gv", input], universal_newlines=True)

    assert "style=dashed" in output, "style=dashed not found in DOT output"
    assert "penwidth=2" in output, "penwidth=2 not found in DOT output"


def test_1879():
    """https://gitlab.com/graphviz/graphviz/-/issues/1879"""

    # locate our associated test case in this directory
    input = Path(__file__).parent / "1879.dot"
    assert input.exists(), "unexpectedly missing test case"

    # process it with DOT
    stdout = subprocess.check_output(
        ["dot", "-Tsvg", "-o", os.devnull, input],
        cwd=Path(__file__).parent,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
    )

    # check we did not trigger an assertion failure
    assert re.search(r"\bAssertion\b.*\bfailed\b", stdout) is None


def test_1879_2():
    """
    another variant of lhead/ltail + compound
    https://gitlab.com/graphviz/graphviz/-/issues/1879
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "1879-2.dot"
    assert input.exists(), "unexpectedly missing test case"

    # process it with DOT
    subprocess.check_call(["dot", "-Gmargin=0", "-Tpng", "-o", os.devnull, input])


def test_1893():
    """
    an HTML label containing just a ] should work
    https://gitlab.com/graphviz/graphviz/-/issues/1893
    """

    # a graph containing a node with an HTML label with a ] in a table cell
    input = "digraph { 0 [label=<<TABLE><TR><TD>]</TD></TR></TABLE>>] }"

    # ask Graphviz to process this
    dot("svg", source=input)

    # we should be able to do the same with an escaped ]
    input = "digraph { 0 [label=<<TABLE><TR><TD>&#93;</TD></TR></TABLE>>] }"

    dot("svg", source=input)


def test_1906():
    """
    graphs that generate large rectangles should be accepted
    https://gitlab.com/graphviz/graphviz/-/issues/1906
    """

    # one of the rtest graphs is sufficient to provoke this
    input = Path(__file__).parent / "graphs/root.gv"
    assert input.exists(), "unexpectedly missing test case"

    # use Circo to translate it to DOT
    subprocess.check_call(["dot", "-Kcirco", "-Tgv", "-o", os.devnull, input])


@pytest.mark.skipif(which("twopi") is None, reason="twopi not available")
def test_1907():
    """
    SVG edges should have title elements that match their names
    https://gitlab.com/graphviz/graphviz/-/issues/1907
    """

    # a trivial graph to provoke this issue
    input = "digraph { A -> B -> C }"

    # generate an SVG from this input with twopi
    output = subprocess.check_output(
        ["twopi", "-Tsvg"], input=input, universal_newlines=True
    )

    assert "<title>A&#45;&gt;B</title>" in output, "element title not found in SVG"


@pytest.mark.skipif(which("gvpr") is None, reason="gvpr not available")
def test_1909():
    """
    GVPR should not output internal names
    https://gitlab.com/graphviz/graphviz/-/issues/1909
    """

    # locate our associated test case in this directory
    prog = Path(__file__).parent / "1909.gvpr"
    graph = Path(__file__).parent / "1909.dot"

    # run GVPR with the given input
    output = subprocess.check_output(
        ["gvpr", "-c", "-f", prog, graph], universal_newlines=True
    )

    # we should have produced this graph without names like "%2" in it
    assert output == "// begin\n" "digraph bug {\n" "	a -> b;\n" "	b -> c;\n" "}\n"


def test_1910():
    """
    Repeatedly using agmemread() should have consistent results
    https://gitlab.com/graphviz/graphviz/-/issues/1910
    """

    # FIXME: Remove skip when
    # https://gitlab.com/graphviz/graphviz/-/issues/1777 is fixed
    if os.getenv("build_system") == "msbuild":
        pytest.skip("Windows MSBuild release does not contain any header files (#1777)")

    # find co-located test source
    c_src = (Path(__file__).parent / "1910.c").resolve()
    assert c_src.exists(), "missing test case"

    # run the test
    _, _ = run_c(c_src, link=["cgraph", "gvc"])


def test_1913():
    """
    ALIGN attributes in <BR> tags should be parsed correctly
    https://gitlab.com/graphviz/graphviz/-/issues/1913
    """

    # a template of a trivial graph using an ALIGN attribute
    graph = (
        "digraph {{\n"
        '  table1[label=<<table><tr><td align="text">hello world'
        '<br align="{}"/></td></tr></table>>];\n'
        "}}"
    )

    def run(input):
        """
        run Dot with the given input and return its exit status and stderr
        """
        with subprocess.Popen(
            ["dot", "-Tsvg", "-o", os.devnull],
            stdin=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
        ) as p:
            _, stderr = p.communicate(input)
            return p.returncode, remove_xtype_warnings(stderr)

    # Graphviz should accept all legal values for this attribute
    for align in ("left", "right", "center"):

        input = align
        ret, stderr = run(graph.format(input))
        assert ret == 0
        assert stderr.strip() == ""

        # these attributes should also be valid when title cased
        input = f"{align[0].upper()}{align[1:]}"
        ret, stderr = run(graph.format(input))
        assert ret == 0
        assert stderr.strip() == ""

        # similarly, they should be valid when upper cased
        input = align.upper()
        ret, stderr = run(graph.format(input))
        assert ret == 0
        assert stderr.strip() == ""

    # various invalid things that have the same prefix or suffix as a valid
    # alignment should be rejected
    for align in ("lamp", "deft", "round", "might", "circle", "venter"):

        input = align
        _, stderr = run(graph.format(input))
        assert f"Warning: Illegal value {input} for ALIGN - ignored" in stderr

        # these attributes should also fail when title cased
        input = f"{align[0].upper()}{align[1:]}"
        _, stderr = run(graph.format(input))
        assert f"Warning: Illegal value {input} for ALIGN - ignored" in stderr

        # similarly, they should fail when upper cased
        input = align.upper()
        _, stderr = run(graph.format(input))
        assert f"Warning: Illegal value {input} for ALIGN - ignored" in stderr


def test_1931():
    """
    New lines within strings should not be discarded during parsing

    """

    # a graph with \n inside of strings
    graph = (
        "graph {\n"
        '  node1 [label="line 1\n'
        "line 2\n"
        '"];\n'
        '  node2 [label="line 3\n'
        'line 4"];\n'
        "  node1 -- node2\n"
        '  node2 -- "line 5\n'
        'line 6"\n'
        "}"
    )

    # ask Graphviz to process this to dot output
    xdot = dot("xdot", source=graph)

    # all new lines in strings should have been preserved
    assert "line 1\nline 2\n" in xdot
    assert "line 3\nline 4" in xdot
    assert "line 5\nline 6" in xdot


@pytest.mark.xfail()  # FIXME
def test_1949():
    """
    rankdir=LR + compound=true should not lead to an assertion failure
    https://gitlab.com/graphviz/graphviz/-/issues/1949
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "1949.dot"
    assert input.exists(), "unexpectedly missing test case"

    # run it through Graphviz
    dot("png", input)


@pytest.mark.skipif(which("edgepaint") is None, reason="edgepaint not available")
def test_1971():
    """
    edgepaint should reject invalid command line options
    https://gitlab.com/graphviz/graphviz/-/issues/1971
    """

    # a basic graph that edgepaint can process
    input = (
        "digraph {\n"
        '  graph [bb="0,0,54,108"];\n'
        '  node [label="\\N"];\n'
        "  a       [height=0.5,\n"
        '           pos="27,90",\n'
        "           width=0.75];\n"
        "  b       [height=0.5,\n"
        '           pos="27,18",\n'
        "           width=0.75];\n"
        '  a -> b  [pos="e,27,36.104 27,71.697 27,63.983 27,54.712 27,46.112"];\n'
        "}"
    )

    # run edgepaint with an invalid option, `-rabbit`, that happens to have the
    # same first character as valid options
    args = ["edgepaint", "-rabbit"]
    with subprocess.Popen(args, stdin=subprocess.PIPE, universal_newlines=True) as p:
        p.communicate(input)

        assert p.returncode != 0, "edgepaint incorrectly accepted '-rabbit'"


def test_1990():
    """
    using ortho and circo in combination should not cause an assertion failure
    https://gitlab.com/graphviz/graphviz/-/issues/14
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "1990.dot"
    assert input.exists(), "unexpectedly missing test case"

    # process it with Graphviz
    subprocess.check_call(["circo", "-Tsvg", "-o", os.devnull, input])


def test_2057():
    """
    gvToolTred should be usable by user code
    https://gitlab.com/graphviz/graphviz/-/issues/2057
    """

    # FIXME: Remove skip when
    # https://gitlab.com/graphviz/graphviz/-/issues/1777 is fixed
    if os.getenv("build_system") == "msbuild":
        pytest.skip("Windows MSBuild release does not contain any header files (#1777)")

    # find co-located test source
    c_src = (Path(__file__).parent / "2057.c").resolve()
    assert c_src.exists(), "missing test case"

    # run the test
    _, _ = run_c(c_src, link=["gvc"])


def test_2078():
    """
    Incorrectly using the "layout" attribute on a subgraph should result in a
    sensible error.
    https://gitlab.com/graphviz/graphviz/-/issues/2078
    """

    # our sample graph that incorrectly uses layout
    input = "graph {\n  subgraph {\n    layout=osage\n  }\n}"

    # run it through Graphviz
    with subprocess.Popen(
        ["dot", "-Tcanon", "-o", os.devnull],
        stdin=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True,
    ) as p:
        _, stderr = p.communicate(input)

        assert p.returncode != 0, "layout on subgraph was incorrectly accepted"

    assert (
        "layout attribute is invalid except on the root graph" in stderr
    ), "expected warning not found"

    # a graph that correctly uses layout
    input = "graph {\n  layout=osage\n  subgraph {\n  }\n}"

    # ensure this one does not trigger warnings
    with subprocess.Popen(
        ["dot", "-Tcanon", "-o", os.devnull],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True,
    ) as p:
        stdout, stderr = p.communicate(input)

        assert p.returncode == 0, "correct layout use was rejected"

    assert stdout.strip() == "", "unexpected output"
    assert (
        "layout attribute is invalid except on the root graph" not in stderr
    ), "incorrect warning output"


def test_2082():
    """
    Check a bug in inside_polygon has not been reintroduced.
    https://gitlab.com/graphviz/graphviz/-/issues/2082
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "2082.dot"
    assert input.exists(), "unexpectedly missing test case"

    # ask Graphviz to process it, which should generate an assertion failure if
    # this bug has been reintroduced
    dot("png", input)


def test_2087():
    """
    spline routing should be aware of and ignore concentrated edges
    https://gitlab.com/graphviz/graphviz/-/issues/2087
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "2087.dot"
    assert input.exists(), "unexpectedly missing test case"

    # process it with Graphviz
    warnings = subprocess.check_output(
        ["dot", "-Tpng", "-o", os.devnull, input],
        stderr=subprocess.STDOUT,
        universal_newlines=True,
    )

    # work around macOS warnings
    warnings = remove_xtype_warnings(warnings).strip()

    # no warnings should have been printed
    assert (
        warnings == ""
    ), "warnings were printed when processing concentrated duplicate edges"


@pytest.mark.xfail(strict=True)
@pytest.mark.parametrize("html_like_first", (False, True))
def test_2089(html_like_first: bool):  # FIXME
    """
    HTML-like and non-HTML-like strings should peacefully coexist
    https://gitlab.com/graphviz/graphviz/-/issues/2089
    """

    # a graph using an HTML-like string and a non-HTML-like string
    if html_like_first:
        graph = 'graph {\n  a[label=<foo>];\n  b[label="foo"];\n}'
    else:
        graph = 'graph {\n  a[label="foo"];\n  b[label=<foo>];\n}'

    # normalize the graph
    canonical = dot("dot", source=graph)

    assert "label=foo" in canonical, "non-HTML-like label not found"
    assert "label=<foo>" in canonical, "HTML-like label not found"


@pytest.mark.xfail(strict=True)  # FIXME
def test_2089_2():
    """
    HTML-like and non-HTML-like strings should peacefully coexist
    https://gitlab.com/graphviz/graphviz/-/issues/2089
    """

    # find co-located test source
    c_src = (Path(__file__).parent / "2089.c").resolve()
    assert c_src.exists(), "missing test case"

    # run it
    _, _ = run_c(c_src, link=["cgraph"])


@pytest.mark.skipif(which("dot2gxl") is None, reason="dot2gxl not available")
def test_2092():
    """
    an empty node ID should not cause a dot2gxl NULL pointer dereference
    https://gitlab.com/graphviz/graphviz/-/issues/2092
    """
    p = subprocess.run(["dot2gxl", "-d"], input='<node id="">', universal_newlines=True)

    assert p.returncode != 0, "dot2gxl accepted invalid input"

    assert p.returncode == 1, "dot2gxl crashed"


@pytest.mark.skipif(which("dot2gxl") is None, reason="dot2gxl not available")
def test_2093():
    """
    dot2gxl should handle elements with no ID
    https://gitlab.com/graphviz/graphviz/-/issues/2093
    """
    with subprocess.Popen(
        ["dot2gxl", "-d"], stdin=subprocess.PIPE, universal_newlines=True
    ) as p:
        p.communicate('<graph x="">')

        assert p.returncode == 1, "dot2gxl did not reject missing ID"


@pytest.mark.skipif(which("dot2gxl") is None, reason="dot2gxl not available")
def test_2094():
    """
    dot2gxl should not crash when decoding a closing node tag after a closing
    graph tag
    https://gitlab.com/graphviz/graphviz/-/issues/2094
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "2094.xml"
    assert input.exists(), "unexpectedly missing test case"

    dot2gxl = which("dot2gxl")
    ret = subprocess.call([dot2gxl, "-d", input])

    assert ret in (
        0,
        1,
    ), "dot2gxl crashed when processing a closing node tag after a closing graph tag"
    assert ret == 1, "dot2gxl did not reject malformed XML"


@pytest.mark.skipif(
    os.environ.get("build_system") == "msbuild"
    and os.environ.get("configuration") == "Debug",
    reason="Graphviz built with MSBuild in Debug mode has an "
    "insufficient stack size for this test",
)
def test_2095():
    """
    Exceeding 1000 boxes during computation should not cause a crash
    https://gitlab.com/graphviz/graphviz/-/issues/2095
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "2095.dot"
    assert input.exists(), "unexpectedly missing test case"

    # ask Graphviz to process it
    dot("pdf", input)


@pytest.mark.skipif(which("gv2gml") is None, reason="gv2gml not available")
def test_2131():
    """
    gv2gml should be able to process basic Graphviz input
    https://gitlab.com/graphviz/graphviz/-/issues/2131
    """

    # a trivial graph
    input = "digraph { a -> b; }"

    # ask gv2gml what it thinks of this
    try:
        subprocess.run(["gv2gml"], input=input, check=True, universal_newlines=True)
    except subprocess.CalledProcessError as e:
        raise RuntimeError("gv2gml rejected a basic graph") from e


@pytest.mark.skipif(which("gvpr") is None, reason="gvpr not available")
@pytest.mark.parametrize("examine", ("indices", "tokens"))
def test_2138(examine: str):
    """
    gvpr splitting and tokenizing should not result in trailing garbage
    https://gitlab.com/graphviz/graphviz/-/issues/2138
    """

    # find our co-located GVPR program
    script = (Path(__file__).parent / "2138.gvpr").resolve()
    assert script.exists(), "missing test case"

    # run it with NUL input
    out = subprocess.check_output(["gvpr", "-f", script], stdin=subprocess.DEVNULL)

    # Decode into text. We do this instead of `universal_newlines=True` above
    # because the trailing garbage can contain invalid UTF-8 data causing cryptic
    # failures. We want to correctly surface this as trailing garbage, not an
    # obscure UTF-8 decoding error.
    result = out.decode("utf-8", "replace")

    if examine == "indices":
        # check no indices are miscalculated
        index_re = (
            r"^// index of space \(st\) :\s*(?P<index>-?\d+)\s*<< must "
            r"NOT be less than -1$"
        )
        for m in re.finditer(index_re, result, flags=re.MULTILINE):
            index = int(m.group("index"))
            assert index >= -1, "illegal index computed"

    if examine == "tokens":
        # check for text the author of 2138.gvpr expected to find
        assert (
            "// tok[3]    >>3456789<<   should NOT include trailing spaces or "
            "junk chars" in result
        ), "token 3456789 not found or has trailing garbage"
        assert (
            "// tok[7]    >>012<<   should NOT include trailing spaces or "
            "junk chars" in result
        ), "token 012 not found or has trailing garbage"


def test_2179():
    """
    processing a label with an empty line should not yield a warning
    https://gitlab.com/graphviz/graphviz/-/issues/2179
    """

    # a graph containing a label with an empty line
    input = 'digraph "" {\n  0 -> 1 [fontname="Lato",label=<<br/>1>]\n}'

    # run a graph with an empty label through Graphviz
    with subprocess.Popen(
        ["dot", "-Tsvg", "-o", os.devnull],
        stdin=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True,
    ) as p:
        _, stderr = p.communicate(input)

        assert p.returncode == 0

    assert (
        "Warning: no hard-coded metrics for" not in stderr
    ), "incorrect warning triggered"


def test_2179_1():
    """
    processing a label with a line containing only a space should not yield a
    warning
    https://gitlab.com/graphviz/graphviz/-/issues/2179
    """

    # a graph containing a label with a line containing only a space
    input = 'digraph "" {\n  0 -> 1 [fontname="Lato",label=< <br/>1>]\n}'

    # run a graph with an empty label through Graphviz
    with subprocess.Popen(
        ["dot", "-Tsvg", "-o", os.devnull],
        stdin=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True,
    ) as p:
        _, stderr = p.communicate(input)

        assert p.returncode == 0

    assert (
        "Warning: no hard-coded metrics for" not in stderr
    ), "incorrect warning triggered"


@pytest.mark.skipif(which("nop") is None, reason="nop not available")
def test_2184_1():
    """
    nop should not reposition labelled graph nodes
    https://gitlab.com/graphviz/graphviz/-/issues/2184
    """

    # run `nop` on a sample with a labelled graph node at the end
    source = Path(__file__).parent / "2184.dot"
    assert source.exists(), "missing test case"
    nopped = subprocess.check_output(["nop", source], universal_newlines=True)

    # the normalized output should have a graph with no label within
    # `clusterSurround1`
    m = re.search(
        r"\bclusterSurround1\b.*\bgraph\b.*\bcluster1\b", nopped, flags=re.DOTALL
    )
    assert m is not None, "nop rearranged a graph in a not-semantically-preserving way"


def test_2184_2():
    """
    canonicalization should not reposition labelled graph nodes
    https://gitlab.com/graphviz/graphviz/-/issues/2184
    """

    # canonicalize a sample with a labelled graph node at the end
    source = Path(__file__).parent / "2184.dot"
    assert source.exists(), "missing test case"
    canonicalized = dot("canon", source)

    # the canonicalized output should have a graph with no label within
    # `clusterSurround1`
    m = re.search(
        r"\bclusterSurround1\b.*\bgraph\b.*\bcluster1\b", canonicalized, flags=re.DOTALL
    )
    assert (
        m is not None
    ), "`dot -Tcanon` rearranged a graph in a not-semantically-preserving way"


def test_2185_1():
    """
    GVPR should deal with strings correctly
    https://gitlab.com/graphviz/graphviz/-/issues/2185
    """

    # find our collocated GVPR program
    script = Path(__file__).parent / "2185.gvpr"
    assert script.exists(), "missing test case"

    # run this with NUL input, checking output is valid UTF-8
    gvpr(script)


def test_2185_2():
    """
    GVPR should deal with strings correctly
    https://gitlab.com/graphviz/graphviz/-/issues/2185
    """

    # find our collocated GVPR program
    script = Path(__file__).parent / "2185.gvpr"
    assert script.exists(), "missing test case"

    # run this with NUL input
    out = subprocess.check_output(["gvpr", "-f", script], stdin=subprocess.DEVNULL)

    # decode output in a separate step to gracefully cope with garbage unicode
    out = out.decode("utf-8", "replace")

    # deal with Windows eccentricities
    eol = "\r\n" if platform.system() == "Windows" else "\n"
    expected = f"one two three{eol}"

    # check the first line is as expected
    assert out.startswith(expected), "incorrect GVPR interpretation"


def test_2185_3():
    """
    GVPR should deal with strings correctly
    https://gitlab.com/graphviz/graphviz/-/issues/2185
    """

    # find our collocated GVPR program
    script = Path(__file__).parent / "2185.gvpr"
    assert script.exists(), "missing test case"

    # run this with NUL input
    out = subprocess.check_output(["gvpr", "-f", script], stdin=subprocess.DEVNULL)

    # decode output in a separate step to gracefully cope with garbage unicode
    out = out.decode("utf-8", "replace")

    # deal with Windows eccentricities
    eol = "\r\n" if platform.system() == "Windows" else "\n"
    expected = f"one two three{eol}one  five three{eol}"

    # check the first two lines are as expected
    assert out.startswith(expected), "incorrect GVPR interpretation"


def test_2185_4():
    """
    GVPR should deal with strings correctly
    https://gitlab.com/graphviz/graphviz/-/issues/2185
    """

    # find our collocated GVPR program
    script = Path(__file__).parent / "2185.gvpr"
    assert script.exists(), "missing test case"

    # run this with NUL input
    out = subprocess.check_output(["gvpr", "-f", script], stdin=subprocess.DEVNULL)

    # decode output in a separate step to gracefully cope with garbage unicode
    out = out.decode("utf-8", "replace")

    # deal with Windows eccentricities
    eol = "\r\n" if platform.system() == "Windows" else "\n"
    expected = f"one two three{eol}one  five three{eol}99{eol}"

    # check the first three lines are as expected
    assert out.startswith(expected), "incorrect GVPR interpretation"


def test_2185_5():
    """
    GVPR should deal with strings correctly
    https://gitlab.com/graphviz/graphviz/-/issues/2185
    """

    # find our collocated GVPR program
    script = Path(__file__).parent / "2185.gvpr"
    assert script.exists(), "missing test case"

    # run this with NUL input
    out = subprocess.check_output(["gvpr", "-f", script], stdin=subprocess.DEVNULL)

    # decode output in a separate step to gracefully cope with garbage unicode
    out = out.decode("utf-8", "replace")

    # deal with Windows eccentricities
    eol = "\r\n" if platform.system() == "Windows" else "\n"
    expected = f"one two three{eol}one  five three{eol}99{eol}Constant{eol}"

    # check the first four lines are as expected
    assert out.startswith(expected), "incorrect GVPR interpretation"


@pytest.mark.xfail(strict=True)  # FIXME
def test_2193():
    """
    the canonical format should be stable
    https://gitlab.com/graphviz/graphviz/-/issues/2193
    """

    # find our collocated test case
    input = Path(__file__).parent / "2193.dot"
    assert input.exists(), "unexpectedly missing test case"

    # derive the initial canonicalization
    canonical = dot("canon", input)

    # now canonicalize this again to see if it changes
    new = dot("canon", source=canonical)
    assert canonical == new, "canonical translation is not stable"


@pytest.mark.skipif(which("gvpr") is None, reason="GVPR not available")
def test_2211():
    """
    GVPR’s `index` function should return correct results
    https://gitlab.com/graphviz/graphviz/-/issues/2211
    """

    # find our collocated test case
    program = Path(__file__).parent / "2211.gvpr"
    assert program.exists(), "unexpectedly missing test case"

    # run it through GVPR
    output = gvpr(program)

    # it should have found the right string indices for characters
    assert (
        output == "index: 9  should be 9\n"
        "index: 3  should be 3\n"
        "index: -1  should be -1\n"
    )


def test_2215():
    """
    Graphviz should not crash with `-v`
    https://gitlab.com/graphviz/graphviz/-/issues/2215
    """

    # try it on a simple graph
    input = "graph g { a -- b; }"
    subprocess.run(["dot", "-v"], input=input, check=True, universal_newlines=True)

    # try the same on a labelled version of this graph
    input = 'graph g { node[label=""] a -- b; }'
    subprocess.run(["dot", "-v"], input=input, check=True, universal_newlines=True)


def test_2342():
    """
    using an arrow with size 0 should not trigger an assertion failure
    https://gitlab.com/graphviz/graphviz/-/issues/2342
    """

    # find our collocated test case
    input = Path(__file__).parent / "2342.dot"
    assert input.exists(), "unexpectedly missing test case"

    # run it through Graphviz
    dot("svg", input)


@pytest.mark.xfail(
    reason="https://gitlab.com/graphviz/graphviz/-/issues/2356",
)
def test_2356():
    """
    Using `mindist` programmatically in a loop should not cause Windows crashes
    https://gitlab.com/graphviz/graphviz/-/issues/2356
    """

    # find co-located test source
    c_src = (Path(__file__).parent / "2356.c").resolve()
    assert c_src.exists(), "missing test case"

    # run the test
    run_c(c_src, link=["cgraph", "gvc"])


def test_2361():
    """
    using `ortho` and `concentrate` in combination should not cause a crash
    https://gitlab.com/graphviz/graphviz/-/issues/2361
    """

    # find our collocated test case
    input = Path(__file__).parent / "2361.dot"
    assert input.exists(), "unexpectedly missing test case"

    # run it through Graphviz
    dot("png", input)


def test_package_version():
    """
    The graphviz_version.h header should define a non-empty PACKAGE_VERSION
    """

    # FIXME: Remove skip when
    # https://gitlab.com/graphviz/graphviz/-/issues/1777 is fixed
    if os.getenv("build_system") == "msbuild":
        pytest.skip("Windows MSBuild release does not contain any header files (#1777)")

    # find co-located test source
    c_src = (Path(__file__).parent / "check-package-version.c").resolve()
    assert c_src.exists(), "missing test case"

    # run the test
    _, _ = run_c(c_src)


def test_user_shapes():
    """
    Graphviz should understand how to embed a custom SVG image as a node’s shape
    """

    # find our collocated test case
    input = Path(__file__).parent / "usershape.dot"
    assert input.exists(), "unexpectedly missing test case"

    # ask Graphviz to translate this to SVG
    output = subprocess.check_output(
        ["dot", "-Tsvg", input], cwd=os.path.dirname(__file__), universal_newlines=True
    )

    # the external SVG should have been parsed and is now referenced
    assert '<image xlink:href="usershape.svg" width="62px" height="44px" ' in output


def test_xdot_json():
    """
    check the output of xdot’s JSON API
    """

    # find our collocated C helper
    c_src = Path(__file__).parent / "xdot2json.c"

    # some valid xdot commands to process
    input = "c 9 -#fffffe00 C 7 -#ffffff P 4 0 0 0 36 54 36 54 0"

    # ask our C helper to process this
    try:
        output, err = run_c(c_src, input=input, link=["xdot"])
    except subprocess.CalledProcessError:
        # FIXME: Remove this try-catch when
        # https://gitlab.com/graphviz/graphviz/-/issues/1777 is fixed
        if os.getenv("build_system") == "msbuild":
            pytest.skip(
                "Windows MSBuild release does not contain any header files (#1777)"
            )
        raise
    assert err == ""

    if os.getenv("build_system") == "msbuild":
        pytest.fail(
            "Windows MSBuild unexpectedly passed compilation of a "
            "Graphviz header. Remove the above try-catch? (#1777)"
        )

    # confirm the output was what we expected
    data = json.loads(output)
    assert data == [
        {"c": "#fffffe00"},
        {"C": "#ffffff"},
        {"P": [0.0, 0.0, 0.0, 36.0, 54.0, 36.0, 54.0, 0.0]},
    ]


# find all .vcxproj files
VCXPROJS: List[Path] = []
for repo_root, _, files in os.walk(ROOT):
    for stem in files:
        # skip files generated by MSBuild itself
        if stem in (
            "VCTargetsPath.vcxproj",
            "CompilerIdC.vcxproj",
            "CompilerIdCXX.vcxproj",
        ):
            continue
        full_path = Path(repo_root) / stem
        if full_path.suffix != ".vcxproj":
            continue
        VCXPROJS.append(full_path)


@pytest.mark.parametrize("vcxproj", VCXPROJS)
def test_vcxproj_inclusive(vcxproj: Path):
    """check .vcxproj files correspond to .vcxproj.filters files"""

    def fix_sep(path: str) -> str:
        """translate Windows path separators to ease running this on non-Windows"""
        return path.replace("\\", os.sep)

    # FIXME: files missing a filters file
    FILTERS_WAIVERS = (Path("lib/version/version.vcxproj"),)

    filters = Path(f"{str(vcxproj)}.filters")
    if vcxproj.relative_to(ROOT) not in FILTERS_WAIVERS:
        assert filters.exists(), f"no {str(filters)} corresponding to {str(vcxproj)}"

    # namespace the MSBuild elements live in
    ns = "http://schemas.microsoft.com/developer/msbuild/2003"

    # parse XML out of the .vcxproj file
    srcs1 = set()
    tree = ET.parse(vcxproj)
    root = tree.getroot()
    for elem in root:
        if elem.tag == f"{{{ns}}}ItemGroup":
            for child in elem:
                if child.tag in (f"{{{ns}}}ClInclude", f"{{{ns}}}ClCompile"):
                    filename = fix_sep(child.attrib["Include"])
                    assert (
                        filename not in srcs1
                    ), f"duplicate source {filename} in {str(vcxproj)}"
                    srcs1.add(filename)

    # parse XML out of the .vcxproj.filters file
    if filters.exists():
        srcs2 = set()
        tree = ET.parse(filters)
        root = tree.getroot()
        for elem in root:
            if elem.tag == f"{{{ns}}}ItemGroup":
                for child in elem:
                    if child.tag in (f"{{{ns}}}ClInclude", f"{{{ns}}}ClCompile"):
                        filename = fix_sep(child.attrib["Include"])
                        assert (
                            filename not in srcs2
                        ), f"duplicate source {filename} in {str(filters)}"
                        srcs2.add(filename)

        assert (
            srcs1 == srcs2
        ), "mismatch between sources in {str(vcxproj)} and {str(filters)}"


@pytest.mark.xfail()  # FIXME: fails on CentOS 7/8, macOS Autotools, MSBuild
@pytest.mark.skipif(which("gvmap") is None, reason="gvmap not available")
def test_gvmap_fclose():
    """
    gvmap should not attempt to fclose(NULL). This example will trigger a crash if
    this bug has been reintroduced and Graphviz is built with ASan support.
    """

    # a reasonable input graph
    input = (
        'graph "Alík: Na vlastní oči" {\n'
        '	graph [bb="0,0,128.9,36",\n'
        "		concentrate=true,\n"
        "		overlap=prism,\n"
        "		start=3\n"
        "	];\n"
        '	node [label="\\N"];\n'
        "	{\n"
        "		bob	[height=0.5,\n"
        '			pos="100.95,18",\n'
        "			width=0.77632];\n"
        "	}\n"
        "	{\n"
        "		alice	[height=0.5,\n"
        '			pos="32.497,18",\n'
        "			width=0.9027];\n"
        "	}\n"
        '	alice -- bob	[pos="65.119,18 67.736,18 70.366,18 72.946,18"];\n'
        "	bob -- alice;\n"
        "}"
    )

    # pass this through gvmap
    subprocess.run(["gvmap"], input=input.encode("utf-8"), check=True)


@pytest.mark.skipif(which("gvpr") is None, reason="gvpr not available")
def test_gvpr_usage():
    """
    gvpr usage information should be included when erroring on a malformed command
    """

    # create a temporary directory, under which we know no files will exist
    with tempfile.TemporaryDirectory() as tmp:

        # ask GVPR to process a non-existent file
        with subprocess.Popen(
            ["gvpr", "-f", "nofile"],
            stderr=subprocess.PIPE,
            cwd=tmp,
            universal_newlines=True,
        ) as p:
            _, stderr = p.communicate()

            assert p.returncode != 0, "GVPR accepted a non-existent file"

    # the stderr output should have contained full usage instructions
    assert (
        "-o <ofile> - write output to <ofile>; stdout by default" in stderr
    ), "truncated or malformed GVPR usage information"


def test_2225():
    """
    sfdp should not segfault with curved splines
    https://gitlab.com/graphviz/graphviz/-/issues/2225
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "2225.dot"
    assert input.exists(), "unexpectedly missing test case"

    # run this through sfdp
    p = subprocess.run(
        ["sfdp", "-Gsplines=curved", "-o", os.devnull, input],
        stderr=subprocess.PIPE,
        universal_newlines=True,
    )

    # if sfdp was built without libgts, it will not handle anything non-trivial
    no_gts_error = "remove_overlap: Graphviz not built with triangulation library"
    if no_gts_error in p.stderr:
        assert p.returncode != 0, "sfdp returned success after an error message"
        return

    p.check_returncode()


def test_2257():
    """
    `$GV_FILE_PATH` being set should prevent Graphviz from running

    `$GV_FILE_PATH` was an environment variable formerly used to implement a file
    system sandboxing policy when Graphviz was exposed to the internet via a web
    server. These days, there are safer and more robust techniques to sandbox
    Graphviz and so `$GV_FILE_PATH` usage has been removed. But if someone
    attempts to use this legacy mechanism, we do not want Graphviz to
    “fail-open,” starting anyway and silently ignoring `$GV_FILE_PATH` giving
    the user the false impression the sandboxing is in force.

    https://gitlab.com/graphviz/graphviz/-/issues/2257
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "2257.dot"
    assert input.exists(), "unexpectedly missing test case"

    env = os.environ.copy()
    env["GV_FILE_PATH"] = "/tmp"

    # Graphviz should refuse to process an input file
    with pytest.raises(subprocess.CalledProcessError):
        subprocess.check_call(["dot", "-Tsvg", input, "-o", os.devnull], env=env)


def test_2258():
    """
    'id' attribute should be propagated to all graph children in output
    https://gitlab.com/graphviz/graphviz/-/issues/2258
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "2258.dot"
    assert input.exists(), "unexpectedly missing test case"

    # translate this to SVG
    svg = dot("svg", input)

    # load this as XML
    root = ET.fromstring(svg)

    # the output is expected to contain a number of linear gradients, all of which
    # are semantic children of graph marked `id = "G2"`
    gradients = root.findall(".//{http://www.w3.org/2000/svg}linearGradient")
    assert len(gradients) > 0, "no gradients in output"

    for gradient in gradients:
        assert "G2" in gradient.get("id"), "ID was not applied to linear gradients"


def test_2270(tmp_path: Path):
    """
    `-O` should result in the expected output filename
    https://gitlab.com/graphviz/graphviz/-/issues/2270
    """

    # write a simple graph
    input = tmp_path / "hello.gv"
    input.write_text("digraph { hello -> world }", encoding="utf-8")

    # process it with Graphviz
    subprocess.check_call(
        ["dot", "-T", "plain:dot:core", "-O", "hello.gv"], cwd=tmp_path
    )

    # it should have produced output in the expected location
    output = tmp_path / "hello.gv.core.dot.plain"
    assert output.exists(), "-O resulted in an unexpected output filename"


def test_2272():
    """
    using `agmemread` with an unterminated string should not fail assertions
    https://gitlab.com/graphviz/graphviz/-/issues/2272
    """

    # FIXME: Remove skip when
    # https://gitlab.com/graphviz/graphviz/-/issues/1777 is fixed
    if os.getenv("build_system") == "msbuild":
        pytest.skip("Windows MSBuild release does not contain any header files (#1777)")

    # find co-located test source
    c_src = (Path(__file__).parent / "2272.c").resolve()
    assert c_src.exists(), "missing test case"

    # run the test
    run_c(c_src, link=["cgraph", "gvc"])


def test_2272_2():
    """
    An unterminated string in the source should not crash Graphviz. Variant of
    `test_2272`.
    """

    # a graph with an open string
    graph = 'graph { a[label="abc'

    # process it with Graphviz, which should not crash
    p = subprocess.run(["dot", "-o", os.devnull], input=graph, universal_newlines=True)
    assert p.returncode != 0, "dot accepted invalid input"
    assert p.returncode == 1, "dot crashed"


def test_2282():
    """
    using the `fdp` layout with JSON output should result in valid JSON
    https://gitlab.com/graphviz/graphviz/-/issues/2282
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "2282.dot"
    assert input.exists(), "unexpectedly missing test case"

    # translate this to JSON
    output = dot("json", input)

    # confirm this is valid JSON
    json.loads(output)


def test_2283():
    """
    `beautify=true` should correctly space nodes
    https://gitlab.com/graphviz/graphviz/-/issues/2283
    """

    # find our collocated test case
    input = Path(__file__).parent / "2283.dot"
    assert input.exists(), "unexpectedly missing test case"

    # translate this to SVG
    p = subprocess.run(
        ["dot", "-Tsvg", input],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True,
    )

    # if sfdp was built without libgts, it will not handle anything non-trivial
    no_gts_error = "remove_overlap: Graphviz not built with triangulation library"
    if no_gts_error in p.stderr:
        assert p.returncode != 0, "sfdp returned success after an error message"
        return
    p.check_returncode()

    svg = p.stdout

    # parse this into something we can inspect
    root = ET.fromstring(svg)

    # find node N0
    n0s = root.findall(
        ".//{http://www.w3.org/2000/svg}title[.='N0']/../{http://www.w3.org/2000/svg}ellipse"
    )
    assert len(n0s) == 1, "failed to locate node N0"
    n0 = n0s[0]

    # find node N1
    n1s = root.findall(
        ".//{http://www.w3.org/2000/svg}title[.='N1']/../{http://www.w3.org/2000/svg}ellipse"
    )
    assert len(n1s) == 1, "failed to locate node N1"
    n1 = n1s[0]

    # find node N6
    n6s = root.findall(
        ".//{http://www.w3.org/2000/svg}title[.='N6']/../{http://www.w3.org/2000/svg}ellipse"
    )
    assert len(n6s) == 1, "failed to locate node N6"
    n6 = n6s[0]

    # N1 and N6 should not have been drawn on top of each other
    n1_x = float(n1.attrib["cx"])
    n1_y = float(n1.attrib["cy"])
    n6_x = float(n6.attrib["cx"])
    n6_y = float(n6.attrib["cy"])

    def sameish(a: float, b: float) -> bool:
        EPSILON = 0.2
        return -EPSILON < abs(a - b) < EPSILON

    assert not (
        sameish(n1_x, n6_x) and sameish(n1_y, n6_y)
    ), "N1 and N6 placed identically"

    # use the Law of Cosines to compute the angle between N0→N1 and N0→N6
    n0_x = float(n0.attrib["cx"])
    n0_y = float(n0.attrib["cy"])
    n0_n1_dist = math.dist((n0_x, n0_y), (n1_x, n1_y))
    n0_n6_dist = math.dist((n0_x, n0_y), (n6_x, n6_y))
    n1_n6_dist = math.dist((n1_x, n1_y), (n6_x, n6_y))
    angle = math.acos(
        (n0_n1_dist**2 + n0_n6_dist**2 - n1_n6_dist**2)
        / (2 * n0_n1_dist * n0_n6_dist)
    )

    number_of_radial_nodes = 6
    assert sameish(
        angle, 2 * math.pi / number_of_radial_nodes
    ), "nodes not placed evenly"


@pytest.mark.skipif(which("gxl2gv") is None, reason="gxl2gv not available")
@pytest.mark.xfail()
def test_2300_1():
    """
    translating GXL with an attribute `name` should not crash
    https://gitlab.com/graphviz/graphviz/-/issues/2300
    """

    # locate our associated test case containing a node attribute `name`
    input = Path(__file__).parent / "2300.gxl"
    assert input.exists(), "unexpectedly missing test case"

    # ask `gxl2gv` to process this
    subprocess.check_call(["gxl2gv", input])


def test_2307():
    """
    'id' attribute should be propagated to 'url' links in SVG output
    https://gitlab.com/graphviz/graphviz/-/issues/2307
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "2258.dot"
    assert input.exists(), "unexpectedly missing test case"

    # translate this to SVG
    svg = dot("svg", input)

    # load this as XML
    root = ET.fromstring(svg)

    # the output is expected to contain a number of polygons, any of which have
    # `url` fills should include the ID “G2”
    polygons = root.findall(".//{http://www.w3.org/2000/svg}polygon")
    assert len(polygons) > 0, "no polygons in output"

    for polygon in polygons:
        m = re.match(r"url\((?P<url>.*)\)$", polygon.get("fill"))
        if m is None:
            continue
        assert (
            re.search(r"\bG2_", m.group("url")) is not None
        ), "ID G2 was not applied to polygon fill url"


def test_2325():
    """
    using more than 63 styles and/or more than 128 style bytes should not trigger
    an out-of-bounds memory read
    https://gitlab.com/graphviz/graphviz/-/issues/2325
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "2325.dot"
    assert input.exists(), "unexpectedly missing test case"

    # run it through Graphviz
    dot("svg", input)


@pytest.mark.skipif(shutil.which("groff") is None, reason="groff not available")
def test_2341():
    """
    PIC backend should generate correct comments
    https://gitlab.com/graphviz/graphviz/-/issues/2341
    """

    # a simple graph
    source = "digraph { a -> b; }"

    # generate PIC from this
    pic = dot("pic", source=source)

    # run this through groff
    groffed = subprocess.check_output(
        ["groff", "-Tascii", "-p"], input=pic, universal_newlines=True
    )

    # it should not contain any comments
    assert (
        re.search(r"^\s*#", groffed) is None
    ), "Graphviz comment remains in groff output"


def test_2352():
    """
    referencing an all-one-line external SVG file should work
    https://gitlab.com/graphviz/graphviz/-/issues/2352
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "2352.dot"
    assert input.exists(), "unexpectedly missing test case"

    # translate it to SVG
    svg = subprocess.check_output(
        ["dot", "-Tsvg", input], cwd=Path(__file__).parent, universal_newlines=True
    )

    assert '<image xlink:href="EDA.svg" ' in svg, "external file reference missing"


def test_2352_1():
    """
    variant of 2352 with a leading space in front of `<svg`
    https://gitlab.com/graphviz/graphviz/-/issues/2352
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "2352_1.dot"
    assert input.exists(), "unexpectedly missing test case"

    # translate it to SVG
    svg = subprocess.check_output(
        ["dot", "-Tsvg", input], cwd=Path(__file__).parent, universal_newlines=True
    )

    assert '<image xlink:href="EDA_1.svg" ' in svg, "external file reference missing"


def test_2352_2():
    """
    variant of 2352 that spaces viewBox such that it is on a 200-character line
    boundary
    https://gitlab.com/graphviz/graphviz/-/issues/2352
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "2352_2.dot"
    assert input.exists(), "unexpectedly missing test case"

    # translate it to SVG
    svg = subprocess.check_output(
        ["dot", "-Tsvg", input], cwd=Path(__file__).parent, universal_newlines=True
    )

    assert '<image xlink:href="EDA_2.svg" ' in svg, "external file reference missing"


def test_2355():
    """
    Using >127 layers should not crash Graphviz
    https://gitlab.com/graphviz/graphviz/-/issues/2355
    """

    # construct a graph with 128 layers
    graph = io.StringIO()
    graph.write("digraph {\n")
    layers = ":".join(f"l{i}" for i in range(128))
    graph.write(f'  layers="{layers}";\n')
    for i in range(128):
        graph.write(f'  n{i}[layer="l{i}"];\n')
    graph.write("}\n")

    # process this with dot
    dot("svg", source=graph.getvalue())


@pytest.mark.xfail(strict=True)  # FIXME
def test_2368():
    """
    routesplines should not corrupt its `prev` and `next` indices
    https://gitlab.com/graphviz/graphviz/-/issues/2368
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "2368.dot"
    assert input.exists(), "unexpectedly missing test case"

    # run it through Graphviz
    dot("svg", input)


@pytest.mark.skipif(shutil.which("tclsh") is None, reason="tclsh not available")
@pytest.mark.xfail()  # FIXME
def test_2370():
    """
    tcldot should have a version number TCL accepts
    https://gitlab.com/graphviz/graphviz/-/issues/2370
    """

    # ask TCL to import the Graphviz package
    response = subprocess.check_output(
        ["tclsh"],
        stderr=subprocess.STDOUT,
        input="package require tcldot;",
        universal_newlines=True,
    )

    assert (
        "error reading package index file" not in response
    ), "tcldot cannot be loaded by TCL"


def test_2371():
    """
    Large graphs should not cause rectangle area calculation overflows
    https://gitlab.com/graphviz/graphviz/-/issues/2371
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "2371.dot"
    assert input.exists(), "unexpectedly missing test case"

    # run it through Graphviz
    subprocess.check_call(["dot", "-Tsvg", "-Knop2", "-o", os.devnull, input])


@pytest.mark.skipif(
    platform.system() == "Windows",
    reason="gvplugin_list symbol is not exposed on Windows",
)
def test_2375():
    """
    `gvplugin_list` should return full plugin names
    https://gitlab.com/graphviz/graphviz/-/issues/2375
    """

    # find co-located test source
    c_src = (Path(__file__).parent / "2375.c").resolve()
    assert c_src.exists(), "missing test case"

    # run the test
    run_c(c_src, link=["gvc"])


def test_2377():
    """
    3 letter hex color codes should be accepted
    https://gitlab.com/graphviz/graphviz/-/issues/2377
    """

    # run some 6 letter color input through Graphviz
    input = 'digraph { n [color="#cc0000" fillcolor="#ffcc00" style=filled] }'
    svg1 = dot("svg", source=input)

    # try the equivalent with 3 letter colors
    input = 'digraph { n [color="#c00" fillcolor="#fc0" style=filled] }'
    svg2 = dot("svg", source=input)

    assert svg1 == svg2, "3 letter hex colors were not translated correctly"


def test_2390():
    """
    using an out of range `xdotversion` should not crash Graphviz
    https://gitlab.com/graphviz/graphviz/-/issues/2390
    """

    # some input with an invalid large `xdotversion`
    input = 'graph { xdotversion=99; n[label="hello world"]; }'

    # run this through Graphviz
    dot("xdot", source=input)


@pytest.mark.xfail(strict=True)  # FIXME
def test_2391():
    """
    `nslimit1=0` should not cause Graphviz to crash
    https://gitlab.com/graphviz/graphviz/-/issues/2391
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "2391.dot"
    assert input.exists(), "unexpectedly missing test case"

    # run it through Graphviz
    dot("svg", input)


@pytest.mark.xfail(strict=True)  # FIXME
def test_2391_1():
    """
    `nslimit1=0` with a label should not cause Graphviz to crash
    https://gitlab.com/graphviz/graphviz/-/issues/2391
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "2391_1.dot"
    assert input.exists(), "unexpectedly missing test case"

    # run it through Graphviz
    dot("svg", input)


@pytest.mark.xfail(
    platform.system() == "Windows" and not is_mingw(),
    reason="cannot link Agdirected on Windows",
    strict=True,
)  # FIXME
def test_2397():
    """
    escapes in strings should be handled correctly
    https://gitlab.com/graphviz/graphviz/-/issues/2397
    """

    # find co-located test source
    c_src = (Path(__file__).parent / "2397.c").resolve()
    assert c_src.exists(), "missing test case"

    # run this to generate a graph
    source, _ = run_c(c_src, link=["cgraph", "gvc"])

    # this should have produced a valid graph
    dot("svg", source=source)


def test_2397_1():
    """
    a variant of test_2397 that confirms the same works via the command line
    https://gitlab.com/graphviz/graphviz/-/issues/2397
    """

    source = 'digraph { a[label="foo\\\\\\"bar"]; }'

    # run this through dot
    output = dot("dot", source=source)

    # the output should be valid dot
    dot("svg", source=output)


@pytest.mark.skipif(shutil.which("shellcheck") is None, reason="shellcheck unavailable")
def test_2404():
    """
    shell syntax used by gvmap should be correct
    https://gitlab.com/graphviz/graphviz/-/issues/2404
    """
    gvmap_sh = Path(__file__).parent / "../cmd/gvmap/gvmap.sh"
    subprocess.check_call(["shellcheck", "-S", "error", gvmap_sh])


def test_2406():
    """
    arrow types like `invdot` and `onormalonormal` should be displayed correctly
    https://gitlab.com/graphviz/graphviz/-/issues/2406
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "2406.dot"
    assert input.exists(), "unexpectedly missing test case"

    # run it through Graphviz
    output = dot("svg", input)

    # the rounded hollows should be present
    assert re.search(r"\bellipse\b", output), "missing element of invdot arrow"


@pytest.mark.parametrize("source", ("2413_1.dot", "2413_2.dot"))
def test_2413(source: str):
    """
    graphs that induce an edge length > 65535 should be supported
    https://gitlab.com/graphviz/graphviz/-/issues/2413
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / source
    assert input.exists(), "unexpectedly missing test case"

    # run it through Graphviz
    proc = subprocess.run(
        ["dot", "-Tsvg", "-o", os.devnull, input],
        stderr=subprocess.PIPE,
        check=True,
        universal_newlines=True,
    )

    # no warnings should have been generated
    assert proc.stderr == "", "long edges resulted in a warning"


@pytest.mark.skipif(
    platform.system() == "Windows", reason="vt target is not supported on Windows"
)
def test_2429():
    """
    the vt target should be usable
    https://gitlab.com/graphviz/graphviz/-/issues/2429
    """

    # a basic graph
    source = "digraph { a -> b; }"

    # run it through Graphviz
    dot("vt", source=source)


@pytest.mark.skipif(which("nop") is None, reason="nop not available")
@pytest.mark.xfail(
    strict=True, reason="https://gitlab.com/graphviz/graphviz/-/issues/2436"
)
def test_2436():
    """
    nop should preserve empty labels
    https://gitlab.com/graphviz/graphviz/-/issues/2436
    """

    # locate our associated test case in this directory
    input = Path(__file__).parent / "2436.dot"
    assert input.exists(), "unexpectedly missing test case"

    # run it through nop
    nop = which("nop")
    output = subprocess.check_output([nop, input], universal_newlines=True)

    # the empty label should be present
    assert re.search(r'\blabel\s*=\s*""', output), "empty label was not preserved"


@pytest.mark.xfail(
    strict=True, reason="https://gitlab.com/graphviz/graphviz/-/issues/2434"
)
def test_2434():
    """
    the order in which `agmemread` and `gvContext` calls are made should have
    no impact
    https://gitlab.com/graphviz/graphviz/-/issues/2434
    """

    # find co-located test source
    c_src = (Path(__file__).parent / "2434.c").resolve()
    assert c_src.exists(), "missing test case"

    # generate an SVG by calling `gvContext` first
    before, _ = run_c(c_src, ["before"], link=["cgraph", "gvc"])

    # generate an SVG by calling `gvContext` second
    after, _ = run_c(c_src, ["after"], link=["cgraph", "gvc"])

    # resulting images should be identical
    assert before == after, "agmemread/gvContext ordering affected image output"


@pytest.mark.xfail(
    strict=True, reason="https://gitlab.com/graphviz/graphviz/-/issues/2416"
)
def test_2416():
    """
    `splines=curved` should not affect arrow directions
    https://gitlab.com/graphviz/graphviz/-/issues/2416
    """

    # an input graph that provokes the problem
    input = "digraph G { splines=curved; b -> a; a -> b; }"

    # run it through Graphviz
    output = dot("json", source=input)
    data = json.loads(output)

    edges = data["edges"]
    assert len(edges) == 2, "unexpected number of output edges"

    # extract the height each edge’s arrow starts at
    y_1 = edges[0]["_hdraw_"][3]["points"][0][1]
    y_2 = edges[1]["_hdraw_"][3]["points"][0][1]

    # assuming the graph is vertical, these should not be too close
    assert abs(y_1 - y_2) > 1, "edge arrows appear to be drawn next to the same node"


def test_changelog_dates():
    """
    Check the dates of releases in the changelog are correctly formatted
    """
    changelog = Path(__file__).parent / "../CHANGELOG.md"
    with open(changelog, "rt", encoding="utf-8") as f:
        for lineno, line in enumerate(f, 1):
            m = re.match(r"## \[\d+\.\d+\.\d+\] [-–] (?P<date>.*)$", line)
            if m is None:
                continue
            d = re.match(r"\d{4}-\d{2}-\d{2}", m.group("date"))
            assert (
                d is not None
            ), f"CHANGELOG.md:{lineno}: date in incorrect format: {line}"


@pytest.mark.skipif(which("gvpack") is None, reason="gvpack not available")
def test_duplicate_hard_coded_metrics_warnings():
    """
    Check “no hard-coded metrics” warnings are not repeated
    """

    # use the #2239 test case that happens to provoke this
    input = Path(__file__).parent / "2239.dot"
    assert input.exists(), "unexpectedly missing test case"

    # run it through gvpack
    gvpack = which("gvpack")
    p = subprocess.run(
        [gvpack, "-u", "-o", os.devnull, input],
        stderr=subprocess.PIPE,
        universal_newlines=True,
    )

    assert (
        p.stderr.count("no hard-coded metrics for 'sans'") <= 1
    ), "multiple identical “no hard-coded metrics” warnings printed"


@pytest.mark.parametrize("branch", (0, 1, 2, 3))
@pytest.mark.skipif(which("gvpr") is None, reason="gvpr not available")
def test_gvpr_switches(branch: int):
    """
    confirm the behavior of GVPR switch statements
    """

    # an input GVPR program with multiple blocks and switches
    program = textwrap.dedent(
        f"""\
    BEGIN {{
      switch ({branch}) {{
        case 0:
          printf("begin 0\\n");
          break;
        case 1:
          printf("begin 1\\n");
          break;
        case 2:
          printf("begin 2\\n");
          break;
        default:
          printf("begin 3\\n");
          break;
      }}
    }}

    END {{
      switch ({branch}) {{
        case 0:
          printf("end 0\\n");
          break;
        case 1:
          printf("end 1\\n");
          break;
        case 2:
          printf("end 2\\n");
          break;
        default:
          printf("end 3\\n");
          break;
      }}
    }}
    """
    )

    # run this through GVPR with no input graph
    gvpr_bin = which("gvpr")
    result = subprocess.check_output(
        [gvpr_bin, program], stdin=subprocess.DEVNULL, universal_newlines=True
    )

    # confirm we got the expected output
    assert result == f"begin {branch}\nend {branch}\n", "incorrect GVPR switch behavior"


@pytest.mark.parametrize(
    "statement,expected",
    (
        ('printf("%d", 5)', "5"),
        ('printf("%d", 0)', "0"),
        ('printf("%.0d", 0)', ""),
        ('printf("%.0d", 1)', "1"),
        ('printf("%.d", 2)', "2"),
        ('printf("%d", -1)', "-1"),
        ('printf("%.3d", 5)', "005"),
        ('printf("%.3d", -5)', "-005"),
        ('printf("%5.3d", 5)', "  005"),
        ('printf("%-5.3d", -5)', "-005 "),
        ('printf("%-d", 5)', "5"),
        ('printf("%-+d", 5)', "+5"),
        ('printf("%+-d", 5)', "+5"),
        ('printf("%+d", -5)', "-5"),
        ('printf("% d", 5)', " 5"),
        ('printf("% .0d", 0)', " "),
        ('printf("%03d", 5)', "005"),
        ('printf("%03d", -5)', "-05"),
        ('printf("% +d", 5)', "+5"),
        ('printf("%-03d", -5)', "-5 "),
        ('printf("%o", 5)', "5"),
        ('printf("%o", 8)', "10"),
        ('printf("%o", 0)', "0"),
        ('printf("%.0o", 0)', ""),
        ('printf("%.0o", 1)', "1"),
        ('printf("%.3o", 5)', "005"),
        ('printf("%.3o", 8)', "010"),
        ('printf("%5.3o", 5)', "  005"),
        ('printf("%u", 5)', "5"),
        ('printf("%u", 0)', "0"),
        ('printf("%.0u", 0)', ""),
        ('printf("%.0u", 1)', "1"),
        ('printf("%.3u", 5)', "005"),
        ('printf("%5.3u", 5)', "  005"),
        ('printf("%u", 5)', "5"),
        ('printf("%u", 0)', "0"),
        ('printf("%.0u", 0)', ""),
        ('printf("%.0u", 1)', "1"),
        ('printf("%.3u", 5)', "005"),
        ('printf("%5.3u", 5)', "  005"),
        ('printf("%-x", 5)', "5"),
        ('printf("%03x", 5)', "005"),
        ('printf("%-x", 5)', "5"),
        ('printf("%03x", 5)', "005"),
        ('printf("%-X", 5)', "5"),
        ('printf("%03X", 5)', "005"),
        ('printf("%.2s", "abc")', "ab"),
        ('printf("%.6s", "abc")', "abc"),
        ('printf("%5s", "abc")', "  abc"),
        ('printf("%-5s", "abc")', "abc  "),
        ('printf("%5.2s", "abc")', "   ab"),
        ('printf("%%")', "%"),
    ),
)
@pytest.mark.skipif(which("gvpr") is None, reason="gvpr not available")
def test_gvpr_printf(statement: str, expected: str):
    """
    check various behaviors of `printf` in a GVPR program
    """

    # a program that performs the given `printf`
    program = f"BEGIN {{ {statement}; }}"

    # run this through GVPR with no input graph
    gvpr_bin = which("gvpr")
    result = subprocess.check_output(
        [gvpr_bin, program], stdin=subprocess.DEVNULL, universal_newlines=True
    )

    # confirm we got the expected output
    assert result == expected, "incorrect GVPR printf behavior"
