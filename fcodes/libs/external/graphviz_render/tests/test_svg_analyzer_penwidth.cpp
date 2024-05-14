#include <catch2/catch.hpp>
#include <fmt/format.h>

#include "svg_analyzer.h"
#include "test_utilities.h"

TEST_CASE("SvgAnalyzer penwidth",
          "Test that the SvgAnalyzer can recreate the original SVG with the "
          "correct `stroke-width` attribute when the Graphviz `penwidth` "
          "attribute is used for nodes and edges") {

  const auto shape = GENERATE(from_range(all_node_shapes));
  INFO("Shape: " << shape);

  const auto penwidth = GENERATE(0.5, 1.0, 2.0);
  INFO("Node and edge penwidth: " << penwidth);

  auto dot = fmt::format(
      "digraph g1 {{node [shape={} penwidth={}]; edge [penwidth={}]; a -> b}}",
      shape, penwidth, penwidth);

  SVGAnalyzer::make_from_dot(dot).re_create_and_verify_svg();
}
