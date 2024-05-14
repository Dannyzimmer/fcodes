#include <string>
#include <string_view>
#include <unordered_set>

#include <catch2/catch.hpp>
#include <cmath>
#include <fmt/format.h>
#include <unordered_map>

#include "svg_analyzer.h"
#include "test_edge_node_overlap_utilities.h"
#include "test_utilities.h"
#include <cgraph/unreachable.h>

/// return union of unordered sets of string views
std::unordered_set<std::string_view>
union_(const std::unordered_set<std::string_view> &a,
       const std::unordered_set<std::string_view> &b,
       const std::unordered_set<std::string_view> &c = {}) {
  std::unordered_set<std::string_view> ret = a;
  ret.insert(b.begin(), b.end());
  ret.insert(c.begin(), c.end());

  return ret;
}

static const std::unordered_set<std::string_view>
    shapes_not_meeting_edge_vertically = {
        "plaintext",       //
        "none",            //
        "promoter",        //
        "cds",             //
        "terminator",      //
        "utr",             //
        "primersite",      //
        "restrictionsite", //
        "fivepoverhang",   //
        "threepoverhang",  //
        "noverhang",       //
        "assembly",        //
        "signature",       //
        "insulator",       //
        "ribosite",        //
        "rnastab",         //
        "proteasesite",    //
        "proteinstab",     //
};

static const std::unordered_set<std::string_view>
    shapes_not_meeting_edge_horizontally = {
        "plaintext", // has space around the label as if it was a box shape
        "none",      // has space around the label as if it was a box shape
};

static const std::unordered_set<std::string_view> &
shapes_not_meeting_edge(const std::string_view rankdir) {
  if (rankdir == "TB" || rankdir == "BT") {
    return shapes_not_meeting_edge_vertically;
  } else if (rankdir == "LR" || rankdir == "RL") {
    return shapes_not_meeting_edge_horizontally;
  }
  UNREACHABLE();
}

static const std::unordered_set<std::string_view> shapes_with_concave_top = {
    "folder", "tab", "promoter", "rpromoter", "rarrow", "larrow", "lpromoter",
};

static const std::unordered_set<std::string_view> shapes_with_concave_bottom = {
    "star", "rpromoter", "rarrow", "larrow", "lpromoter"};

static const std::unordered_set<std::string_view> shapes_with_concave_left = {
    "component"};

static const std::unordered_set<std::string_view>
    shapes_with_left_extreme_not_centered = {
        "egg",           "triangle", "invtriangle", "trapezium", "invtrapezium",
        "parallelogram", "pentagon", "septagon",    "star"};

static const std::unordered_set<std::string_view>
    shapes_with_right_extreme_not_centered = {
        "egg",           "triangle", "invtriangle", "trapezium", "invtrapezium",
        "parallelogram", "pentagon", "septagon",    "star"};

static const std::unordered_set<std::string_view>
    shapes_with_invisible_descent = {"plain"};

static const std::unordered_set<std::string_view>
    shapes_with_invisible_left_extension = {"plain"};

static const std::unordered_set<std::string_view>
    shapes_with_invisible_right_extension = {"plain"};

static const std::unordered_set<std::string_view>
    shapes_not_to_check_for_overlap_at_top = shapes_with_concave_top;

static const std::unordered_set<std::string_view>
    shapes_not_to_check_for_overlap_at_bottom =
        union_(shapes_with_concave_bottom, shapes_with_invisible_descent);

static const std::unordered_set<std::string_view>
    shapes_not_to_check_for_overlap_at_left_side =
        union_(shapes_with_left_extreme_not_centered, shapes_with_concave_left,
               shapes_with_invisible_left_extension);

static const std::unordered_set<std::string_view>
    shapes_not_to_check_for_overlap_at_right_side =
        union_(shapes_with_right_extreme_not_centered,
               shapes_with_invisible_right_extension);

static const std::unordered_set<std::string_view> &
shapes_not_to_check_for_max_overlap_at_edge_head(
    const std::string_view rankdir) {
  if (rankdir == "TB") {
    return shapes_not_to_check_for_overlap_at_top;
  } else if (rankdir == "BT") {
    return shapes_not_to_check_for_overlap_at_bottom;
  } else if (rankdir == "LR") {
    return shapes_not_to_check_for_overlap_at_left_side;
  } else if (rankdir == "RL") {
    return shapes_not_to_check_for_overlap_at_right_side;
  }
  UNREACHABLE();
}

const std::unordered_set<std::string_view> &
shapes_not_to_check_for_max_overlap_at_edge_tail(
    const std::string_view rankdir) {
  if (rankdir == "TB") {
    return shapes_not_to_check_for_overlap_at_bottom;
  } else if (rankdir == "BT") {
    return shapes_not_to_check_for_overlap_at_top;
  } else if (rankdir == "LR") {
    return shapes_not_to_check_for_overlap_at_right_side;
  } else if (rankdir == "RL") {
    return shapes_not_to_check_for_overlap_at_left_side;
  }
  UNREACHABLE();
}

/// return the overlap in the rank direction from an intersection rectangle
static double overlap_in_rank_direction(SVG::SVGRect intersection,
                                        const std::string_view rankdir) {
  if (rankdir == "LR" || rankdir == "RL") {
    return intersection.width;
  }
  if (rankdir == "TB" || rankdir == "BT") {
    return intersection.height;
  }
  UNREACHABLE();
}

static bool skip_max_check_at_head_node(std::string_view rankdir,
                                        std::string_view node_shape) {
  return shapes_not_to_check_for_max_overlap_at_edge_head(rankdir).contains(
      node_shape);
}

static bool skip_max_check_at_tail_node(std::string_view rankdir,
                                        std::string_view node_shape) {
  return shapes_not_to_check_for_max_overlap_at_edge_tail(rankdir).contains(
      node_shape);
}

static bool skip_min_check_at_head_node(std::string_view rankdir,
                                        std::string_view node_shape) {
  return shapes_not_meeting_edge(rankdir).contains(node_shape);
}

static bool skip_min_check_at_tail_node(std::string_view rankdir,
                                        std::string_view node_shape) {
  return shapes_not_meeting_edge(rankdir).contains(node_shape);
}

/// check overlap between the edge and the nodes and between the edge stem and
/// the edge arrows
static bool check_analyzed_svg(SVGAnalyzer &svg_analyzer,
                               const graph_options &graph_options,
                               const check_options &check_options) {

  const auto rankdir = graph_options.rankdir;
  const auto node_shape = graph_options.node_shape;
  const auto dir = graph_options.dir;
  const auto primitive_arrowhead_shape =
      graph_options.primitive_arrowhead_shape;
  const auto primitive_arrowtail_shape =
      graph_options.primitive_arrowtail_shape;

  REQUIRE(svg_analyzer.graphs().size() == 1);
  auto &recreated_graph = svg_analyzer.graphs().back();

  auto &tail_node = recreated_graph.node("a");
  auto &head_node = recreated_graph.node("b");
  auto &edge = recreated_graph.edge("a->b");

  auto success = true;

// macro for doing the actual check and continue the execution in the same test
// case even if the assertion fails, while still capturing the result to be
// used to decide whether to write SVG files at the end of the test case
#define DO_CHECK(condition)                                                    \
  do {                                                                         \
    CHECK(condition);                                                          \
    success = success && (condition);                                          \
  } while (0)

  const auto edge_bbox = edge.outline_bbox();

  // check head node and edge overlap
  {
    const auto head_node_bbox = head_node.outline_bbox();
    const auto overlap_bbox = edge_bbox.intersection(head_node_bbox);
    INFO("Head node overlap:");
    INFO(fmt::format("  width:  {:.3f}", overlap_bbox.width));
    INFO(fmt::format("  height: {:.3f}", overlap_bbox.height));
    const auto head_node_edge_overlap =
        overlap_in_rank_direction(overlap_bbox, rankdir);

    // check maximum head node and edge overlap
    if (check_options.check_max_edge_node_overlap) {
      if (!skip_max_check_at_head_node(rankdir, node_shape)) {
        DO_CHECK(head_node_edge_overlap <=
                 check_options.max_node_edge_overlap +
                     check_options.svg_rounding_error * 2);
      }
    }

    // check minimum head node and edge overlap
    if (check_options.check_min_edge_node_overlap) {
      if (!skip_min_check_at_head_node(rankdir, node_shape)) {
        DO_CHECK(head_node_edge_overlap >=
                 check_options.min_node_edge_overlap -
                     check_options.svg_rounding_error * 2);
      }
    }
  }

  // check tail node and edge overlap
  {
    const auto tail_node_bbox = tail_node.outline_bbox();
    const auto overlap_bbox = edge_bbox.intersection(tail_node_bbox);
    INFO("Tail node overlap:");
    INFO(fmt::format("  width:  {:.6f}", overlap_bbox.width));
    INFO(fmt::format("  height: {:.6f}", overlap_bbox.height));
    const auto tail_node_edge_overlap =
        overlap_in_rank_direction(overlap_bbox, rankdir);

    // check maximum tail node and edge overlap
    if (check_options.check_max_edge_node_overlap) {
      if (!skip_max_check_at_tail_node(rankdir, node_shape)) {
        DO_CHECK(tail_node_edge_overlap <=
                 check_options.max_node_edge_overlap +
                     check_options.svg_rounding_error * 2);
      }
    }

    // check minimum overlap at edge tail
    if (check_options.check_min_edge_node_overlap) {
      if (!skip_min_check_at_tail_node(rankdir, node_shape)) {
        DO_CHECK(tail_node_edge_overlap >=
                 check_options.min_node_edge_overlap -
                     check_options.svg_rounding_error * 2);
      }
    }
  }

  auto &edge_stem = edge.stem();
  const auto edge_stem_bbox = edge_stem.outline_bbox();

  // check overlap of edge stem and arrowhead
  if ((dir == "forward" || dir == "both") &&
      primitive_arrowhead_shape != "none") {
    const auto edge_arrowhead_bbox =
        edge.arrowhead_outline_bbox(dir, primitive_arrowhead_shape);
    const auto overlap_bbox = edge_stem_bbox.intersection(edge_arrowhead_bbox);
    INFO("Edge stem and arrowhead overlap:");
    INFO(fmt::format("  width:  {:.3f}", overlap_bbox.width));
    INFO(fmt::format("  height: {:.3f}", overlap_bbox.height));
    const auto edge_stem_arrowhead_overlap =
        overlap_in_rank_direction(overlap_bbox, rankdir);

    // check maximum overlap of edge stem and arrowhead
    if (check_options.check_max_edge_stem_arrow_overlap) {
      const auto max_edge_stem_arrowhead_overlap =
          check_options.max_edge_stem_arrow_overlap.at(
              graph_options.primitive_arrowhead_shape);
      DO_CHECK(edge_stem_arrowhead_overlap <=
               max_edge_stem_arrowhead_overlap +
                   check_options.svg_rounding_error * 2);
    }

    // check minimum overlap of edge stem and arrowhead
    if (check_options.check_min_edge_stem_arrow_overlap) {
      const auto min_edge_stem_arrowhead_overlap =
          check_options.min_edge_stem_arrow_overlap;
      DO_CHECK(edge_stem_arrowhead_overlap >=
               min_edge_stem_arrowhead_overlap -
                   check_options.svg_rounding_error * 2);
    }
  }

  // check overlap of edge stem and arrowtail
  if ((dir == "back" || dir == "both") && primitive_arrowtail_shape != "none") {
    const auto edge_arrowtail_bbox =
        edge.arrowtail_outline_bbox(dir, primitive_arrowtail_shape);
    const auto overlap_bbox = edge_stem_bbox.intersection(edge_arrowtail_bbox);
    INFO("Edge stem and arrowtail overlap:");
    INFO(fmt::format("  width:  {:.3f}", overlap_bbox.width));
    INFO(fmt::format("  height: {:.3f}", overlap_bbox.height));
    const auto edge_stem_arrowtail_overlap =
        overlap_in_rank_direction(overlap_bbox, rankdir);

    // check maximum overlap of edge stem and arrowtail
    if (check_options.check_max_edge_stem_arrow_overlap) {
      const auto max_edge_stem_arrowtail_overlap =
          check_options.max_edge_stem_arrow_overlap.at(
              graph_options.primitive_arrowtail_shape);
      DO_CHECK(edge_stem_arrowtail_overlap <=
               max_edge_stem_arrowtail_overlap +
                   check_options.svg_rounding_error * 2);
    }

    // check minimum overlap of edge stem and arrowtail
    if (check_options.check_min_edge_stem_arrow_overlap) {
      const auto min_edge_stem_arrowtail_overlap =
          check_options.min_edge_stem_arrow_overlap;
      DO_CHECK(edge_stem_arrowtail_overlap >=
               min_edge_stem_arrowtail_overlap -
                   check_options.svg_rounding_error * 2);
    }
  }

  return success;
}

/// write SVG files for manual analysis if any of the above checks failed or if
/// we explicitly have requested it
static void write_svg_files(SVGAnalyzer &svg_analyzer,
                            const check_options &check_options,
                            const write_options &write_options) {
  const std::filesystem::path test_artifacts_directory = "test_artifacts";

  if (write_options.write_original_svg) {
    // write the original SVG generated by Graphviz to a file
    const std::filesystem::path filename =
        write_options.filename_base + "_original.svg";
    write_to_file(test_artifacts_directory, filename,
                  svg_analyzer.original_svg());
  }

  if (write_options.write_recreated_svg) {
    // write the SVG recreated by the SVG analyzer to a file
    const std::filesystem::path filename =
        write_options.filename_base + "_recreated.svg";
    const auto recreated_svg = svg_analyzer.svg_string();
    write_to_file(test_artifacts_directory, filename, recreated_svg);
  }

  if (write_options.write_annotated_svg) {
    // annotate the SVG recreated by the SVG analyzer with bounding boxes
    // and write to file
    svg_analyzer.add_bboxes();
    svg_analyzer.add_outline_bboxes();
    svg_analyzer.add_node_edge_outline_bbox_overlaps(
        check_options.max_node_edge_overlap);
    const std::filesystem::path filename =
        write_options.filename_base + "_annotated.svg";
    write_to_file(test_artifacts_directory, filename,
                  svg_analyzer.svg_string());
  }
}

const std::unordered_set<std::string_view>
    polygon_shapes_with_left_or_right_corner_unknown_during_layout = {
        // FIXME: These shapes are represented as a box during layout and do not
        // get their final shape until the graph is rendered. They have a corner
        // on their left or right side which the `poly_inside` function is
        // unaware of.
        // This causes a lager overlap since the outline of the box is half the
        // penwidth outside the nominal box, whereas the miter point of the
        // corner is further away.
        "cds",       // corner on right side, unknown during layout
        "rpromoter", // corner on right side, unknown during layout
        "rarrow",    // corner on right side, unknown during layout
        "lpromoter", // corner on left side, unknown during layout
        "larrow"     // corner on left side, unknown during layout

};

static bool has_corner_in_rank_direction_unknown_during_layout(
    const std::string_view shape, const std::string_view rankdir) {
  if (rankdir == "LR" || rankdir == "RL") {
    return polygon_shapes_with_left_or_right_corner_unknown_during_layout
        .contains(shape);
  }
  return false;
}

/// generate DOT source based on given options
static std::string generate_dot(const graph_options &graph_options) {
  // use a semi-transparent color to easily see overlaps
  const auto color = "\"#00000060\"";
  const auto arrowhead = fmt::format("{}{}", graph_options.arrowhead_modifier,
                                     graph_options.primitive_arrowhead_shape);
  const auto arrowtail = fmt::format("{}{}", graph_options.arrowtail_modifier,
                                     graph_options.primitive_arrowtail_shape);
  return fmt::format(
      "digraph g1 {{"
      "  graph [rankdir={}]"
      "  node [penwidth={} shape={} color={} fontname=Courier fontsize={}]"
      "  edge [penwidth={} color={} dir={} arrowhead={} "
      "arrowtail={} arrowsize={}]"
      "  a -> b"
      "}}",
      graph_options.rankdir, graph_options.node_penwidth,
      graph_options.node_shape, color, graph_options.node_fontsize,
      graph_options.edge_penwidth, color, graph_options.dir, arrowhead,
      arrowtail, graph_options.edge_arrowsize);
}

void test_edge_node_overlap(const graph_options &graph_options,
                            const tc_check_options &tc_check_options,
                            const write_options &write_options) {
  const auto dot = generate_dot(graph_options);

  auto svg_analyzer = SVGAnalyzer::make_from_dot(dot);

  // The binary search in the bezier_clip function in lib/common/splines.c has a
  // limit for when to consider the boundary found and to be the point inside
  // the boundary. It is the maximum distance between two points on a bezier
  // curve that are on opposite sides of the node boundary (for shape_clip) or
  // on the opposite sides of the boundary of a virtual circle at a specified
  // distance from a given point (for arrow_clip). An margin is needed to
  // account for the error that this limit introduces.
  const double graphviz_bezier_clip_margin = 0.5;
  const int graphviz_num_decimals_in_svg = 2;
  const double graphviz_max_svg_rounding_error =
      std::pow(10, -graphviz_num_decimals_in_svg) / 2;

  const std::unordered_map<std::string_view, double>
      max_edge_stem_arrow_overlap = {
          {"box",
           graph_options.edge_penwidth / 2 + graphviz_bezier_clip_margin},
          {"crow",
           [&]() { // FIXME: calculate this accurately for crow/vee arrow
             const auto tip_scale = 2.222222; // empirical value
             return graph_options.edge_penwidth / 2 * tip_scale +
                    graphviz_bezier_clip_margin;
           }()},
          {"curve",
           graph_options.edge_penwidth / 2 + graphviz_bezier_clip_margin},
          {"diamond",
           [&]() { // FIXME: calculate this accurately for diamond arrow
             const auto tip_scale = 1.8027756377319939; // empirical value
             return graph_options.edge_penwidth / 2 * tip_scale +
                    graphviz_bezier_clip_margin;
           }()},
          {"dot",
           graph_options.edge_penwidth / 2 + graphviz_bezier_clip_margin},
          {"icurve",
           graph_options.edge_penwidth / 2 + graphviz_bezier_clip_margin},
          {"inv",
           [&]() { // FIXME: calculate this accurately for normal/inv arrow
             const auto tip_scale = 2 * 1.5135442928; // empirical value
             return graph_options.edge_penwidth / 2 * tip_scale +
                    graphviz_bezier_clip_margin;
           }()},
          {"normal",
           graph_options.edge_penwidth / 2 + graphviz_bezier_clip_margin},
          {"tee",
           graph_options.edge_penwidth / 2 + graphviz_bezier_clip_margin},
          {"vee",
           graph_options.edge_penwidth / 2 + graphviz_bezier_clip_margin},
      };

  const auto extra_max_node_edge_overlap =
      has_corner_in_rank_direction_unknown_during_layout(
          graph_options.node_shape, graph_options.rankdir)
          // the corner angle is roughly 90 degrees which gives a miter point
          // penwidth / 2 * sqrt(2) from the nominal corner, i.e., penwidth / 2
          // * (sqrt(2) - 1) from the outline bounding box which is only what
          // the `poly_inside` function knows about during layout
          ? graph_options.node_penwidth / 2 * (std::sqrt(2) - 1)
          : 0;

  const auto extra_node_edge_overlap_margin =
      graph_options.node_shape == "plain"
          // The plain shape has no visible borders and its bounding box is
          // solely determined by the invisible bounding box of the text and it
          // may vary between text renderers. We therefore can, and need to,
          // allow more margin when checking for overlap.
          ? 0.7
          : 0;

  const check_options check_options = {
      .check_max_edge_node_overlap =
          tc_check_options.check_max_edge_node_overlap,
      .check_min_edge_node_overlap =
          tc_check_options.check_min_edge_node_overlap,
      .check_max_edge_stem_arrow_overlap =
          tc_check_options.check_max_edge_stem_arrow_overlap,
      .check_min_edge_stem_arrow_overlap =
          tc_check_options.check_min_edge_stem_arrow_overlap,
      .max_node_edge_overlap = graphviz_bezier_clip_margin +
                               extra_max_node_edge_overlap +
                               extra_node_edge_overlap_margin,
      .min_node_edge_overlap = 0 - extra_node_edge_overlap_margin,
      .max_edge_stem_arrow_overlap = max_edge_stem_arrow_overlap,
      .min_edge_stem_arrow_overlap = 0,
      .svg_rounding_error = graphviz_max_svg_rounding_error,
  };

  const auto success =
      check_analyzed_svg(svg_analyzer, graph_options, check_options);

  if (!success || write_options.write_svg_on_success) {
    write_svg_files(svg_analyzer, check_options, write_options);
  }
}
