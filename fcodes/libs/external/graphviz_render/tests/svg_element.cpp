#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <stdexcept>

#include <fmt/format.h>

#include "svg_element.h"
#include <cgraph/unreachable.h>

SVG::SVGElement::SVGElement(SVGElementType type) : type(type) {}

static double px_to_pt(double px) {
  // a `pt` is 0.75 `px`. See e.g.
  // https://oreillymedia.github.io/Using_SVG/guide/units.html
  return px * 3 / 4;
}

bool SVG::SVGElement::is_closed_shape_element() const {
  switch (type) {
  case SVG::SVGElementType::Circle:
  case SVG::SVGElementType::Ellipse:
  case SVG::SVGElementType::Polygon:
  case SVG::SVGElementType::Rect:
    return true;
  default:
    return false;
  }
}

bool SVG::SVGElement::is_shape_element() const {
  switch (type) {
  case SVG::SVGElementType::Circle:
  case SVG::SVGElementType::Ellipse:
  case SVG::SVGElementType::Line:
  case SVG::SVGElementType::Path:
  case SVG::SVGElementType::Polygon:
  case SVG::SVGElementType::Polyline:
  case SVG::SVGElementType::Rect:
    return true;
  default:
    return false;
  }
}

static std::string xml_encode(const std::string &text) {
  std::string out;
  for (const char &ch : text) {
    switch (ch) {
    case '>':
      out += "&gt;";
      break;
    case '<':
      out += "&lt;";
      break;
    case '-':
      out += "&#45;";
      break;
    case '&':
      out += "&amp;";
      break;
    default:
      out += ch;
    }
  }
  return out;
}

static std::string rgb_to_hex(const std::string &color, double opacity = 1.0) {
  const auto comma1 = color.find_first_of(",");
  const auto r = std::stoi(color.substr(4, comma1 - 1));
  const auto comma2 = color.find_first_of(",", comma1 + 1);
  const auto g = std::stoi(color.substr(comma1 + 1, comma2 - 1));
  const auto close_par = color.find_first_of(")", comma2 + 1);
  const auto b = std::stoi(color.substr(comma2 + 1, close_par - 1));
  const auto opacity_int = std::lround(opacity * 255.0);
  const auto opacity_hex_str =
      opacity_int < 255 ? fmt::format("{:02x}", opacity_int) : "";
  return fmt::format("#{:02x}{:02x}{:02x}{}", r, g, b, opacity_hex_str);
}

// convert a valid color specification to the flavor that Graphviz uses in the
// SVG
static std::string to_graphviz_color(const std::string &color) {
  if (color == "rgb(0,0,0)") {
    return "black";
  } else if (color == "rgb(255,255,255)") {
    return "white";
  } else if (color.starts_with("rgb")) {
    return rgb_to_hex(color);
  } else {
    return color;
  }
}

// convert a valid color specification to the RGB or RGBA type that Graphviz
// uses in the DOT source
std::string SVG::to_dot_color(const std::string &color, double opacity) {
  if (color == "none") {
    return "#00000000";
  }
  if (opacity < 1.0) {
    if (!color.starts_with("rgb")) {
      throw std::runtime_error{fmt::format(
          "Cannot convert stroke={}, stroke_opacity={} to Graphviz color",
          color, opacity)};
    }
  }
  return rgb_to_hex(color, opacity);
}

void SVG::SVGElement::add_bbox() {
  const auto bbox = SVGElement::bbox();
  add_rect(bbox, "green");
}

void SVG::SVGElement::add_rect(SVGRect rect, const std::string color) {
  SVG::SVGElement element{SVG::SVGElementType::Rect};
  element.attributes.x = rect.x;
  element.attributes.y = rect.y;
  element.attributes.width = rect.width;
  element.attributes.height = rect.height;
  element.attributes.stroke_width = 0.1;
  element.attributes.stroke = color;
  element.attributes.fill = "none";
  children.push_back(element);
}

void SVG::SVGElement::add_outline_bbox() {
  const auto bbox = SVGElement::outline_bbox();
  add_rect(bbox, "blue");
}

void SVG::SVGElement::add_outline_overlap_bbox(SVG::SVGElement other,
                                               const double tolerance) {
  const auto bbox = outline_bbox();
  const auto other_bbox = other.outline_bbox();
  const auto overlap_bbox = bbox.intersection(other_bbox);
  if (overlap_bbox.width <= 0 || overlap_bbox.height <= 0) {
    return;
  }
  const auto within_tolerance =
      overlap_bbox.width <= tolerance && overlap_bbox.height <= tolerance;
  const auto color = within_tolerance ? "yellow" : "red";
  add_rect(overlap_bbox, color);
}

SVG::SVGRect SVG::SVGElement::bbox(bool throw_if_bbox_not_defined) {
  if (!m_bbox.has_value()) {
    // negative width and height bbox that will be immediately replaced by the
    // first bbox found
    m_bbox = {.x = std::numeric_limits<double>::max() / 2,
              .y = std::numeric_limits<double>::max() / 2,
              .width = std::numeric_limits<double>::lowest(),
              .height = std::numeric_limits<double>::lowest()};
    switch (type) {
    case SVG::SVGElementType::Group:
      // SVG group bounding box is detemined solely by its children
      break;
    case SVG::SVGElementType::Ellipse: {
      m_bbox = {
          .x = attributes.cx - attributes.rx,
          .y = attributes.cy - attributes.ry,
          .width = attributes.rx * 2,
          .height = attributes.ry * 2,
      };
      break;
    }
    case SVG::SVGElementType::Polygon:
    case SVG::SVGElementType::Polyline: {
      for (const auto &point : attributes.points) {
        m_bbox->extend(point);
      }
      break;
    }
    case SVG::SVGElementType::Path: {
      if (path_points.empty()) {
        throw std::runtime_error{"No points for 'path' element"};
      }
      for (const auto &point : path_points) {
        m_bbox->extend(point);
      }
      break;
    }
    case SVG::SVGElementType::Rect: {
      m_bbox = {
          .x = attributes.x,
          .y = attributes.y,
          .width = attributes.width,
          .height = attributes.height,
      };
      break;
    }
    case SVG::SVGElementType::Text: {
      m_bbox = text_bbox();
      break;
    }
    case SVG::SVGElementType::Title:
      // title has no size
      if (throw_if_bbox_not_defined) {
        throw std::runtime_error{"A 'title' element has no bounding box"};
      }
      break;
    default:
      throw std::runtime_error{
          fmt::format("Unhandled svg element type {}", tag(type))};
    }

    const auto throw_if_child_bbox_is_not_defined = false;
    for (auto &child : children) {
      const auto child_bbox = child.bbox(throw_if_child_bbox_is_not_defined);
      m_bbox->extend(child_bbox);
    }
  }

  return *m_bbox;
}

SVG::SVGRect SVG::SVGElement::outline_bbox(bool throw_if_bbox_not_defined) {
  if (m_outline_bbox.has_value()) {
    return *m_outline_bbox;
  }
  // negative width and height bbox that will be immediately replaced by the
  // first bbox found
  m_bbox = {.x = std::numeric_limits<double>::max() / 2,
            .y = std::numeric_limits<double>::max() / 2,
            .width = std::numeric_limits<double>::lowest(),
            .height = std::numeric_limits<double>::lowest()};
  switch (type) {
  case SVG::SVGElementType::Group:
    // SVG group bounding box is detemined solely by its children
    break;
  case SVG::SVGElementType::Ellipse: {
    const auto stroke_width = (attributes.rx == 0 && attributes.ry == 0)
                                  ? 0
                                  : attributes.stroke_width;
    m_bbox = {
        .x = attributes.cx - attributes.rx - stroke_width / 2,
        .y = attributes.cy - attributes.ry - stroke_width / 2,
        .width = attributes.rx * 2 + stroke_width,
        .height = attributes.ry * 2 + stroke_width,
    };
    break;
  }
  case SVG::SVGElementType::Polygon: {
    // it takes at least 3 points to make a polygon (triangle) and Graphviz
    // always generates the last point to be the same as the first so there
    // will always be at least 4 points
    const auto &points = attributes.points;
    if (points.size() < 4) {
      throw std::runtime_error{"Too few points"};
    }
    if (points.front().x != points.back().x ||
        points.front().y != points.back().y) {
      throw std::runtime_error{"First and last point are not the same"};
    }
    if (has_all_points_equal()) {
      // the polygon has no size and will not be visible, so just arbitrarily
      // select one of its (equal) points as its outline bounding box
      const auto first_point = points.at(0);
      m_bbox->extend(first_point);
      break;
    }
    const auto clockwise = has_clockwise_points();
    // the first and last points are always the same so we skip the last
    for (auto it = points.cbegin(); it != points.cend() - 1; ++it) {
      const SVG::SVGPoint &prev_point = [&]() {
        if (it == points.begin()) {
          // the last point is the same as the first so we must use the next
          // to last one as the next point to get the start point of the
          // current path segment
          return *(points.cend() - 2);
        } else {
          return *std::prev(it);
        }
      }();
      const auto &point = *it;
      // there is always a next point since we iterate only to the next to
      // last point
      const auto &next_point = *std::next(it);
      const SVG::SVGPoints miter_shape =
          clockwise ?
                    // Graphviz draws some polygons clockwise and some
                    // counter-clockwise.
              SVGElement::miter_shape(prev_point, point, next_point)
                    :
                    // the SVG spec assumes clockwise so we swap the points
              SVGElement::miter_shape(next_point, point, prev_point);
      for (const auto &point : miter_shape) {
        m_bbox->extend(point);
      }
    }
    break;
  }
  case SVG::SVGElementType::Path: {
    if (path_points.empty()) {
      throw std::runtime_error{"No points for 'path' element"};
    }
    const auto first_point = path_points.front();
    auto is_vertical = std::all_of(
        path_points.cbegin(), path_points.cend(),
        [&](const SVGPoint &point) { return point.x == first_point.x; });
    auto is_horizontal = std::all_of(
        path_points.cbegin(), path_points.cend(),
        [&](const SVGPoint &point) { return point.y == first_point.y; });
    if (!is_vertical && !is_horizontal) {
      const std::size_t num_points_in_cylinder_node_shape_path1 = 19;
      const std::size_t num_points_in_cylinder_node_shape_path2 = 7;
      if (path_points.size() == num_points_in_cylinder_node_shape_path1 ||
          path_points.size() == num_points_in_cylinder_node_shape_path2) {
        // cylinder node shape which is flat at the extreme points so we can
        // just extend the crossing points with penwidth / 2 and exclude the
        // intermediate control points. Graphviz uses cubic splines so there are
        // always two intermediate control points between the curve segment
        // endpoints.
        const auto num_intermediate_control_points = 2;
        for (std::size_t i = 0; i < path_points.size();
             i += num_intermediate_control_points + 1) {
          const auto &point = path_points[i];
          SVG::SVGRect point_bbox = {
              .x = point.x - attributes.stroke_width / 2,
              .y = point.y - attributes.stroke_width / 2,
              .width = attributes.stroke_width,
              .height = attributes.stroke_width,
          };
          m_bbox->extend(point_bbox);
        }
        break;
      }
      const std::size_t num_points_in_curve_arrow_shape_path = 4;
      if (path_points.size() == num_points_in_curve_arrow_shape_path) {
        const auto horizontal_start = path_points[0].x;
        const auto horizontal_control1 = path_points[1].x;
        const auto horizontal_control2 = path_points[2].x;
        const auto horizontal_end = path_points[3].x;

        const auto vertical_start = path_points[0].y;
        const auto vertical_control1 = path_points[1].y;
        const auto vertical_control2 = path_points[2].y;
        const auto vertical_end = path_points[3].y;

        const auto has_horizontally_symmetric_control_points =
            horizontal_start - horizontal_control1 ==
            horizontal_control2 - horizontal_end;

        const auto has_vertically_symmetric_control_points =
            vertical_start - vertical_control1 ==
            vertical_control2 - vertical_end;

        const auto is_horizontally_mirrored =
            horizontal_start == horizontal_end &&
            horizontal_control1 == horizontal_control2 &&
            has_vertically_symmetric_control_points;

        const auto is_vertically_mirrored =
            vertical_start == vertical_end &&
            vertical_control1 == vertical_control2 &&
            has_horizontally_symmetric_control_points;

        if (is_horizontally_mirrored || is_vertically_mirrored) {
          // A bezier curve which is either vertically or horizontally mirrored
          // is flat at the center and has a negligible extension "backwards" at
          // the start and end points. The Graphviz arrow shape `curve`, which
          // is a cubic Bezier approximation of a semicircle, is a special case
          // of this. A vertical edge with a `curve` arrow head is vertically
          // mirrored and a horizontal edge with a `curve` arrow head is
          // horizonally mirrored.
          {
            const auto horizontal_endpoint_extension =
                is_vertically_mirrored ? attributes.stroke_width / 2 : 0;
            const auto vertical_endpoint_extension =
                is_horizontally_mirrored ? attributes.stroke_width / 2 : 0;
            const auto num_intermediate_control_points = 2;
            for (std::size_t i = 0; i < path_points.size();
                 i += num_intermediate_control_points + 1) {
              const auto &point = path_points[i];
              const SVG::SVGRect point_bbox = {
                  .x = point.x - horizontal_endpoint_extension,
                  .y = point.y - vertical_endpoint_extension,
                  .width = horizontal_endpoint_extension * 2,
                  .height = vertical_endpoint_extension * 2,
              };
              m_bbox->extend(point_bbox);
            }
            const SVGPoint midpoint = cubic_bezier(0.5);
            const SVG::SVGRect point_bbox = {
                .x = midpoint.x - attributes.stroke_width / 2,
                .y = midpoint.y - attributes.stroke_width / 2,
                .width = attributes.stroke_width,
                .height = attributes.stroke_width,
            };
            m_bbox->extend(point_bbox);
          }
          break;
        }
        const auto xmin = std::min(horizontal_start, horizontal_end);
        const auto xmax = std::max(horizontal_start, horizontal_end);
        const auto ymin = std::min(vertical_start, vertical_end);
        const auto ymax = std::max(vertical_start, vertical_end);
        if (horizontal_control1 >= xmin && horizontal_control1 <= xmax &&
            horizontal_control2 >= xmin && horizontal_control2 <= xmax &&
            vertical_control1 >= ymin && vertical_control1 <= ymax &&
            vertical_control2 >= ymin && vertical_control2 <= ymax) {
          // the control points are within a bounding box defined by the start
          // and end points. This means that the Bezier curve is completely
          // contained within this box, except for the extension caused by the
          // stroke width. This extension can be calculated using the angle at
          // the start and end points which is given by the closest control
          // point. The Graphviz arrow shapes `rcurve`, `lcurve`, `ricurve` and
          // `licurve` fulfills this condition.
          const auto start_dx = horizontal_start - horizontal_control1;
          const auto start_dy = vertical_start - vertical_control1;
          const auto start_hypot = std::hypot(start_dx, start_dy);
          const auto start_cos = start_dx / start_hypot;
          const auto start_sin = start_dy / start_hypot;
          const SVG::SVGPoint start_extension_left = {
              horizontal_start + attributes.stroke_width / 2 * start_sin,
              vertical_start - attributes.stroke_width / 2 * start_cos};
          const SVG::SVGPoint start_extension_right = {
              horizontal_start - attributes.stroke_width / 2 * start_sin,
              vertical_start + attributes.stroke_width / 2 * start_cos};
          m_bbox->extend(start_extension_left);
          m_bbox->extend(start_extension_right);
          const auto end_dx = horizontal_end - horizontal_control2;
          const auto end_dy = vertical_end - vertical_control2;
          const auto end_hypot = std::hypot(end_dx, end_dy);
          const auto end_cos = end_dx / end_hypot;
          const auto end_sin = end_dy / end_hypot;
          const SVG::SVGPoint end_extension_left = {
              horizontal_end + attributes.stroke_width / 2 * end_sin,
              vertical_end + -attributes.stroke_width / 2 * end_cos};
          const SVG::SVGPoint end_extension_right = {
              horizontal_end + -attributes.stroke_width / 2 * end_sin,
              vertical_end + attributes.stroke_width / 2 * end_cos};
          m_bbox->extend(end_extension_left);
          m_bbox->extend(end_extension_right);
          break;
        }
      }
      throw std::runtime_error(
          "paths other than straight vertical, straight horizontal or the "
          "cylinder special case are currently not supported");
    }
    // we now know we have a straight horizontal or vertical line (or the
    // degenerate case of a point)
    if (is_vertical) {
      const SVG::SVGRect first_point_bbox = {
          first_point.x - attributes.stroke_width / 2, first_point.y,
          attributes.stroke_width, 0};
      m_bbox->extend(first_point_bbox);
      for (const auto &point : path_points) {
        m_bbox->extend(point);
      }
    }
    if (is_horizontal) {
      for (const auto &point : path_points) {
        m_bbox->extend(point);
      }
      const SVG::SVGRect first_point_bbox = {
          first_point.x, first_point.y - attributes.stroke_width / 2, 0,
          attributes.stroke_width};
      m_bbox->extend(first_point_bbox);
    }
    break;
  }
  case SVG::SVGElementType::Polyline: {
    const auto &points = attributes.points;
    if (points.size() < 2) {
      throw std::runtime_error{"Too few points for 'polyline' element"};
    }

    // handle first point which may not be part of a corner
    {
      const auto first_point = points.front();
      const auto next_point = points[1];
      const auto dx = first_point.x - next_point.x;
      const auto dy = first_point.y - next_point.y;
      const auto hypot = std::hypot(dx, dy);
      const auto cosAlpha = dx / hypot;
      const auto sinAlpha = dy / hypot;
      // a polyline extends with half the stroke width in both perpendicular
      // directions, but not along the line at its endpoints
      const auto x_extension = std::abs(attributes.stroke_width / 2 * sinAlpha);
      const auto y_extension = std::abs(attributes.stroke_width / 2 * cosAlpha);
      const SVG::SVGRect first_point_bbox = {points.front().x - x_extension,
                                             points.front().y - y_extension,
                                             x_extension * 2, y_extension * 2};
      m_bbox->extend(first_point_bbox);
    }

    // handle last point which may not be part of a corner
    {
      const auto last_point = points.back();
      const auto prev_point = *(points.cend() - 2);
      const auto dx = last_point.x - prev_point.x;
      const auto dy = last_point.y - prev_point.y;
      const auto hypot = std::hypot(dx, dy);
      const auto cosAlpha = dx / hypot;
      const auto sinAlpha = dy / hypot;
      // a polyline extends with half the stroke width in both perpendicular
      // directions, but not along the line at its endpoints
      const auto x_extension = std::abs(attributes.stroke_width / 2 * sinAlpha);
      const auto y_extension = std::abs(attributes.stroke_width / 2 * cosAlpha);
      const SVG::SVGRect last_point_bbox = {points.back().x - x_extension,
                                            points.back().y - y_extension,
                                            x_extension * 2, y_extension * 2};
      m_bbox->extend(last_point_bbox);
    }

    if (points.size() >= 3) {
      // at least one corner
      if (has_all_points_equal()) {
        // the polyline has no size and will not be visible, so just arbitrarily
        // select one of its (equal) points as its outline bounding box
        const auto first_point = points.at(0);
        m_bbox->extend(first_point);
        break;
      }

      const auto clockwise = has_clockwise_points();
      for (auto it = points.cbegin() + 1; it < points.cend() - 1; ++it) {
        // there is always a previous point since we iterate from the second
        // point
        const auto &prev_point = *std::prev(it);
        const auto &point = *it;
        // there is always a next point since we iterate only to the next to
        // last point
        const auto &next_point = *std::next(it);
        SVG::SVGPoints miter_shape =
            // Graphviz draws some polylines clockwise and some
            // counter-clockwise.
            clockwise ? SVGElement::miter_shape(prev_point, point, next_point) :
                      // `miter_point` assumes clockwise so we swap the points
                SVGElement::miter_shape(next_point, point, prev_point);
        for (const auto &point : miter_shape) {
          m_bbox->extend(point);
        }
      }
    }
    break;
  }
  case SVG::SVGElementType::Rect:
    m_bbox = {
        .x = attributes.x - attributes.stroke_width / 2,
        .y = attributes.y - attributes.stroke_width / 2,
        .width = attributes.width + attributes.stroke_width,
        .height = attributes.height + attributes.stroke_width,
    };
    break;
  case SVG::SVGElementType::Text:
    m_bbox = text_bbox();
    break;
  case SVG::SVGElementType::Title:
    // title has no size
    if (throw_if_bbox_not_defined) {
      throw std::runtime_error{"A 'title' element has no bounding box"};
    }
    break;
  default:
    throw std::runtime_error{
        fmt::format("Unhandled svg element type {}", tag(type))};
  }

  const auto throw_if_child_bbox_is_not_defined = false;
  for (auto &child : children) {
    const auto child_bbox =
        child.outline_bbox(throw_if_child_bbox_is_not_defined);
    m_bbox->extend(child_bbox);
  }

  return *m_bbox;
}

SVG::SVGElement &SVG::SVGElement::find_child(const SVG::SVGElementType type,
                                             std::size_t index) {
  std::size_t i = 0;
  for (auto &child : children) {
    if (child.type == type) {
      if (i == index) {
        return child;
      }
      ++i;
    }
  }
  throw std::runtime_error(
      fmt::format("SVG element only has {} \"{}\" children, index {} not found",
                  i, tag(type), index));
}

SVG::SVGRect SVG::SVGElement::text_bbox() const {
  assert(type == SVG::SVGElementType::Text && "Not a 'text' element");

  if (attributes.font_family != "Courier,monospace") {
    throw std::runtime_error(
        fmt::format("Cannot calculate bounding box for font \"{}\"",
                    attributes.font_family));
  }

  // Epirically determined font metrics for the Courier font
  const auto courier_width_per_pt = 0.6;
  const auto courier_height_per_pt = 1.2;
  const auto descent_per_pt = 0.257;
  const auto font_width = attributes.font_size * courier_width_per_pt;
  const auto font_height = attributes.font_size * courier_height_per_pt;
  const auto descent = attributes.font_size * descent_per_pt;

  const SVG::SVGRect bbox = {
      .x = attributes.x - font_width * text.size() / 2,
      .y = attributes.y - font_height + descent,
      .width = font_width * text.size(),
      .height = font_height,
  };
  return bbox;
}

void SVG::SVGElement::append_attribute(std::string &output,
                                       const std::string &attribute) const {
  if (attribute.empty()) {
    return;
  }
  if (!output.empty()) {
    output += " ";
  }
  output += attribute;
}

static double interpolate(double y0, double y1, double x) {
  double dy = y1 - y0;

  return y0 + x * dy;
}

// The cubic Bezier implementation is taken from
// https://stackoverflow.com/a/37642695/3122101
SVG::SVGPoint SVG::SVGElement::cubic_bezier(double t) {
  assert(t >= 0);
  assert(t <= 1);
  assert(path_points.size() == 4);

  const auto x0 = path_points[0].x;
  const auto y0 = path_points[0].y;
  const auto x1 = path_points[1].x;
  const auto y1 = path_points[1].y;
  const auto x2 = path_points[2].x;
  const auto y2 = path_points[2].y;
  const auto x3 = path_points[3].x;
  const auto y3 = path_points[3].y;

  const auto xa = interpolate(x0, x1, t);
  const auto ya = interpolate(y0, y1, t);
  const auto xb = interpolate(x1, x2, t);
  const auto yb = interpolate(y1, y2, t);
  const auto xc = interpolate(x2, x3, t);
  const auto yc = interpolate(y2, y3, t);

  const auto xm = interpolate(xa, xb, t);
  const auto ym = interpolate(ya, yb, t);
  const auto xn = interpolate(xb, xc, t);
  const auto yn = interpolate(yb, yc, t);

  const auto x = interpolate(xm, xn, t);
  const auto y = interpolate(ym, yn, t);

  return {x, y};
}

bool SVG::SVGElement::has_all_points_equal() const {
  assert((type == SVG::SVGElementType::Polygon ||
          type == SVG::SVGElementType::Polyline) &&
         "not a polygon or polyline");
  const auto points = attributes.points;
  assert(!points.empty() > 0 && "no points");
  return std::equal(points.begin() + 1, points.end(), points.begin());
}

bool SVG::SVGElement::has_clockwise_points() const {
  assert((type == SVG::SVGElementType::Polygon ||
          type == SVG::SVGElementType::Polyline) &&
         "not a polygon or polyline");
  assert(attributes.points.size() >= 3 && "too few points");
  // Sum over the edges, (x2 − x1)(y2 + y1). If the result is positive, the
  // curve is clockwise, if it's negative the curve is counter-clockwise.
  // Implementation is based on https://stackoverflow.com/a/1165943/3122101
  const auto &points = attributes.points;
  double sum = 0;
  for (auto it = points.cbegin(); it < points.cend() - 1; ++it) {
    const auto &[x1, y1i] = *it;
    const auto &[x2, y2i] = *std::next(it);
    // SVG uses inverted y axis, so negate y values
    const auto y1 = -y1i;
    const auto y2 = -y2i;
    sum += (x2 - x1) * (y2 + y1);
  }
  return sum > 0;
}

std::string SVG::SVGElement::fill_attribute_to_string() const {
  if (attributes.fill.empty()) {
    return "";
  }

  return fmt::format(R"(fill="{}")", to_graphviz_color(attributes.fill));
}

std::string SVG::SVGElement::id_attribute_to_string() const {
  if (attributes.id.empty()) {
    return "";
  }

  return fmt::format(R"(id="{}")", attributes.id);
}

std::string SVG::SVGElement::fill_opacity_attribute_to_string() const {
  if (attributes.fill_opacity == 1) {
    // Graphviz doesn't set `fill-opacity` to 1 since that's the default
    return "";
  }

  if (attributes.fill_opacity == 0) {
    // Graphviz doesn't set `fill-opacity` to 0 since in that case it sets
    // `fill` to "none" instead
    return "";
  }

  return fmt::format(R"(fill-opacity="{}")", attributes.fill_opacity);
}

std::string SVG::SVGElement::points_attribute_to_string() const {
  std::string points_attribute_str = R"|(points=")|";
  const char *separator = "";
  for (const auto &point : attributes.points) {
    points_attribute_str += separator + fmt::format("{},{}", point.x, point.y);
    separator = " ";
  }
  points_attribute_str += '"';

  return points_attribute_str;
}

std::string SVG::SVGElement::stroke_attribute_to_string() const {
  if (attributes.stroke.empty()) {
    return "";
  }

  return fmt::format(R"(stroke="{}")",
                     stroke_to_graphviz_color(attributes.stroke));
}

std::string SVG::SVGElement::stroke_opacity_attribute_to_string() const {
  if (attributes.stroke_opacity == 1) {
    // Graphviz doesn't set `stroke-opacity` to 1 since that's the default
    return "";
  }

  if (attributes.stroke_opacity == 0) {
    // Graphviz doesn't set `stroke-opacity` to 0 since in that case it sets
    // `stroke` to "none" instead
    return "";
  }

  return fmt::format(R"(stroke-opacity="{}")", attributes.stroke_opacity);
}

std::string SVG::SVGElement::stroke_width_attribute_to_string() const {
  if (attributes.stroke_width == 1) {
    // Graphviz doesn't set `stroke-width` to 1 since that's the default
    return "";
  }

  return fmt::format(R"(stroke-width="{}")", attributes.stroke_width);
}

std::string SVG::SVGElement::to_string(std::size_t indent_size = 2) const {
  std::string output;
  output += R"(<?xml version="1.0" encoding="UTF-8" standalone="no"?>)"
            "\n";
  output += R"(<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN")"
            "\n";
  output += R"( "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">)"
            "\n";
  output += fmt::format("<!-- Generated by graphviz version {} "
                        "({})\n -->\n",
                        graphviz_version, graphviz_build_date);
  to_string_impl(output, indent_size, 0);
  return output;
}

void SVG::SVGElement::to_string_impl(std::string &output,
                                     std::size_t indent_size,
                                     std::size_t current_indent) const {
  const auto indent_str = std::string(current_indent, ' ');
  output += indent_str;

  if (type == SVG::SVGElementType::Svg) {
    const auto comment = fmt::format("Title: {} Pages: 1", graphviz_id);
    output += fmt::format("<!-- {} -->\n", xml_encode(comment));
  }
  if (type == SVG::SVGElementType::Group &&
      (attributes.class_ == "node" || attributes.class_ == "edge")) {
    const auto comment = graphviz_id;
    output += fmt::format("<!-- {} -->\n", xml_encode(comment));
  }

  output += "<";
  output += tag(type);

  std::string attributes_str{};
  append_attribute(attributes_str, id_attribute_to_string());
  switch (type) {
  case SVG::SVGElementType::Ellipse:
    append_attribute(attributes_str, fill_attribute_to_string());
    append_attribute(attributes_str, fill_opacity_attribute_to_string());
    append_attribute(attributes_str, stroke_attribute_to_string());
    append_attribute(attributes_str, stroke_width_attribute_to_string());
    append_attribute(attributes_str, stroke_opacity_attribute_to_string());
    attributes_str +=
        fmt::format(R"( cx="{}" cy="{}" rx="{}" ry="{}")", attributes.cx,
                    attributes.cy, attributes.rx, attributes.ry);
    break;
  case SVG::SVGElementType::Group:
    attributes_str += fmt::format(R"( class="{}")", attributes.class_);
    if (attributes.transform.has_value()) {
      const auto transform = attributes.transform;
      attributes_str += fmt::format(
          R"|( transform="scale({} {}) rotate({}) translate({} {})")|",
          transform->a, transform->d, transform->c, transform->e, transform->f);
    }
    break;
  case SVG::SVGElementType::Path: {
    append_attribute(attributes_str, fill_attribute_to_string());
    append_attribute(attributes_str, fill_opacity_attribute_to_string());
    append_attribute(attributes_str, stroke_attribute_to_string());
    append_attribute(attributes_str, stroke_width_attribute_to_string());
    append_attribute(attributes_str, stroke_opacity_attribute_to_string());
    attributes_str += R"|( d=")|";
    auto command = 'M';
    for (const auto &point : path_points) {
      attributes_str += fmt::format("{}{},{}", command, point.x, point.y);
      switch (command) {
      case 'M':
        command = 'C';
        break;
      case 'C':
        command = ' ';
        break;
      case ' ':
        break;
      default:
        UNREACHABLE();
      }
    }
    attributes_str += '"';
    break;
  }
  case SVG::SVGElementType::Polygon:
    append_attribute(attributes_str, fill_attribute_to_string());
    append_attribute(attributes_str, fill_opacity_attribute_to_string());
    append_attribute(attributes_str, stroke_attribute_to_string());
    append_attribute(attributes_str, stroke_width_attribute_to_string());
    append_attribute(attributes_str, stroke_opacity_attribute_to_string());
    append_attribute(attributes_str, points_attribute_to_string());
    break;
  case SVG::SVGElementType::Polyline:
    append_attribute(attributes_str, fill_attribute_to_string());
    append_attribute(attributes_str, stroke_attribute_to_string());
    append_attribute(attributes_str, stroke_width_attribute_to_string());
    append_attribute(attributes_str, stroke_opacity_attribute_to_string());
    append_attribute(attributes_str, points_attribute_to_string());
    break;
  case SVG::SVGElementType::Rect:
    attributes_str +=
        fmt::format(R"(x="{}" y="{}" width="{}" height="{}")", attributes.x,
                    attributes.y, attributes.width, attributes.height);
    append_attribute(attributes_str, fill_attribute_to_string());
    append_attribute(attributes_str, stroke_attribute_to_string());
    append_attribute(attributes_str, stroke_width_attribute_to_string());
    append_attribute(attributes_str, stroke_opacity_attribute_to_string());
    break;
  case SVG::SVGElementType::Svg:
    attributes_str += fmt::format(
        R"(width="{}pt" height="{}pt")"
        "\n"
        R"( viewBox="{:.2f} {:.2f} {:.2f} {:.2f}" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink")",
        std::lround(px_to_pt(attributes.width)),
        std::lround(px_to_pt(attributes.height)), attributes.viewBox.x,
        attributes.viewBox.y, attributes.viewBox.width,
        attributes.viewBox.height);
    break;
  case SVG::SVGElementType::Text:
    attributes_str += fmt::format(
        R"(text-anchor="{}" x="{}" y="{}" font-family="{}" font-size="{:.2f}")",
        attributes.text_anchor, attributes.x, attributes.y,
        attributes.font_family, attributes.font_size);
    break;
  case SVG::SVGElementType::Title:
    // Graphviz doesn't generate attributes on 'title' elements
    break;
  default:
    throw std::runtime_error{fmt::format(
        "Attributes on '{}' elements are not yet implemented", tag(type))};
  }
  if (!attributes_str.empty()) {
    output += " ";
  }
  output += attributes_str;

  if (children.empty() && text.empty()) {
    output += "/>\n";
  } else {
    output += ">";
    if (!text.empty()) {
      output += xml_encode(text);
    }
    if (!children.empty()) {
      output += "\n";
      for (const auto &child : children) {
        child.to_string_impl(output, indent_size, current_indent + indent_size);
      }
      output += indent_str;
    }
    output += "</";
    output += tag(type);
    output += ">\n";
  }
}

SVG::SVGPoints
SVG::SVGElement::miter_shape(SVG::SVGPoint segment_start,
                             SVG::SVGPoint segment_end,
                             SVG::SVGPoint following_segment_end) const {
  /*
   * Compute the stroke shape miter shape according to
   * https://www.w3.org/TR/SVG2/painting.html#StrokeShape.
   *
   * The spec assumes the points of a shape are given in clockwise
   * (mathematically negative) direction which is how Graphviz draws the points
   * of an arrow head. A standard arrow head looks like this:
   *
   *             1
   *             ^
   *            /◡\
   *           / θ \
   *          /     \
   *         /       \
   *        /         \
   *      0 ----------- 2
   *     (3)     |
   *             |
   *             |
   *             |
   *
   *
   * NOTE: Graphviz draws node shapes in the opposite direction, i.e., in
   * counter-clockwise (mathematically positive) direction which means that such
   * points must be reordered before calling this method.
   *
   * See https://www.w3.org/TR/SVG2/painting.html#TermLineJoinShape for how the
   * terminating line join shape should be calculated. Below is an attempt to
   * copy the diagram from the spec with some details added
   *
   *
   *                   P3
   *                   /\
   *                  .  .
   *              l  .    .
   *                .      .
   *               .        .
   *            P1           . P2
   *             /˙·.ٜ  P ٜ .·˙ \
   *            /      /\      \
   *           /      /◟◞\      \
   *          /      / θ  \      \
   *         /      /      \      \
   *  Aleft /    A /        \ B    \ Bleft
   *       /      /          \      \
   *      /      /            \      \
   *     /      / α    /\      \      \  β-π
   * .../....../◝...../..\.....◜ ◝.....\◝.....
   *   /      /      /    \    ◟ \      \
   *         /      /      \   β  \
   *               /        \
   *            Aright     Bright
   *
   * A is the current segment that ends in P.
   * B is the following segment that starts in P.
   *
   * θ is the angle between the A segment and the B segment in the reverse
   * direction
   *
   * α is the angle of the A segment to the x-axis.
   *
   * β is the angle of the B segment to the x-axis.
   *     NOTE: In the diagram above, the B segment crosses the x-axis in the
   *           downwards direction so its angle to the x-axis is in this case
   *           larger than a semi-circle (π or 180°). In the picture it is
   *           around 5π/3 or 300°. The B segement in the opposite direction has
   *           an angle to the x-axis which is β-π. This is denoted next to the
   *           Bleft line in the picture.
   *
   * π is the number pi ≃ 3.14
   *
   * l is the calculated length between P1 and P3.
   *
   * The distance between P and P1 and between P and P2 is stroke-width / 2.
   *
   * NOTE: This method only implements the 'miter' join, but falls back to
   * 'bevel' when stroke-miterlimit is exceeded.
   */

  if (segment_start == segment_end || segment_end == following_segment_end) {
    // the stroke shape is really a point so we just return this point without
    // extending it with stroke width in any direction, which seems to be the
    // way SVG renderers render this.
    return {segment_end};
  }

  const auto stroke_width = attributes.stroke_width;

  // SVG has inverted y axis so invert all y values before use
  const SVG::SVGPoint P = {segment_end.x, -segment_end.y};
  const SVG::SVGLine A = {segment_start.x, -segment_start.y, segment_end.x,
                          -segment_end.y};
  const SVG::SVGLine B = {segment_end.x, -segment_end.y,
                          following_segment_end.x, -following_segment_end.y};

  const auto dxA = A.x2 - A.x1;
  const auto dyA = A.y2 - A.y1;
  const auto hypotA = std::hypot(dxA, dyA);
  const auto cosAlpha = dxA / hypotA;
  const auto sinAlpha = dyA / hypotA;
  const auto alpha = dyA > 0 ? std::acos(cosAlpha) : -std::acos(cosAlpha);

  const SVG::SVGPoint P1 = {P.x - stroke_width / 2.0 * sinAlpha,
                            P.y + stroke_width / 2.0 * cosAlpha};

  const auto dxB = B.x2 - B.x1;
  const auto dyB = B.y2 - B.y1;
  const auto hypotB = std::hypot(dxB, dyB);
  const auto cosBeta = dxB / hypotB;
  const auto beta = dyB > 0 ? std::acos(cosBeta) : -std::acos(cosBeta);

  const auto sinBeta = dyB / hypotB;
  const auto sinBetaMinusPi = -sinBeta;
  const auto cosBetaMinusPi = -cosBeta;
  const SVG::SVGPoint P2 = {P.x + stroke_width / 2.0 * sinBetaMinusPi,
                            P.y - stroke_width / 2.0 * cosBetaMinusPi};

  // angle between the A segment and the B segment in the reverse direction
  const auto beta_rev = beta - std::numbers::pi;
  const auto theta = beta_rev - alpha;

  SVG::SVGPoints line_join_shape;
  const auto miter_limit = 4.0;
  const auto miter_length = stroke_width / std::sin(std::abs(theta) / 2.0);

  // add the bevel, which is is the triangle formed from the three points P, P1
  // and P2, to the line join shape. SVG has inverted y axis so invert the
  // returned y value
  line_join_shape.emplace_back(P.x, -P.y);
  line_join_shape.emplace_back(P1.x, -P1.y);
  line_join_shape.emplace_back(P2.x, -P2.y);

  if (miter_length > miter_limit * stroke_width) {
    // fall back to bevel only
    return line_join_shape;
  }

  // length between P1 and P3 (and between P2 and P3)
  const auto l = stroke_width / 2.0 / std::tan(theta / 2.0);

  const SVG::SVGPoint P3 = {P1.x + l * cosAlpha, P1.y + l * sinAlpha};

  // SVG has inverted y axis so invert the returned y value
  line_join_shape.emplace_back(P3.x, -P3.y);
  return line_join_shape;
}

std::string
SVG::SVGElement::stroke_to_graphviz_color(const std::string &color) const {
  return to_graphviz_color(color);
}

void SVG::SVGRect::extend(const SVGPoint &point) {
  const auto xmin = std::min(x, point.x);
  const auto ymin = std::min(y, point.y);
  const auto xmax = std::max(x + width, point.x);
  const auto ymax = std::max(y + height, point.y);

  x = xmin;
  y = ymin;
  width = xmax - xmin;
  height = ymax - ymin;
}

SVG::SVGPoint SVG::SVGRect::center() const {
  return {x + width / 2, y + height / 2};
}

SVG::SVGRect SVG::SVGRect::intersection(SVG::SVGRect other) const {
  const SVG::SVGLine intersection_diagonal = {
      std::max(x, other.x), std::max(y, other.y),
      std::min(x + width, other.x + other.width),
      std::min(y + height, other.y + other.height)};

  return SVG::SVGRect{
      .x = intersection_diagonal.x1,
      .y = intersection_diagonal.y1,
      .width = intersection_diagonal.x2 - intersection_diagonal.x1,
      .height = intersection_diagonal.y2 - intersection_diagonal.y1};
}

void SVG::SVGRect::extend(const SVG::SVGRect &other) {
  const auto xmin = std::min(x, other.x);
  const auto ymin = std::min(y, other.y);
  const auto xmax = std::max(x + width, other.x + other.width);
  const auto ymax = std::max(y + height, other.y + other.height);

  x = xmin;
  y = ymin;
  width = xmax - xmin;
  height = ymax - ymin;
}

std::string_view SVG::tag(SVGElementType type) {
  switch (type) {
  case SVG::SVGElementType::Circle:
    return "circle";
  case SVG::SVGElementType::Ellipse:
    return "ellipse";
  case SVG::SVGElementType::Group:
    return "g";
  case SVG::SVGElementType::Line:
    return "line";
  case SVG::SVGElementType::Path:
    return "path";
  case SVG::SVGElementType::Polygon:
    return "polygon";
  case SVG::SVGElementType::Polyline:
    return "polyline";
  case SVG::SVGElementType::Rect:
    return "rect";
  case SVG::SVGElementType::Svg:
    return "svg";
  case SVG::SVGElementType::Text:
    return "text";
  case SVG::SVGElementType::Title:
    return "title";
  }
  UNREACHABLE();
}

bool SVG::SVGPoint::operator==(const SVGPoint &rhs) const {
  return x == rhs.x && y == rhs.y;
}

bool SVG::SVGPoint::operator!=(const SVGPoint &rhs) const {
  return !(*this == rhs);
}

bool SVG::SVGPoint::is_higher_than(const SVGPoint &other) const {
  // SVG uses inverted y axis, so smaller is higher
  return y < other.y;
}

bool SVG::SVGPoint::is_lower_than(const SVGPoint &other) const {
  // SVG uses inverted y axis, so larger is lower
  return y > other.y;
}

bool SVG::SVGPoint::is_more_left_than(const SVGPoint &other) const {
  return x < other.x;
}

bool SVG::SVGPoint::is_more_right_than(const SVGPoint &other) const {
  return x > other.x;
}
