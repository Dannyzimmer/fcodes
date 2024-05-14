#pragma once

#include <stdexcept>

#include <boost/range/any_range.hpp>
#include <boost/range/iterator_range_core.hpp>
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
#endif
#include <svgpp/svgpp.hpp>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

class ISVGAnalyzer;

/**
 * @brief The SvgppContext class provides a context containing SVG element
 * callbacks for the SVG++ parser which is called from the SVG document
 * traverser and forwards these callbacks to the SVG analyzer. It's separated
 * from the SVG analyzer to avoid the very time consuming recompilation of the
 * SVG document traverser when then SVG analyzer header is changed, which is
 * expected to happen often as new functionality is added.
 *
 * Most of this is taken from
 * http://svgpp.org/lesson01.html#handling-shapes-geometry.
 */

class SvgppContext {
public:
  SvgppContext(ISVGAnalyzer *svg_analyzer);
  void on_enter_element(svgpp::tag::element::svg e);
  void on_enter_element(svgpp::tag::element::g e);
  void on_enter_element(svgpp::tag::element::circle e);
  void on_enter_element(svgpp::tag::element::ellipse e);
  void on_enter_element(svgpp::tag::element::line e);
  void on_enter_element(svgpp::tag::element::path e);
  void on_enter_element(svgpp::tag::element::polygon e);
  void on_enter_element(svgpp::tag::element::polyline e);
  void on_enter_element(svgpp::tag::element::rect e);
  void on_enter_element(svgpp::tag::element::text e);
  void on_enter_element(svgpp::tag::element::title e);
  void on_exit_element();
  void path_move_to(double x, double y, svgpp::tag::coordinate::absolute);
  void path_line_to(double x, double y, svgpp::tag::coordinate::absolute);
  void path_cubic_bezier_to(double x1, double y1, double x2, double y2,
                            double x, double y,
                            svgpp::tag::coordinate::absolute);
  void path_quadratic_bezier_to(double x1, double y1, double x, double y,
                                svgpp::tag::coordinate::absolute);
  void path_elliptical_arc_to(double rx, double ry, double x_axis_rotation,
                              bool large_arc_flag, bool sweep_flag, double x,
                              double y, svgpp::tag::coordinate::absolute);
  void path_close_subpath();
  void path_exit();
  void set(svgpp::tag::attribute::cy cy, double v);
  void set(svgpp::tag::attribute::cx cx, double v);

  void set(svgpp::tag::attribute::fill, svgpp::tag::value::none);
  void set(svgpp::tag::attribute::fill, svgpp::tag::value::currentColor);
  using color_t = int;
  void set(svgpp::tag::attribute::fill, color_t color,
           svgpp::tag::skip_icc_color = svgpp::tag::skip_icc_color());
  template <class IRI> void set(svgpp::tag::attribute::fill, IRI const &) {
    throw std::runtime_error{
        "this flavor of the 'fill' attribute is not yet implemented"};
  };
  template <class IRI>
  void set(svgpp::tag::attribute::fill, svgpp::tag::iri_fragment, IRI const &) {
    throw std::runtime_error{
        "this flavor of the 'fill' attribute is not yet implemented"};
  };
  template <class IRI>
  void set(svgpp::tag::attribute::fill, IRI const &, svgpp::tag::value::none) {
    throw std::runtime_error{
        "this flavor of the 'fill' attribute is not yet implemented"};
  };
  template <class IRI>
  void set(svgpp::tag::attribute::fill, svgpp::tag::iri_fragment, IRI const &,
           svgpp::tag::value::none) {
    throw std::runtime_error{
        "this flavor of the 'fill' attribute is not yet implemented"};
  };
  template <class IRI>
  void set(svgpp::tag::attribute::fill, IRI const &,
           svgpp::tag::value::currentColor) {
    throw std::runtime_error{
        "this flavor of the 'fill' attribute is not yet implemented"};
  };
  template <class IRI>
  void set(svgpp::tag::attribute::fill, svgpp::tag::iri_fragment, IRI const &,
           svgpp::tag::value::currentColor) {
    throw std::runtime_error{
        "this flavor of the 'fill' attribute is not yet implemented"};
  };
  template <class IRI>
  void set(svgpp::tag::attribute::fill, IRI const &, color_t,
           svgpp::tag::skip_icc_color = svgpp::tag::skip_icc_color()) {
    throw std::runtime_error{
        "this flavor of the 'fill' attribute is not yet implemented"};
  };
  template <class IRI>
  void set(svgpp::tag::attribute::fill, svgpp::tag::iri_fragment, IRI const &,
           color_t, svgpp::tag::skip_icc_color = svgpp::tag::skip_icc_color()) {
    throw std::runtime_error{
        "this flavor of the 'fill' attribute is not yet implemented"};
  };

  void set(svgpp::tag::attribute::fill_opacity, double v);
  void set(svgpp::tag::attribute::stroke, svgpp::tag::value::none);
  void set(svgpp::tag::attribute::stroke, svgpp::tag::value::currentColor);
  void set(svgpp::tag::attribute::stroke, color_t color,
           svgpp::tag::skip_icc_color = svgpp::tag::skip_icc_color());
  template <class IRI> void set(svgpp::tag::attribute::stroke, IRI const &) {
    throw std::runtime_error{
        "this flavor of the 'stroke' attribute is not yet implemented"};
  };
  template <class IRI>
  void set(svgpp::tag::attribute::stroke, svgpp::tag::iri_fragment,
           IRI const &) {
    throw std::runtime_error{
        "this flavor of the 'stroke' attribute is not yet implemented"};
  };
  template <class IRI>
  void set(svgpp::tag::attribute::stroke, IRI const &,
           svgpp::tag::value::none) {
    throw std::runtime_error{
        "this flavor of the 'stroke' attribute is not yet implemented"};
  };
  template <class IRI>
  void set(svgpp::tag::attribute::stroke, svgpp::tag::iri_fragment, IRI const &,
           svgpp::tag::value::none) {
    throw std::runtime_error{
        "this flavor of the 'stroke' attribute is not yet implemented"};
  };
  template <class IRI>
  void set(svgpp::tag::attribute::stroke, IRI const &,
           svgpp::tag::value::currentColor) {
    throw std::runtime_error{
        "this flavor of the 'stroke' attribute is not yet implemented"};
  };
  template <class IRI>
  void set(svgpp::tag::attribute::stroke, svgpp::tag::iri_fragment, IRI const &,
           svgpp::tag::value::currentColor) {
    throw std::runtime_error{
        "this flavor of the 'stroke' attribute is not yet implemented"};
  };
  template <class IRI>
  void set(svgpp::tag::attribute::stroke, IRI const &, color_t,
           svgpp::tag::skip_icc_color = svgpp::tag::skip_icc_color()) {
    throw std::runtime_error{
        "this flavor of the 'stroke' attribute is not yet implemented"};
  };
  template <class IRI>
  void set(svgpp::tag::attribute::stroke, svgpp::tag::iri_fragment, IRI const &,
           color_t, svgpp::tag::skip_icc_color = svgpp::tag::skip_icc_color()) {
    throw std::runtime_error{
        "this flavor of the 'stroke' attribute is not yet implemented"};
  };
  void set(svgpp::tag::attribute::stroke_opacity, double v);
  void set(svgpp::tag::attribute::stroke_width, double v);
  void transform_matrix(const boost::array<double, 6> &matrix);
  void set(svgpp::tag::attribute::r r, double v);
  void set(svgpp::tag::attribute::rx rx, double v);
  void set(svgpp::tag::attribute::ry ry, double v);
  void set(svgpp::tag::attribute::x1 x1, double v);
  void set(svgpp::tag::attribute::y1 y1, double v);
  void set(svgpp::tag::attribute::x2 x2, double v);
  void set(svgpp::tag::attribute::y2 y2, double v);
  typedef boost::any_range<std::pair<double, double>,
                           boost::single_pass_traversal_tag,
                           const std::pair<double, double> &, std::ptrdiff_t>
      PointsRange;
  void set(svgpp::tag::attribute::points points, const PointsRange &range);
  void set(svgpp::tag::attribute::x, double v);
  typedef boost::any_range<double, boost::single_pass_traversal_tag, double,
                           std::ptrdiff_t>
      NumbersRange;
  void set(svgpp::tag::attribute::x, const NumbersRange &range);
  void set(svgpp::tag::attribute::y, double v);
  void set(svgpp::tag::attribute::y, const NumbersRange &range);
  void set(svgpp::tag::attribute::width width, double v);
  void set(svgpp::tag::attribute::height height, double v);
  void set(svgpp::tag::attribute::id a, boost::iterator_range<const char *> v);
  void set(svgpp::tag::attribute::class_ a,
           boost::iterator_range<const char *> v);
  void set(svgpp::tag::attribute::font_family a,
           boost::iterator_range<const char *> v);
  void set(svgpp::tag::attribute::font_size a, double v);
  void set(svgpp::tag::attribute::font_size a, svgpp::tag::value::xx_small v);
  void set(svgpp::tag::attribute::font_size a, svgpp::tag::value::x_small v);
  void set(svgpp::tag::attribute::font_size a, svgpp::tag::value::smaller v);
  void set(svgpp::tag::attribute::font_size a, svgpp::tag::value::small v);
  void set(svgpp::tag::attribute::font_size a, svgpp::tag::value::medium v);
  void set(svgpp::tag::attribute::font_size a, svgpp::tag::value::large v);
  void set(svgpp::tag::attribute::font_size a, svgpp::tag::value::larger v);
  void set(svgpp::tag::attribute::font_size a, svgpp::tag::value::x_large v);
  void set(svgpp::tag::attribute::font_size a, svgpp::tag::value::xx_large v);
  void set(svgpp::tag::attribute::text_anchor a, svgpp::tag::value::start v);
  void set(svgpp::tag::attribute::text_anchor a, svgpp::tag::value::middle v);
  void set(svgpp::tag::attribute::text_anchor a, svgpp::tag::value::end v);
  void set(svgpp::tag::attribute::viewBox a, double v1, double v2, double v3,
           double v4);
  void set_text(boost::iterator_range<const char *> v);

private:
  ISVGAnalyzer *m_svg_analyzer = nullptr;
};
