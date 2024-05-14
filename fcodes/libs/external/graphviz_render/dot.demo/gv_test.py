#!/usr/bin/env python3

"""
example usage of Graphviz Python module
"""

import gv  # pylint: disable=import-error

g = gv.digraph("G")
n = gv.node(g, "hello")
m = gv.node(g, "world")
e = gv.edge(n, m)
gv.layout(g, "dot")
gv.render(g, "png", "gv_test.png")
gv.rm(g)
