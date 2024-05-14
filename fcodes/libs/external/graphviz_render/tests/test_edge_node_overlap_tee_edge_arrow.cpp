#include <string>

#include <catch2/catch.hpp>

#include "test_edge_node_overlap_utilities.h"
#include "test_utilities.h"

TEST_CASE("Edge node overlap for 'tee' arrow",
          "An edge connected to a node shall touch that node and not overlap "
          "it too much") {

  const std::string_view primitive_arrow_shape = "tee";

  INFO("Edge arrowhead: " << primitive_arrow_shape);

  std::string filename_base = AUTO_NAME();

  const graph_options graph_options = {
      .node_shape = "polygon",
      .node_penwidth = 2,
      .dir = "both",
      // The 'tee' arrow shape consists of a 'polygon' part which expands with
      // penwidth in the forward and reverse direction of the arrow and a
      // 'polyline' part which does not expand with penwidth in the forward and
      // reverse direction of the arrow.
      //
      // Edge penwidth > 2 causes the 'polygon' part of the arrow to overlap
      // the 'polyline' part at the end of the arrow.
      //
      // Edge penwidth > 4 causes the 'polygon' part of the arrow to overlap
      // the 'polyline' part at the start of the arrow.
      //
      // However, in the 'test_edge_node_overlap' function, we allow a quite
      // large overlap between the edge stem and the edge arrow because it's not
      // visible if it's less than the edge penwidth (unless the color is
      // semi-transparent). We therefore set an edge penwidth much larger than 4
      // in this test in order to be able to check that edge stem and edge arrow
      // don't overlap too much.
      .edge_penwidth = 7,
      .primitive_arrowhead_shape = primitive_arrow_shape,
      .primitive_arrowtail_shape = primitive_arrow_shape,
  };

  test_edge_node_overlap(graph_options, {}, {.filename_base = filename_base});
}
