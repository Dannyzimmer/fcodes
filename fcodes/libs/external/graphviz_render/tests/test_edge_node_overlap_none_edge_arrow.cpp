#include <string>

#include <catch2/catch.hpp>

#include "test_edge_node_overlap_utilities.h"
#include "test_utilities.h"

TEST_CASE("Edge node overlap for 'none' arrow",
          "An edge connected to a node shall touch that node and not overlap "
          "it too much") {

  const std::string_view primitive_arrow_shape = "none";

  INFO("Edge arrowhead: " << primitive_arrow_shape);

  std::string filename_base = AUTO_NAME();

  const graph_options graph_options = {
      .node_shape = "polygon",
      .node_penwidth = 2,
      .dir = "both",
      .edge_penwidth = 2,
      .primitive_arrowhead_shape = primitive_arrow_shape,
      .primitive_arrowtail_shape = primitive_arrow_shape,
  };

  test_edge_node_overlap(graph_options, {}, {.filename_base = filename_base});
}
