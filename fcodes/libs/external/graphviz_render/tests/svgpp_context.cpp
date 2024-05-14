#include <any>
#include <stdexcept>

#include <boost/array.hpp>
#include <fmt/format.h>

#include "svg_analyzer_interface.h"
#include "svgpp_context.h"

static std::string to_color_string(SvgppContext::color_t color) {
  return fmt::format("rgb({},{},{})", (color >> 16) & 0xff, (color >> 8) & 0xff,
                     (color >> 0) & 0xff);
}

SvgppContext::SvgppContext(ISVGAnalyzer *svg_analyzer)
    : m_svg_analyzer(svg_analyzer){};

void SvgppContext::on_enter_element(svgpp::tag::element::svg) {
  m_svg_analyzer->on_enter_element_svg();
}

void SvgppContext::on_enter_element(svgpp::tag::element::g) {
  m_svg_analyzer->on_enter_element_g();
}

void SvgppContext::on_enter_element(svgpp::tag::element::circle) {
  m_svg_analyzer->on_enter_element_circle();
}

void SvgppContext::on_enter_element(svgpp::tag::element::ellipse) {
  m_svg_analyzer->on_enter_element_ellipse();
}

void SvgppContext::on_enter_element(svgpp::tag::element::line) {
  m_svg_analyzer->on_enter_element_line();
}

void SvgppContext::on_enter_element(svgpp::tag::element::path) {
  m_svg_analyzer->on_enter_element_path();
}

void SvgppContext::on_enter_element(svgpp::tag::element::polygon) {
  m_svg_analyzer->on_enter_element_polygon();
}

void SvgppContext::on_enter_element(svgpp::tag::element::polyline) {
  m_svg_analyzer->on_enter_element_polyline();
}

void SvgppContext::on_enter_element(svgpp::tag::element::rect) {
  m_svg_analyzer->on_enter_element_rect();
}

void SvgppContext::on_enter_element(svgpp::tag::element::text) {
  m_svg_analyzer->on_enter_element_text();
}

void SvgppContext::on_enter_element(svgpp::tag::element::title) {
  m_svg_analyzer->on_enter_element_title();
}

void SvgppContext::on_exit_element() { m_svg_analyzer->on_exit_element(); }

void SvgppContext::path_move_to(double x, double y,
                                svgpp::tag::coordinate::absolute c) {
  if (!c.is_absolute) {
    throw std::runtime_error{
        "'path_move_to' using relative coordinates is not yet implemented"};
  }
  m_svg_analyzer->path_move_to(x, y);
}

void SvgppContext::path_line_to(double, double,
                                svgpp::tag::coordinate::absolute) {
  throw std::runtime_error{"'path_line_to' is not yet implemented"};
}

void SvgppContext::path_cubic_bezier_to(double x1, double y1, double x2,
                                        double y2, double x, double y,
                                        svgpp::tag::coordinate::absolute c) {
  if (!c.is_absolute) {
    throw std::runtime_error{
        "'path_cubic_bezier_to' using relative coordinates "
        "is not yet implemented"};
  }
  m_svg_analyzer->path_cubic_bezier_to(x1, y1, x2, y2, x, y);
}

void SvgppContext::path_quadratic_bezier_to(double, double, double, double,
                                            svgpp::tag::coordinate::absolute) {
  throw std::runtime_error{"'path_quadratic_bezier_to' is not yet implemented"};
}

void SvgppContext::path_elliptical_arc_to(double, double, double, bool, bool,
                                          double, double,
                                          svgpp::tag::coordinate::absolute) {
  throw std::runtime_error{"'path_elliptical_arc_to' is not yet implemented"};
}

void SvgppContext::path_close_subpath() {
  throw std::runtime_error{"'path_close_subpath' is not yet implemented"};
}

void SvgppContext::path_exit() { m_svg_analyzer->path_exit(); }

void SvgppContext::set(svgpp::tag::attribute::cy, const double v) {
  m_svg_analyzer->set_cy(v);
}

void SvgppContext::set(svgpp::tag::attribute::cx, const double v) {
  m_svg_analyzer->set_cx(v);
}

void SvgppContext::set(svgpp::tag::attribute::fill, svgpp::tag::value::none) {
  m_svg_analyzer->set_fill("none");
}

void SvgppContext::set(svgpp::tag::attribute::fill,
                       svgpp::tag::value::currentColor) {
  throw std::runtime_error{
      "the 'fill' attribute 'currentColor' value is not yet implemented"};
}

void SvgppContext::set(svgpp::tag::attribute::fill, color_t color,
                       svgpp::tag::skip_icc_color) {
  m_svg_analyzer->set_fill(to_color_string(color));
}

void SvgppContext::set(svgpp::tag::attribute::fill_opacity, const double v) {
  m_svg_analyzer->set_fill_opacity(v);
}

void SvgppContext::set(svgpp::tag::attribute::stroke, svgpp::tag::value::none) {
  m_svg_analyzer->set_stroke("none");
}

void SvgppContext::set(svgpp::tag::attribute::stroke,
                       svgpp::tag::value::currentColor) {
  throw std::runtime_error{
      "the 'stroke' attribute 'currentColor' value is not yet implemented"};
}

void SvgppContext::set(svgpp::tag::attribute::stroke,
                       SvgppContext::color_t color,
                       svgpp::tag::skip_icc_color) {
  m_svg_analyzer->set_stroke(to_color_string(color));
}

void SvgppContext::set(svgpp::tag::attribute::stroke_opacity, const double v) {
  m_svg_analyzer->set_stroke_opacity(v);
}

void SvgppContext::set(svgpp::tag::attribute::stroke_width, const double v) {
  m_svg_analyzer->set_stroke_width(v);
}

void SvgppContext::transform_matrix(const boost::array<double, 6> &matrix) {
  double a = matrix.at(0);
  double b = matrix.at(1);
  double c = matrix.at(2);
  double d = matrix.at(3);
  double e = matrix.at(4);
  double f = matrix.at(5);
  m_svg_analyzer->set_transform(a, b, c, d, e, f);
}

void SvgppContext::set(svgpp::tag::attribute::r, const double) {
  throw std::runtime_error{"the 'r' attribute is not yet implemented"};
}

void SvgppContext::set(svgpp::tag::attribute::rx, const double v) {
  m_svg_analyzer->set_rx(v);
}

void SvgppContext::set(svgpp::tag::attribute::ry, const double v) {
  m_svg_analyzer->set_ry(v);
}

void SvgppContext::set(svgpp::tag::attribute::x1, const double) {
  throw std::runtime_error{"the 'x1' attribute is not yet implemented"};
}

void SvgppContext::set(svgpp::tag::attribute::y1, const double) {
  throw std::runtime_error{"the 'y1' attribute is not yet implemented"};
}

void SvgppContext::set(svgpp::tag::attribute::x2, const double) {
  throw std::runtime_error{"the 'x2' attribute is not yet implemented"};
}

void SvgppContext::set(svgpp::tag::attribute::y2, const double) {
  throw std::runtime_error{"the 'y2' attribute is not yet implemented"};
}

void SvgppContext::set(svgpp::tag::attribute::x, const double v) {
  m_svg_analyzer->set_x(v);
}

void SvgppContext::set(svgpp::tag::attribute::x, const NumbersRange &range) {
  if (boost::size(range) != 1) {
    throw std::runtime_error{
        "Multiple value list for the 'x' attribute is not yet implemented"};
  }
  m_svg_analyzer->set_x(*range.begin());
}

void SvgppContext::set(svgpp::tag::attribute::y, const double v) {
  m_svg_analyzer->set_y(v);
}

void SvgppContext::set(svgpp::tag::attribute::y, const NumbersRange &range) {
  if (boost::size(range) != 1) {
    throw std::runtime_error{
        "Multiple value list for the 'y' attribute is not yet implemented"};
  }
  m_svg_analyzer->set_y(*range.begin());
}

void SvgppContext::set(svgpp::tag::attribute::width, const double v) {
  m_svg_analyzer->set_width(v);
}

void SvgppContext::set(svgpp::tag::attribute::height, const double v) {
  m_svg_analyzer->set_height(v);
}

void SvgppContext::set(svgpp::tag::attribute::id,
                       boost::iterator_range<const char *> v) {
  m_svg_analyzer->set_id({v.begin(), v.end()});
}

void SvgppContext::set(svgpp::tag::attribute::class_,
                       boost::iterator_range<const char *> v) {
  m_svg_analyzer->set_class({v.begin(), v.end()});
}

void SvgppContext::set(svgpp::tag::attribute::font_family,
                       boost::iterator_range<const char *> v) {
  m_svg_analyzer->set_font_family({v.begin(), v.end()});
}

void SvgppContext::set(svgpp::tag::attribute::font_size, const double v) {
  m_svg_analyzer->set_font_size(v);
}

void SvgppContext::set(svgpp::tag::attribute::font_size,
                       svgpp::tag::value::xx_small) {
  throw std::runtime_error{
      "the 'font_size' attribute 'xx_small' value is not yet implemented"};
}

void SvgppContext::set(svgpp::tag::attribute::font_size,
                       svgpp::tag::value::x_small) {
  throw std::runtime_error{
      "the 'font_size' attribute 'x_small' value is not yet implemented"};
}

void SvgppContext::set(svgpp::tag::attribute::font_size,
                       svgpp::tag::value::smaller) {
  throw std::runtime_error{
      "the 'font_size' attribute 'smaller' value is not yet implemented"};
}

void SvgppContext::set(svgpp::tag::attribute::font_size,
                       svgpp::tag::value::small) {
  throw std::runtime_error{
      "the 'font_size' attribute 'small' value is not yet implemented"};
}

void SvgppContext::set(svgpp::tag::attribute::font_size,
                       svgpp::tag::value::medium) {
  throw std::runtime_error{
      "the 'font_size' attribute 'medium' value is not yet implemented"};
}

void SvgppContext::set(svgpp::tag::attribute::font_size,
                       svgpp::tag::value::large) {
  throw std::runtime_error{
      "the 'font_size' attribute 'large' value is not yet implemented"};
}

void SvgppContext::set(svgpp::tag::attribute::font_size,
                       svgpp::tag::value::larger) {
  throw std::runtime_error{
      "the 'font_size' attribute 'larger' value is not yet implemented"};
}

void SvgppContext::set(svgpp::tag::attribute::font_size,
                       svgpp::tag::value::x_large) {
  throw std::runtime_error{
      "the 'font_size' attribute 'x_large' value is not yet implemented"};
}

void SvgppContext::set(svgpp::tag::attribute::font_size,
                       svgpp::tag::value::xx_large) {
  throw std::runtime_error{
      "the 'font_size' attribute 'xx_large' value is not yet implemented"};
}

void SvgppContext::set(svgpp::tag::attribute::text_anchor,
                       svgpp::tag::value::start) {
  m_svg_analyzer->set_text_anchor("start");
}

void SvgppContext::set(svgpp::tag::attribute::text_anchor,
                       svgpp::tag::value::middle) {
  m_svg_analyzer->set_text_anchor("middle");
}

void SvgppContext::set(svgpp::tag::attribute::text_anchor,
                       svgpp::tag::value::end) {
  m_svg_analyzer->set_text_anchor("end");
}

void SvgppContext::set(svgpp::tag::attribute::viewBox, const double v1,
                       const double v2, const double v3, const double v4) {
  m_svg_analyzer->set_viewBox(v1, v2, v3, v4);
}

void SvgppContext::set(svgpp::tag::attribute::points,
                       const SvgppContext::PointsRange &range) {
  for (auto &it : range) {
    m_svg_analyzer->set_point(it);
  }
}

void SvgppContext::set_text(boost::iterator_range<const char *> v) {
  m_svg_analyzer->set_text({v.begin(), v.end()});
}
