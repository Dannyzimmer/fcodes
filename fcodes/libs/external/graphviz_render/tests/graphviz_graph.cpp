#include <stdexcept>

#include <fmt/format.h>

#include "graphviz_graph.h"

GraphvizGraph::GraphvizGraph(SVG::SVGElement &svg_g_element)
    : m_svg_g_element(svg_g_element) {}

void GraphvizGraph::add_node(SVG::SVGElement &svg_g_element) {
  m_nodes.emplace_back(svg_g_element);
}

const std::vector<GraphvizNode> &GraphvizGraph::nodes() const {
  return m_nodes;
}

const SVG::SVGElement &GraphvizGraph::svg_g_element() const {
  return m_svg_g_element;
}

const GraphvizNode &GraphvizGraph::node(std::string_view node_id) const {
  for (auto &node : m_nodes) {
    if (node.node_id() == node_id) {
      return node;
    }
  }
  throw std::runtime_error{
      fmt::format("Unknown node '{}' in graph '{}'", node_id, m_graph_id)};
}

void GraphvizGraph::add_edge(SVG::SVGElement &svg_g_element) {
  m_edges.emplace_back(svg_g_element);
}

const std::vector<GraphvizEdge> &GraphvizGraph::edges() const {
  return m_edges;
}

const GraphvizEdge &GraphvizGraph::edge(std::string_view edgeop) const {
  for (auto &edge : m_edges) {
    if (edge.edgeop() == edgeop) {
      return edge;
    }
  }
  throw std::runtime_error{
      fmt::format("Unknown edge '{}' in graph '{}'", edgeop, m_graph_id)};
}

void GraphvizGraph::add_bboxes() {
  for (auto &node : m_nodes) {
    node.add_bbox();
  }
  for (auto &edge : m_edges) {
    edge.add_bbox();
  }
}

void GraphvizGraph::add_node_edge_outline_bbox_overlaps(
    const double tolerance) {
  for (auto &node : m_nodes) {
    for (auto &edge : m_edges) {
      edge.add_outline_overlap_bbox(node, tolerance);
    }
  }
}

void GraphvizGraph::add_outline_bboxes() {
  for (auto &node : m_nodes) {
    node.add_outline_bbox();
  }
  for (auto &edge : m_edges) {
    edge.add_outline_bbox();
  }
}
