#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "graphviz_edge.h"
#include "graphviz_node.h"

/**
 * @brief The GraphvizGraph class represents a Graphviz graph according to the
 * DOT language
 */

class GraphvizGraph {
public:
  GraphvizGraph() = delete;
  explicit GraphvizGraph(SVG::SVGElement &g_element);

  /// Add SVG `rect` elements representing the bounding boxes of nodes and edges
  /// to the corresponding `g` elements
  void add_bboxes();
  /// Add a Graphviz edge to the graph
  void add_edge(SVG::SVGElement &svg_g_element);
  /// Add a Graphviz node to the graph
  void add_node(SVG::SVGElement &svg_g_element);
  /// Add SVG `rect` elements representing the overlap between the outline
  /// bounding box of nodes and edges. The outline bounding box is the bounding
  /// box with penwidth taken into account.
  void add_node_edge_outline_bbox_overlaps(double tolerance = 0);
  /// Add SVG `rect` elements representing the outline bounding boxes of nodes
  /// and edges to the corresponding `g` elements. The outline bounding box is
  /// the bounding box with penwidth taken into account.
  void add_outline_bboxes();
  const GraphvizEdge &edge(std::string_view edgeop) const;
  /// Return a non-mutable reference to the list of Graphviz edges
  const std::vector<GraphvizEdge> &edges() const;
  /// Return the node with the specified `node_id`
  const GraphvizNode &node(std::string_view node_id) const;
  /// Return a non-mutable reference to the list of Graphviz nodes
  const std::vector<GraphvizNode> &nodes() const;
  /// Return a non-mutable reference to the SVG `g` element corresponding to the
  /// graph
  const SVG::SVGElement &svg_g_element() const;

private:
  /// A list of edges belonging to this graph
  std::vector<GraphvizEdge> m_edges;
  /// The `ID` according to the DOT language specification. Note that this is
  /// not the same as the `id` attribute of a graph
  std::string m_graph_id;
  /// A list of nodes belonging to this graph
  std::vector<GraphvizNode> m_nodes;
  /// The SVG `g` element corresponding to the graph
  SVG::SVGElement &m_svg_g_element;
};
