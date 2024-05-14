#include <catch2/catch.hpp>
#include <fmt/format.h>

#include "test_edge_node_overlap_utilities.h"
#include "test_utilities.h"

TEST_CASE("Maximum edge and node overlap for polygon node shapes",
          "Test that an edge connected to a polygon based node does not "
          "overlap that node, regardless of the node shape") {

  const auto shape = GENERATE(from_range(node_shapes_consisting_of_polygon));
  INFO("Node shape: " << shape);

  const graph_options graph_options = {
      .node_shape = shape,
      .node_penwidth = 2,
      .edge_penwidth = 2,
  };

  const tc_check_options check_options = {
      .check_max_edge_node_overlap = true,
      .check_min_edge_node_overlap = false,
      .check_max_edge_stem_arrow_overlap = false,
      .check_min_edge_stem_arrow_overlap = false,
  };

  const auto filename_base = fmt::format("{}_{}", AUTO_NAME(), shape);

  test_edge_node_overlap(graph_options, check_options,
                         {.filename_base = filename_base});
}
