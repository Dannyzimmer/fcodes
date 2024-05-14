#!/usr/bin/env python3

"""
example usage of the Graphviz Python module
"""

import gv  # pylint: disable=import-error

# create a new empty graph
G = gv.digraph("G")

# define a simple graph ( A->B )
gv.edge(gv.node(G, "A"), gv.node(G, "B"))

# compute a directed graph layout
gv.layout(G, "dot")

# annotate the graph with the layout information
gv.render(G, "ps")
