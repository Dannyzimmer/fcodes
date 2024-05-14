#include <string>
#include <string_view>
#include <unordered_map>

#include "svg_analyzer.h"

/// check options that can be controlled from the test case
struct tc_check_options {
  /// whether to check that there is not too much overlap
  bool check_max_edge_node_overlap = true;
  /// whether to check that there is enough overlap
  bool check_min_edge_node_overlap = true;
  /// whether to check that there is not too much overlap between edge stem and
  /// arrow
  bool check_max_edge_stem_arrow_overlap = true;
  /// whether to check that there is enough overlap between edge stem and arrow
  bool check_min_edge_stem_arrow_overlap = true;
};

struct check_options {
  /// whether to check that there is not too much overlap
  bool check_max_edge_node_overlap = true;
  /// whether to check that there is enough overlap
  bool check_min_edge_node_overlap = true;
  /// whether to check that there is not too much overlap between edge stem and
  /// arrow
  bool check_max_edge_stem_arrow_overlap = true;
  /// whether to check that there is enough overlap between edge stem and arrow
  bool check_min_edge_stem_arrow_overlap = true;
  /// maximum allowed overlap between edge and node
  double max_node_edge_overlap;
  /// minimum required overlap between edge and node
  double min_node_edge_overlap;
  /// map between primitive arrow shape and maximum allowed overlap between edge
  /// stem and arrow
  std::unordered_map<std::string_view, double> max_edge_stem_arrow_overlap;
  /// minimum required overlap between edge stem and arrow
  double min_edge_stem_arrow_overlap;
  /// rounding error caused by limited precision in SVG attribute values
  double svg_rounding_error;
};

struct graph_options {
  std::string_view rankdir = "TB";
  std::string_view node_shape = "polygon";
  double node_fontsize = 14;
  double node_penwidth = 1;
  std::string_view dir = "forward";
  double edge_penwidth = 1;
  double edge_arrowsize = 1;
  std::string_view primitive_arrowhead_shape = "normal";
  std::string_view primitive_arrowtail_shape = "normal";
  std::string_view arrowhead_modifier = "";
  std::string_view arrowtail_modifier = "";
};

struct write_options {
  std::string filename_base = "test_edge_node_overlap";
  bool write_svg_on_success = false;
  bool write_original_svg = false;
  bool write_recreated_svg = false;
  bool write_annotated_svg = true;
};

/// generate an SVG graph from the `dot` source and check that edges don't
/// overlap nodes
void test_edge_node_overlap(const graph_options &graph_options = {},
                            const tc_check_options &tc_check_options = {},
                            const write_options &write_options = {});
