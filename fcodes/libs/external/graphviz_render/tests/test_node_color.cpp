#include <string_view>

#include <catch2/catch.hpp>
#include <fmt/format.h>

#include "svg_analyzer.h"
#include "test_utilities.h"

TEST_CASE("Node color",
          "Test that the Graphviz `color` attribute is used to set the "
          "`stroke` and `stroke-opacity` attributes correctly for nodes in the "
          "generated SVG") {

  const auto shape = GENERATE(filter(
      [](std::string_view shape) {
        return !node_shapes_without_svg_shape.contains(shape);
      },
      from_range(all_node_shapes)));
  INFO("Shape: " << shape);

  const auto node_rgb_color = GENERATE("#000000", "#ffffff", "#5580aa");
  const auto opacity = GENERATE(0, 100, 255);
  const auto node_rgba_color = fmt::format("{}{:02x}", node_rgb_color, opacity);
  INFO("Node color: " << node_rgba_color);

  auto dot = fmt::format("digraph g1 {{node [shape={} color=\"{}\"]; a -> b}}",
                         shape, node_rgba_color);

  const auto engine = "dot";
  auto svg_analyzer = SVGAnalyzer::make_from_dot(dot, engine);

  const auto expected_node_color = [&]() -> const std::string {
    if (opacity == 255) {
      return node_rgb_color;
    }
    if (opacity == 0) {
      return "#00000000"; // transparent/none
    }
    return node_rgba_color;
  }();

  for (const auto &graph : svg_analyzer.graphs()) {
    for (const auto &node : graph.nodes()) {
      if (shape == "underline" && opacity != 0) {
        // FIXME: The 'underline' node shape consists of a transparent polygon
        // and a polyline with the specified color which the node color
        // retrieval function cannot yet handle (unless the specified color is
        // also transparent) and will throw an exception for
        REQUIRE_THROWS_AS(node.color(), std::runtime_error);
        continue;
      }
      CHECK(node.color() == expected_node_color);
    }
  }
}
