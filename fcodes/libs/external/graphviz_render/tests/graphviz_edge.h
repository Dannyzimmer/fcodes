#pragma once

#include <string>
#include <string_view>

#include "graphviz_node.h"
#include "svg_element.h"

/**
 * @brief The GraphvizEdge class represents a Graphviz edge according to the DOT
 * language
 */

class GraphvizEdge {
public:
  GraphvizEdge() = delete;
  explicit GraphvizEdge(SVG::SVGElement &svg_g_element);

  /// Add an SVG `rect` element to the edge's corresponding `g` element. The
  /// `rect` is represes the bounding box of the edge.
  void add_bbox();
  /// Add an SVG `rect` element to the edge's corresponding `g` element. The
  /// `rect` represents the outline bounding box of the edge. The outline
  /// bounding box is the bounding box with penwidth taken into account.
  void add_outline_bbox();
  /// Add an SVG `rect` element representing the overlap between the outline
  /// bounding box of the edge and the specified node, to the corresponding `g`
  /// element. The outline bounding box is the bounding box with penwidth taken
  /// into account.
  void add_outline_overlap_bbox(const GraphvizNode &node, double tolerance = 0);
  /// Return the edge arrowhead/arrowtail with the specified index. If there's
  /// both an arrowhead and an arrowtail, the arrowtail is at index 0 and the
  /// arrowhead is at index 1. If there's only one, it's at index 0. Throws an
  /// exception if there's no arrow at the specified index.
  SVG::SVGRect
  arrowhead_outline_bbox(std::string_view dir,
                         std::string_view primitive_arrow_shape) const;
  /// Return the outline bounding box of the edge arrowtail. The outline
  /// bounding box is the bounding box with penwidth taken into account.
  SVG::SVGRect
  arrowtail_outline_bbox(std::string_view dir,
                         std::string_view primitive_arrow_shape) const;
  /// Return the bounding box of the edge
  SVG::SVGRect bbox() const;
  /// Return the center of the edge's bounding box
  SVG::SVGPoint center() const;
  /// Return the edge's `color` attribute in RGB hex format if the opacity is
  /// 100 % or in RGBA hex format otherwise.
  std::string color() const;
  /// Return the 'edgeop' according to the DOT language specification. Note that
  /// this is not the same as the 'id' attribute of an edge.
  std::string_view edgeop() const;
  /// Return the edge's `fillcolor` attribute in RGB hex format if the opacity
  /// is 100 % or in RGBA hex format otherwise.
  std::string fillcolor() const;
  /// Return the outline bounding box of the edge. The outline bounding box is
  /// the bounding box with penwidth taken into account.
  SVG::SVGRect outline_bbox() const;
  /// Return the edge stem, i.e., the part of the edge that does not include any
  /// arrowhead or arrowtail
  SVG::SVGElement &stem() const;
  /// Return the edge's `penwidth` attribute
  double penwidth() const;
  /// Return a non-mutable reference to the SVG `g` element corresponding to the
  /// edge
  const SVG::SVGElement &svg_g_element() const;

private:
  /// The 'edgeop' according to the DOT language specification
  std::string m_edgeop;
  /// The SVG `g` element corresponding to the edge
  SVG::SVGElement &m_svg_g_element;
};
