#include <string_view>

#include <catch2/catch.hpp>
#include <fmt/format.h>

#include "svg_analyzer.h"
#include "test_utilities.h"

TEST_CASE("SvgAnalyzer fillcolor",
          "Test that the SvgAnalyzer can recreate the original SVG with the "
          "correct `fill` and `fill-opacity` attributes when the Graphviz "
          "`fillcolor` attribute is used for nodes and edges ") {

  const auto shape = GENERATE(from_range(all_node_shapes));
  INFO("Shape: " << shape);

  const std::string_view fillcolor =
      GENERATE("", "\"#10204000\"", "\"#10204080\"", "\"#102040ff\"");
  INFO("Fillcolor: " << fillcolor);
  const auto fillcolor_attr =
      fillcolor.empty() ? "" : fmt::format(" fillcolor={}", fillcolor);
  const std::string_view node_style = fillcolor.empty() ? "" : "filled";
  const auto node_style_attr =
      node_style.empty() ? "" : fmt::format(" style={}", node_style);

  auto dot =
      fmt::format("digraph g1 {{node [shape={}{}{}]; edge [{}]; a -> b}}",
                  shape, node_style_attr, fillcolor_attr, fillcolor_attr);

  SVGAnalyzer::make_from_dot(dot).re_create_and_verify_svg();
}
