#include <iostream>
#include <string>

#include <catch2/catch.hpp>

#include "test_edge_node_overlap_utilities.h"
#include "test_utilities.h"

TEST_CASE("Overlap cylinder node shape",
          "Test that an edge connected to a 'cylinder' shaped node touches "
          "that node and does not overlap it too much") {

  const auto shape = "cylinder";
  INFO("Node shape: " << shape);

  const auto rankdir = GENERATE(from_range(all_rank_directions));
  INFO("Rank direction: " << rankdir);

  const graph_options graph_options = {
      .rankdir = rankdir,
      .node_shape = shape,
      .node_penwidth = 2,
      .edge_penwidth = 2,
  };

  const auto filename_base = fmt::format("{}_rankdir_{}", AUTO_NAME(), rankdir);

  test_edge_node_overlap(graph_options, {}, {.filename_base = filename_base});
}
