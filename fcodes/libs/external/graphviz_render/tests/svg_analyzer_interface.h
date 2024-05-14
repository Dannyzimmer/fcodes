#pragma once

#include <string_view>

/**
 * @brief The ISVGAnalyzer class is an interface class declaring
 * callbacks that can be implemented by an SVGAnalyzer class. Its
 * purpose is to isolate the SVGAnalyzer class from the SvgppContext
 * class to avoid recompiling the SvgppContext class when the
 * SVGAnalyzer class is changed, which is expected to happen
 * frequently.
 */

class ISVGAnalyzer {
public:
  virtual ~ISVGAnalyzer() = default;
  virtual void on_enter_element_svg() = 0;
  virtual void on_enter_element_g() = 0;
  virtual void on_enter_element_circle() = 0;
  virtual void on_enter_element_ellipse() = 0;
  virtual void on_enter_element_line() = 0;
  virtual void on_enter_element_path() = 0;
  virtual void on_enter_element_polygon() = 0;
  virtual void on_enter_element_polyline() = 0;
  virtual void on_enter_element_rect() = 0;
  virtual void on_enter_element_text() = 0;
  virtual void on_enter_element_title() = 0;
  virtual void on_exit_element() = 0;
  virtual void path_exit() = 0;
  virtual void path_move_to(double x, double y) = 0;
  virtual void path_cubic_bezier_to(double x1, double y1, double x2, double y2,
                                    double x, double y) = 0;
  virtual void set_class(std::string_view) = 0;
  virtual void set_cx(double cx) = 0;
  virtual void set_cy(double cy) = 0;
  virtual void set_font_family(std::string_view font_family) = 0;
  virtual void set_font_size(double font_size) = 0;
  virtual void set_fill(std::string_view fill) = 0;
  virtual void set_fill_opacity(double fill_opacity) = 0;
  virtual void set_height(double height) = 0;
  virtual void set_id(std::string_view id) = 0;
  virtual void set_rx(double rx) = 0;
  virtual void set_ry(double ry) = 0;
  virtual void set_point(std::pair<double, double> point) = 0;
  virtual void set_stroke(std::string_view stroke) = 0;
  virtual void set_stroke_opacity(double stroke_opacity) = 0;
  virtual void set_stroke_width(double stroke_width) = 0;
  virtual void set_text(std::string_view text) = 0;
  virtual void set_text_anchor(std::string_view text_anchor) = 0;
  virtual void set_transform(double a, double b, double c, double d, double e,
                             double f) = 0;
  virtual void set_viewBox(double x, double y, double width, double height) = 0;
  virtual void set_width(double width) = 0;
  virtual void set_x(double x) = 0;
  virtual void set_y(double y) = 0;
};
