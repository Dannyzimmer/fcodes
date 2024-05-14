#include <memory>
#include <stdexcept>

#include <boost/algorithm/string.hpp>
#include <catch2/catch.hpp>
#include <fmt/format.h>

#include "svg_analyzer.h"
#include "svgpp_context.h"
#include "svgpp_document_traverser.h"
#include <cgraph++/AGraph.h>
#include <gvc++/GVContext.h>
#include <gvc++/GVLayout.h>
#include <gvc++/GVRenderData.h>

SVGAnalyzer::SVGAnalyzer(char *text)
    : m_original_svg(text), m_svg(SVG::SVGElement(SVG::SVGElementType::Svg)) {
  m_elements_in_process.push_back(&m_svg);
  SvgppContext context{this};
  traverseDocumentWithSvgpp(context, text);
  if (m_elements_in_process.size() != 1) {
    throw std::runtime_error{
        "Wrong number of elements in process after traversing SVG document"};
  }
  m_elements_in_process.pop_back();
  retrieve_graphviz_components();
}

const std::vector<GraphvizGraph> &SVGAnalyzer::graphs() const {
  return m_graphs;
}

void SVGAnalyzer::on_enter_element_svg() { m_num_svgs++; }

void SVGAnalyzer::on_enter_element_g() {
  enter_element(SVG::SVGElementType::Group);
  m_num_groups++;
}

void SVGAnalyzer::on_enter_element_circle() {
  enter_element(SVG::SVGElementType::Circle);
  m_num_circles++;
}

void SVGAnalyzer::on_enter_element_ellipse() {
  enter_element(SVG::SVGElementType::Ellipse);
  m_num_ellipses++;
}

void SVGAnalyzer::on_enter_element_line() {
  enter_element(SVG::SVGElementType::Line);
  m_num_lines++;
}

void SVGAnalyzer::on_enter_element_path() {
  enter_element(SVG::SVGElementType::Path);
  m_num_paths++;
}

void SVGAnalyzer::on_enter_element_polygon() {
  enter_element(SVG::SVGElementType::Polygon);
  m_num_polygons++;
}

void SVGAnalyzer::on_enter_element_polyline() {
  enter_element(SVG::SVGElementType::Polyline);
  m_num_polylines++;
}

void SVGAnalyzer::on_enter_element_rect() {
  enter_element(SVG::SVGElementType::Rect);
  m_num_rects++;
}

void SVGAnalyzer::on_enter_element_text() {
  enter_element(SVG::SVGElementType::Text);
  m_num_texts++;
}

void SVGAnalyzer::on_enter_element_title() {
  enter_element(SVG::SVGElementType::Title);
  m_num_titles++;
}

void SVGAnalyzer::on_exit_element() { m_elements_in_process.pop_back(); }

void SVGAnalyzer::path_cubic_bezier_to(double x1, double y1, double x2,
                                       double y2, double x, double y) {
  auto &path_points = current_element().path_points;
  path_points.emplace_back(x1, y1);
  path_points.emplace_back(x2, y2);
  path_points.emplace_back(x, y);
}

void SVGAnalyzer::path_move_to(double x, double y) {
  auto &paths = current_element().path_points;
  paths.emplace_back(x, y);
}

void SVGAnalyzer::path_exit() {}

SVG::SVGElement &SVGAnalyzer::grandparent_element() {
  if (m_elements_in_process.empty()) {
    throw std::runtime_error{"No current element to get grandparent of"};
  }
  if (m_elements_in_process.size() == 1) {
    throw std::runtime_error{"No parent element"};
  }
  if (m_elements_in_process.size() == 2) {
    throw std::runtime_error{"No grandparent element"};
  }
  return *m_elements_in_process.end()[-3];
}

SVG::SVGElement &SVGAnalyzer::parent_element() {
  if (m_elements_in_process.empty()) {
    throw std::runtime_error{"No current element to get parent of"};
  }
  if (m_elements_in_process.size() == 1) {
    throw std::runtime_error{"No parent element"};
  }
  return *m_elements_in_process.end()[-2];
}

SVG::SVGElement &SVGAnalyzer::current_element() {
  if (m_elements_in_process.empty()) {
    throw std::runtime_error{"No current element"};
  }
  return *m_elements_in_process.back();
}

void SVGAnalyzer::enter_element(SVG::SVGElementType type) {
  if (m_elements_in_process.empty()) {
    throw std::runtime_error{
        "No element is currently being processed by the SVG++ document "
        "traverser when entering a new element. Expecting at least the top "
        "level 'svg' to be in process"};
  }
  auto &element = current_element();
  element.children.emplace_back(type);
  m_elements_in_process.push_back(&element.children.back());
}

void SVGAnalyzer::set_class(std::string_view class_) {
  current_element().attributes.class_ = class_;
}

void SVGAnalyzer::retrieve_graphviz_components() {
  retrieve_graphviz_components_impl(m_svg);
}

void SVGAnalyzer::retrieve_graphviz_components_impl(
    SVG::SVGElement &svg_element) {
  if (svg_element.type == SVG::SVGElementType::Group) {
    // The SVG 'class' attribute determines which type of Graphviz element a 'g'
    // element corresponds to
    const auto &class_ = svg_element.attributes.class_;
    if (class_ == "graph") {
      m_graphs.emplace_back(svg_element);
    } else if (class_ == "node") {
      if (m_graphs.empty()) {
        throw std::runtime_error{
            fmt::format("No graph to add node {} to", svg_element.graphviz_id)};
      }
      m_graphs.back().add_node(svg_element);
    } else if (class_ == "edge") {
      if (m_graphs.empty()) {
        throw std::runtime_error{
            fmt::format("No graph to add edge {} to", svg_element.graphviz_id)};
      }
      m_graphs.back().add_edge(svg_element);
    } else if (class_ == "cluster") {
      // ignore for now
    } else {
      throw std::runtime_error{fmt::format("Unknown class {}", class_)};
    }
  }

  for (auto &child : svg_element.children) {
    retrieve_graphviz_components_impl(child);
  }
}

void SVGAnalyzer::re_create_and_verify_svg() {

  const auto indent_size = 0;
  auto recreated_svg = svg_string(indent_size);

  // compare the recreated SVG with the original SVG
  if (recreated_svg != m_original_svg) {
    std::vector<std::string> original_svg_lines;
    boost::split(original_svg_lines, m_original_svg, boost::is_any_of("\n"));

    std::vector<std::string> recreated_svg_lines;
    boost::split(recreated_svg_lines, recreated_svg, boost::is_any_of("\n"));

    for (std::size_t i = 0; i < original_svg_lines.size(); i++) {
      REQUIRE(i < recreated_svg_lines.size());
      REQUIRE(recreated_svg_lines[i] == original_svg_lines[i]);
    }

    REQUIRE(recreated_svg_lines.size() == original_svg_lines.size());
  }
}

void SVGAnalyzer::set_cx(double cx) { current_element().attributes.cx = cx; }

void SVGAnalyzer::set_cy(double cy) { current_element().attributes.cy = cy; }

void SVGAnalyzer::set_font_family(std::string_view font_family) {
  current_element().attributes.font_family = font_family;
}

void SVGAnalyzer::set_font_size(double font_size) {
  current_element().attributes.font_size = font_size;
}

void SVGAnalyzer::set_fill(std::string_view fill) {
  current_element().attributes.fill = fill;
}

void SVGAnalyzer::set_fill_opacity(double fill_opacity) {
  current_element().attributes.fill_opacity = fill_opacity;
}

void SVGAnalyzer::set_height(double height) {
  current_element().attributes.height = height;
}

void SVGAnalyzer::set_stroke(std::string_view stroke) {
  current_element().attributes.stroke = stroke;
}

void SVGAnalyzer::set_stroke_opacity(double stroke_opacity) {
  current_element().attributes.stroke_opacity = stroke_opacity;
}

void SVGAnalyzer::set_stroke_width(double stroke_width) {
  current_element().attributes.stroke_width = stroke_width;
}

void SVGAnalyzer::set_id(std::string_view id) {
  current_element().attributes.id = id;
}

void SVGAnalyzer::set_rx(double rx) { current_element().attributes.rx = rx; }

void SVGAnalyzer::set_ry(double ry) { current_element().attributes.ry = ry; }

void SVGAnalyzer::set_point(std::pair<double, double> point) {
  current_element().attributes.points.emplace_back(point.first, point.second);
}

void SVGAnalyzer::set_text(std::string_view text) {
  auto &element = current_element();
  element.text = text;

  if (element.type == SVG::SVGElementType::Title) {
    // The title text is normally the 'graph_id', 'node_id' or 'edgeop'
    // according to the DOT language specification. Save it on the parent 'g'
    // element to avoid having to look it up later.
    if (parent_element().type != SVG::SVGElementType::Group) {
      throw std::runtime_error{"Unexpected parent element of 'title' element"};
    }
    parent_element().graphviz_id = text;
    // If the 'g' element corresponds to the graph, set the Graphviz ID also on
    // the top level 'svg' element.
    if (grandparent_element().type == SVG::SVGElementType::Svg) {
      grandparent_element().graphviz_id = text;
    }
  }
}

void SVGAnalyzer::set_text_anchor(std::string_view text_anchor) {
  current_element().attributes.text_anchor = text_anchor;
}

void SVGAnalyzer::set_width(double width) {
  current_element().attributes.width = width;
}

void SVGAnalyzer::set_x(double x) { current_element().attributes.x = x; }

void SVGAnalyzer::set_y(double y) { current_element().attributes.y = y; }

void SVGAnalyzer::add_bboxes() {
  for (auto &graph : m_graphs) {
    graph.add_bboxes();
  }
}

void SVGAnalyzer::add_node_edge_outline_bbox_overlaps(double tolerance) {
  for (auto &graph : m_graphs) {
    graph.add_node_edge_outline_bbox_overlaps(tolerance);
  }
}

void SVGAnalyzer::add_outline_bboxes() {
  for (auto &graph : m_graphs) {
    graph.add_outline_bboxes();
  }
}

SVGAnalyzer SVGAnalyzer::make_from_dot(const std::string &dot_source,
                                       const std::string &engine) {
  auto g = CGraph::AGraph{dot_source};

  const auto demand_loading = false;
  auto gvc =
      std::make_shared<GVC::GVContext>(lt_preloaded_symbols, demand_loading);

  const auto layout = GVC::GVLayout(gvc, std::move(g), engine);

  const auto result = layout.render("svg");

  auto svg_analyzer = SVGAnalyzer{result.c_str()};

  svg_analyzer.set_graphviz_version(gvc->version());
  svg_analyzer.set_graphviz_build_date(gvc->buildDate());

  return svg_analyzer;
}

std::string_view SVGAnalyzer::original_svg() const { return m_original_svg; }

void SVGAnalyzer::set_transform(double a, double b, double c, double d,
                                double e, double f) {
  current_element().attributes.transform = {a, b, c, d, e, f};
}

void SVGAnalyzer::set_viewBox(double x, double y, double width, double height) {
  current_element().attributes.viewBox = {x, y, width, height};
}

void SVGAnalyzer::set_graphviz_version(std::string_view version) {
  m_svg.graphviz_version = version;
}

void SVGAnalyzer::set_graphviz_build_date(std::string_view build_date) {
  m_svg.graphviz_build_date = build_date;
}

std::string SVGAnalyzer::svg_string(std::size_t indent_size) const {
  std::string output{};
  output += m_svg.to_string(indent_size);
  return output;
}
