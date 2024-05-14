#include <string>

#include <catch2/catch.hpp>

#include "test_edge_node_overlap_utilities.h"
#include "test_utilities.h"

TEST_CASE("Maximum edge node overlap",
          "An edge connected to a node shall not overlap that node too much") {

  const graph_options graph_options = {
      .node_shape = "polygon",
      .node_penwidth = 2,
      .edge_penwidth = 2,
  };

  const tc_check_options check_options = {
      .check_max_edge_node_overlap = true,
      .check_min_edge_node_overlap = false,
      .check_max_edge_stem_arrow_overlap = false,
      .check_min_edge_stem_arrow_overlap = false};

  const auto filename_base = AUTO_NAME();

  test_edge_node_overlap(graph_options, check_options,
                         {.filename_base = filename_base});
}
