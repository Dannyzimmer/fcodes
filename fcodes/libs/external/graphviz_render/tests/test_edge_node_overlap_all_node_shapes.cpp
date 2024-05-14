#include <iostream>
#include <string>

#include <catch2/catch.hpp>
#include <fmt/format.h>

#include "test_edge_node_overlap_utilities.h"
#include "test_utilities.h"

TEST_CASE("Overlap all node shapes",
          "Test that an edge connected to a node touches that node and does "
          "not overlap it too much, regardless of the node shape") {

  const auto shape = GENERATE(from_range(all_node_shapes));
  INFO("Node shape: " << shape);

  const graph_options graph_options = {
      .node_shape = shape,
      .node_penwidth = 2,
      .edge_penwidth = 2,
  };

  const auto filename_base = fmt::format("{}_{}", AUTO_NAME(), shape);

  test_edge_node_overlap(graph_options, {}, {.filename_base = filename_base});
}
