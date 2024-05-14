#include <catch2/catch.hpp>
#include <fmt/format.h>

#include "test_edge_node_overlap_utilities.h"
#include "test_utilities.h"

TEST_CASE(
    "Overlap polygon node shapes",
    "Test that an edge connected to a polygon based node touches that node and "
    "does not overlap it too much, regardless of the node shape") {

  const auto shape = GENERATE(from_range(node_shapes_consisting_of_polygon));
  INFO("Node shape: " << shape);

  const auto rankdir = GENERATE(from_range(all_rank_directions));
  INFO("Rank direction: " << rankdir);

  const graph_options graph_options = {
      .rankdir = rankdir,
      .node_shape = shape,
      .node_penwidth = 2,
      .edge_penwidth = 2,
  };

  const auto filename_base =
      fmt::format("{}_{}_rankdir_{}", AUTO_NAME(), shape, rankdir);

  test_edge_node_overlap(graph_options, {}, {.filename_base = filename_base});
}
