#include <catch2/catch.hpp>
#include <fmt/format.h>

#include "svg_analyzer.h"
#include "test_utilities.h"

TEST_CASE("Edge color",
          "Test that the Graphviz `color` attribute is used to set the "
          "`stroke` and `stroke-opacity` attributes correctly for edges in the "
          "generated SVG") {

  const auto primitive_arrow_shape =
      GENERATE(from_range(all_primitive_arrow_shapes));
  INFO("Primitive arrow shape: " << primitive_arrow_shape);

  const auto edge_rgb_color = GENERATE("#000000", "#ffffff", "#5580aa");
  const auto opacity = GENERATE(0, 100, 255);
  const auto edge_rgba_color = fmt::format("{}{:02x}", edge_rgb_color, opacity);
  INFO("Edge color: " << edge_rgba_color);

  auto dot =
      fmt::format("digraph g1 {{edge [arrowhead={} color=\"{}\"]; a -> b}}",
                  primitive_arrow_shape, edge_rgba_color);

  const auto engine = "dot";
  auto svg_analyzer = SVGAnalyzer::make_from_dot(dot, engine);

  const auto expected_edge_color = [&]() -> const std::string {
    if (opacity == 255) {
      return edge_rgb_color;
    }
    if (opacity == 0) {
      return "#00000000"; // transparent/none
    }
    return edge_rgba_color;
  }();

  for (const auto &graph : svg_analyzer.graphs()) {
    for (const auto &edge : graph.edges()) {
      CHECK(edge.color() == expected_edge_color);
    }
  }
}
