#include <cassert>
#include <string>
#include <unordered_set>

#include "graphviz_edge.h"
#include "graphviz_node.h"
#include "svg_element.h"

GraphvizEdge::GraphvizEdge(SVG::SVGElement &svg_g_element)
    : m_edgeop(svg_g_element.graphviz_id), m_svg_g_element(svg_g_element) {}

void GraphvizEdge::add_bbox() { m_svg_g_element.add_bbox(); }

void GraphvizEdge::add_outline_bbox() { m_svg_g_element.add_outline_bbox(); }

void GraphvizEdge::add_outline_overlap_bbox(const GraphvizNode &node,
                                            const double tolerance) {
  m_svg_g_element.add_outline_overlap_bbox(node.svg_g_element(), tolerance);
}

static const std::unordered_set<std::string_view>
    path_based_primitive_arrow_shapes = {
        "curve",  //
        "icurve", //
};

static const std::unordered_set<std::string_view>
    polygon_based_primitive_arrow_shapes = {
        "box",     //
        "crow",    //
        "diamond", //
        "inv",     //
        "normal",  //
        "tee",     //
        "vee",     //
};

static const std::unordered_set<std::string_view>
    ellipse_based_primitive_arrow_shapes = {
        "dot", //
};

static const std::unordered_set<std::string_view>
    primitive_arrow_shapes_containing_polyline = {
        "box",    //
        "curve",  //
        "icurve", //
        "tee",    //
};

static SVG::SVGElementType
edge_arrowhead_main_svg_element_type(std::string_view primitive_arrow_shape) {
  if (polygon_based_primitive_arrow_shapes.contains(primitive_arrow_shape)) {
    return SVG::SVGElementType::Polygon;
  } else if (ellipse_based_primitive_arrow_shapes.contains(
                 primitive_arrow_shape)) {
    return SVG::SVGElementType::Ellipse;
  } else if (path_based_primitive_arrow_shapes.contains(
                 primitive_arrow_shape)) {
    return SVG::SVGElementType::Path;
  } else {
    assert(false && "unsupported primitive arrow shape");
  }
}

SVG::SVGRect GraphvizEdge::arrowhead_outline_bbox(
    std::string_view dir, std::string_view primitive_arrow_shape) const {
  assert((dir == "forward" || dir == "both") &&
         "no arrowhead for this edge direction");

  const auto index = dir == "forward" ? 0 : 1;

  const auto main_svg_element_type =
      edge_arrowhead_main_svg_element_type(primitive_arrow_shape);
  const auto main_index =
      index + (main_svg_element_type == SVG::SVGElementType::Path ? 1 : 0);
  auto edge_arrowhead =
      m_svg_g_element.find_child(main_svg_element_type, main_index);
  auto edge_arrowhead_bbox = edge_arrowhead.outline_bbox();
  if (primitive_arrow_shapes_containing_polyline.contains(
          primitive_arrow_shape)) {
    auto edge_arrowhead_stem =
        m_svg_g_element.find_child(SVG::SVGElementType::Polyline, index);
    auto edge_arrowhead_stem_bbox = edge_arrowhead_stem.outline_bbox();
    edge_arrowhead_bbox.extend(edge_arrowhead_stem_bbox);
  }

  return edge_arrowhead_bbox;
}

SVG::SVGRect GraphvizEdge::arrowtail_outline_bbox(
    std::string_view dir, std::string_view primitive_arrow_shape) const {
  assert((dir == "back" || dir == "both") &&
         "no arrowhead for this edge direction");
  const auto index = 0;
  const auto main_svg_element_type =
      edge_arrowhead_main_svg_element_type(primitive_arrow_shape);
  const auto main_index =
      index + (main_svg_element_type == SVG::SVGElementType::Path ? 1 : 0);
  auto edge_arrowtail =
      m_svg_g_element.find_child(main_svg_element_type, main_index);
  auto edge_arrowtail_bbox = edge_arrowtail.outline_bbox();
  if (primitive_arrow_shapes_containing_polyline.contains(
          primitive_arrow_shape)) {
    auto edge_arrowtail_stem =
        m_svg_g_element.find_child(SVG::SVGElementType::Polyline, index);
    auto edge_arrowtail_stem_bbox = edge_arrowtail_stem.outline_bbox();
    edge_arrowtail_bbox.extend(edge_arrowtail_stem_bbox);
  }

  return edge_arrowtail_bbox;
}

std::string_view GraphvizEdge::edgeop() const { return m_edgeop; }

std::string GraphvizEdge::fillcolor() const {
  const auto fill = m_svg_g_element.attribute_from_subtree<std::string>(
      &SVG::SVGAttributes::fill, &SVG::SVGElement::is_closed_shape_element, "");
  const auto fill_opacity = m_svg_g_element.attribute_from_subtree<double>(
      &SVG::SVGAttributes::fill_opacity,
      &SVG::SVGElement::is_closed_shape_element, 1);
  if (fill.empty() && fill_opacity == 1) {
    return "";
  }
  return SVG::to_dot_color(fill, fill_opacity);
}

SVG::SVGElement &GraphvizEdge::stem() const {
  return m_svg_g_element.find_child(SVG::SVGElementType::Path);
}

const SVG::SVGElement &GraphvizEdge::svg_g_element() const {
  return m_svg_g_element;
}

SVG::SVGRect GraphvizEdge::bbox() const { return m_svg_g_element.bbox(); }

SVG::SVGPoint GraphvizEdge::center() const { return bbox().center(); }

std::string GraphvizEdge::color() const {
  const auto stroke = m_svg_g_element.attribute_from_subtree<std::string>(
      &SVG::SVGAttributes::stroke, &SVG::SVGElement::is_shape_element, "");
  const auto stroke_opacity = m_svg_g_element.attribute_from_subtree<double>(
      &SVG::SVGAttributes::stroke_opacity, &SVG::SVGElement::is_shape_element,
      1);
  return SVG::to_dot_color(stroke, stroke_opacity);
}

double GraphvizEdge::penwidth() const {
  return m_svg_g_element.attribute_from_subtree<double>(
      &SVG::SVGAttributes::stroke_width, &SVG::SVGElement::is_shape_element, 1);
}

SVG::SVGRect GraphvizEdge::outline_bbox() const {
  return m_svg_g_element.outline_bbox();
}
