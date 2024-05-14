#!/usr/bin/env python3

"""
example usage of the Graphviz Python module
"""

import gv  # pylint: disable=import-error


def main():  # pylint: disable=missing-function-docstring
    # create a new empty graph
    G = gv.digraph("G")
    # define a simple graph ( A->B )
    gv.edge(gv.node(G, "A"), gv.node(G, "B"))
    # compute a directed graph layout
    gv.layout(G, "dot")
    # annotate the graph with the layout information
    gv.render(G)
    # do something with the layout
    n = gv.firstnode(G)
    while n:
        print(f"node {gv.nameof(n)} is at {gv.getv(n, 'pos')}")
        e = gv.firstout(n)
        while e:
            print(
                f"edge {gv.nameof(gv.tailof(e))}->{gv.nameof(gv.headof(e))} "
                f"is at {gv.getv(e, 'pos')}"
            )
            e = gv.nextout(n, e)
        n = gv.nextnode(G, n)


if __name__ == "__main__":
    main()
