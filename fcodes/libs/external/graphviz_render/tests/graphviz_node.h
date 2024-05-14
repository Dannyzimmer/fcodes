#pragma once

#include <string>
#include <string_view>

#include "svg_element.h"

/**
 * @brief The GraphvizNode class represents a Graphviz node according to the DOT
 * language
 */

class GraphvizNode {
public:
  GraphvizNode() = delete;
  explicit GraphvizNode(SVG::SVGElement &svg_element);

  /// Add an SVG `rect` element representing the bounding box of the node to the
  /// corresponding `g` element
  void add_bbox();
  /// Add an SVG `rect` element representing the outline bounding box of the
  /// node to the corresponding `g` element. The outline bounding box is the
  /// bounding box with penwidth taken into account.
  void add_outline_bbox();
  /// Return the node's bounding box
  SVG::SVGRect bbox() const;
  /// Return the center of the node's bounding box
  SVG::SVGPoint center() const;
  /// Return the node's `color` attribute in RGB hex format if the opacity is
  /// 100 % or in RGBA hex format otherwise.
  std::string color() const;
  /// Return the node's `fillcolor` attribute in RGB hex format if the opacity
  /// is 100 % or in RGBA hex format otherwise.
  std::string fillcolor() const;
  /// Return the node's `node_id` as defined by the DOT language
  std::string_view node_id() const;
  /// Return the outline bounding box of the node. The outline bounding box is
  /// the bounding box with penwidth taken into account.
  SVG::SVGRect outline_bbox() const;
  /// Return the node's `penwidth` attribute
  double penwidth() const;
  /// Return a non-mutable reference to the SVG `g` element corresponding to the
  /// node
  const SVG::SVGElement &svg_g_element() const;

private:
  /// The `node_id` according to the DOT language specification. Note that this
  /// is not the same as the `id` attribute of a node
  std::string m_node_id;
  /// The SVG `g` element corresponding to the node
  SVG::SVGElement &m_svg_g_element;
};
