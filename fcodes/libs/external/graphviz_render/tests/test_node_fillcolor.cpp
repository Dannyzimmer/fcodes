#include <string_view>

#include <catch2/catch.hpp>
#include <fmt/format.h>

#include "svg_analyzer.h"
#include "test_utilities.h"

TEST_CASE("Node fillcolor",
          "Test that the Graphviz `fillcolor` attribute is used to set "
          "the `fill` and `fill-opacity` attributes correctly for nodes in the "
          "generated SVG") {

  const auto shape = GENERATE(filter(
      [](std::string_view shape) {
        return !node_shapes_without_svg_shape.contains(shape);
      },
      from_range(all_node_shapes)));
  INFO("Shape: " << shape);

  const auto node_rgb_fillcolor =
      GENERATE("#000000", "#ffffff", "#ff0000", "#aa0f55");
  const auto opacity = GENERATE(0, 200, 255);
  const auto node_rgba_fillcolor =
      fmt::format("{}{:02x}", node_rgb_fillcolor, opacity);
  INFO("Node fillcolor: " << node_rgba_fillcolor);

  auto dot = fmt::format(
      "digraph g1 {{node [shape={} style=filled fillcolor=\"{}\"]; a -> b}}",
      shape, node_rgba_fillcolor);

  const auto engine = "dot";
  auto svg_analyzer = SVGAnalyzer::make_from_dot(dot, engine);

  const auto expected_node_fillcolor = [&]() -> const std::string {
    if (opacity == 255) {
      return node_rgb_fillcolor;
    }
    if (opacity == 0) {
      return "#00000000"; // transparent/none
    }
    return node_rgba_fillcolor;
  }();

  for (const auto &graph : svg_analyzer.graphs()) {
    for (const auto &node : graph.nodes()) {
      if (contains_multiple_shapes_with_different_fill(shape) && opacity != 0) {
        // FIXME: these node shapes consists of both a transparent shape and a
        // shape with the specified filllcolor which the node fillcolor
        // retrieval function cannot yet handle (unless the specified fillcolor
        // is also transparent) and will throw an exception for
        REQUIRE_THROWS_AS(node.fillcolor(), std::runtime_error);
        continue;
      }
      CHECK(node.fillcolor() == expected_node_fillcolor);
    }
  }
}
