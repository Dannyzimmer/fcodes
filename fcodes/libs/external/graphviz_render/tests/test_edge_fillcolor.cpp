#include <string_view>

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

  const auto edge_rgb_fillcolor = GENERATE("#000000", "#ffffff", "#5580aa");
  const auto opacity = GENERATE(0, 100, 255);
  const auto edge_rgba_fillcolor =
      fmt::format("{}{:02x}", edge_rgb_fillcolor, opacity);
  INFO("Edge fillcolor: " << edge_rgba_fillcolor);

  auto dot =
      fmt::format("digraph g1 {{edge [arrowhead={} fillcolor=\"{}\"]; a -> b}}",
                  primitive_arrow_shape, edge_rgba_fillcolor);

  const auto engine = "dot";
  auto svg_analyzer = SVGAnalyzer::make_from_dot(dot, engine);

  const auto expected_edge_fillcolor = [&]() -> const std::string {
    if (primitive_arrow_shapes_without_closed_svg_shape.contains(
            primitive_arrow_shape)) {
      return "";
    }
    if (opacity == 255) {
      return edge_rgb_fillcolor;
    }
    if (opacity == 0) {
      return "#00000000"; // transparent/none
    }
    return edge_rgba_fillcolor;
  }();

  for (const auto &graph : svg_analyzer.graphs()) {
    for (const auto &edge : graph.edges()) {
      CHECK(edge.fillcolor() == expected_edge_fillcolor);
    }
  }
}
