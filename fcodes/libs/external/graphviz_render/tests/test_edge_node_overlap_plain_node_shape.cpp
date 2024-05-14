#include <iostream>
#include <string>

#include <catch2/catch.hpp>

#include "test_edge_node_overlap_utilities.h"
#include "test_utilities.h"

TEST_CASE("Overlap plain node shape",
          "Test that an edge connected to a 'plain' shaped node touches "
          "that node and does not overlap it too much") {

  std::string filename_base = AUTO_NAME();

  const auto shape = "plain";
  INFO("Node shape: " << shape);

  const double fontsize = GENERATE(range(2, 101));
  INFO("Node fontsize: " << fontsize);
  filename_base += fmt::format("_fontsize_{}", fontsize);

  const auto rankdir = GENERATE(from_range(all_rank_directions));
  INFO("Rank direction: " << rankdir);
  filename_base += fmt::format("_rankdir_{}", rankdir);

  const graph_options graph_options = {
      .rankdir = rankdir,
      .node_shape = shape,
      .node_fontsize = fontsize,
      .node_penwidth = 1,
      .edge_penwidth = 1,
  };

  test_edge_node_overlap(graph_options, {}, {.filename_base = filename_base});
}
