#include <string_view>

#include <catch2/catch.hpp>
#include <fmt/format.h>

#include "svg_analyzer.h"
#include "test_utilities.h"

TEST_CASE("Node penwidth",
          "Test that the Graphviz 'penwidth' attribute is used to set the "
          "'stroke-width' attribute correctly for nodes in the generated SVG") {

  const auto shape = GENERATE(filter(
      [](std::string_view shape) {
        return !node_shapes_without_svg_shape.contains(shape);
      },
      from_range(all_node_shapes)));
  INFO("Shape: " << shape);

  const auto node_penwidth = GENERATE(0.5, 1.0, 2.0);
  INFO("Node penwidth: " << node_penwidth);

  auto dot = fmt::format("digraph g1 {{node [shape={} penwidth={}]; a -> b}}",
                         shape, node_penwidth);

  const auto engine = "dot";
  auto svg_analyzer = SVGAnalyzer::make_from_dot(dot, engine);

  for (const auto &graph : svg_analyzer.graphs()) {
    for (const auto &node : graph.nodes()) {
      CHECK(node.penwidth() == node_penwidth);
    }
  }
}
