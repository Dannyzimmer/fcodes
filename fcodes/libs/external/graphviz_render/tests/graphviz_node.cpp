#include <string>

#include "graphviz_node.h"
#include "svg_element.h"

GraphvizNode::GraphvizNode(SVG::SVGElement &svg_element)
    : m_node_id(svg_element.graphviz_id), m_svg_g_element(svg_element) {}

void GraphvizNode::add_bbox() { m_svg_g_element.add_bbox(); }

void GraphvizNode::add_outline_bbox() { m_svg_g_element.add_outline_bbox(); }

SVG::SVGPoint GraphvizNode::center() const { return bbox().center(); }

std::string GraphvizNode::color() const {
  const auto stroke = m_svg_g_element.attribute_from_subtree<std::string>(
      &SVG::SVGAttributes::stroke, &SVG::SVGElement::is_shape_element, "");
  const auto stroke_opacity = m_svg_g_element.attribute_from_subtree<double>(
      &SVG::SVGAttributes::stroke_opacity, &SVG::SVGElement::is_shape_element,
      1);
  return SVG::to_dot_color(stroke, stroke_opacity);
}

std::string GraphvizNode::fillcolor() const {
  const auto fill = m_svg_g_element.attribute_from_subtree<std::string>(
      &SVG::SVGAttributes::fill, &SVG::SVGElement::is_shape_element, "");
  const auto fill_opacity = m_svg_g_element.attribute_from_subtree<double>(
      &SVG::SVGAttributes::fill_opacity, &SVG::SVGElement::is_shape_element, 1);
  return SVG::to_dot_color(fill, fill_opacity);
}

double GraphvizNode::penwidth() const {
  return m_svg_g_element.attribute_from_subtree<double>(
      &SVG::SVGAttributes::stroke_width, &SVG::SVGElement::is_shape_element, 1);
}

std::string_view GraphvizNode::node_id() const { return m_node_id; }

SVG::SVGRect GraphvizNode::bbox() const { return m_svg_g_element.bbox(); }

SVG::SVGRect GraphvizNode::outline_bbox() const {
  return m_svg_g_element.outline_bbox();
}

const SVG::SVGElement &GraphvizNode::svg_g_element() const {
  return m_svg_g_element;
}
