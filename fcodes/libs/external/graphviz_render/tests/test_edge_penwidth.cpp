#include <catch2/catch.hpp>
#include <fmt/format.h>

#include "svg_analyzer.h"
#include "test_utilities.h"

TEST_CASE("Edge penwidth",
          "Test that the Graphviz 'penwidth' attribute is used to set the "
          "'stroke-width' attribute correctly for edges in the generated SVG") {

  const auto primitive_arrow_shape =
      GENERATE(from_range(all_primitive_arrow_shapes));
  INFO("Primitive arrow shape: " << primitive_arrow_shape);

  const auto edge_penwidth = GENERATE(0.5, 1.0, 2.0);
  INFO("Edge penwidth: " << edge_penwidth);

  auto dot =
      fmt::format("digraph g1 {{edge [arrowhead={} penwidth={}]; a -> b}}",
                  primitive_arrow_shape, edge_penwidth);

  const auto engine = "dot";
  auto svg_analyzer = SVGAnalyzer::make_from_dot(dot, engine);

  for (const auto &graph : svg_analyzer.graphs()) {
    for (const auto &edge : graph.edges()) {
      CHECK(edge.penwidth() == edge_penwidth);
    }
  }
}
