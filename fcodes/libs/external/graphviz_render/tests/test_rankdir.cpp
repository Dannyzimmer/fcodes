#include <string_view>

#include <catch2/catch.hpp>
#include <fmt/format.h>

#include "svg_analyzer.h"
#include "test_utilities.h"

TEST_CASE("Graph rankdir", "Test that the Graphviz `rankdir` attribute affects "
                           "the relative placement of nodes and edges "
                           "correctly when the 'dot` layout engine is used") {

  const auto rankdir = GENERATE(from_range(all_rank_directions));
  INFO("Rankdir: " << rankdir);

  const auto shape = GENERATE(filter(
      [](std::string_view shape) {
        return !node_shapes_without_svg_shape.contains(shape);
      },
      from_range(all_node_shapes)));
  INFO("Shape: " << shape);

  auto dot = fmt::format(
      "digraph g1 {{rankdir={}; node [shape={} fontname=Courier]; a -> b}}",
      rankdir, shape);

  const auto engine = "dot";
  auto svg_analyzer = SVGAnalyzer::make_from_dot(dot, engine);

  REQUIRE(svg_analyzer.graphs().size() == 1);
  const auto &graph = svg_analyzer.graphs().back();
  const auto node_a = graph.node("a");
  const auto node_b = graph.node("b");
  const auto edge_ab = graph.edge("a->b");
  if (rankdir == "TB") {
    CHECK(node_a.center().is_higher_than(node_b.center()));
    CHECK(node_a.center().is_higher_than(edge_ab.center()));
    CHECK(edge_ab.center().is_higher_than(node_b.center()));
  } else if (rankdir == "BT") {
    CHECK(node_a.center().is_lower_than(node_b.center()));
    CHECK(node_a.center().is_lower_than(edge_ab.center()));
    CHECK(edge_ab.center().is_lower_than(node_b.center()));
  } else if (rankdir == "LR") {
    CHECK(node_a.center().is_more_left_than(node_b.center()));
    CHECK(node_a.center().is_more_left_than(edge_ab.center()));
    CHECK(edge_ab.center().is_more_left_than(node_b.center()));
  } else if (rankdir == "RL") {
    CHECK(node_a.center().is_more_right_than(node_b.center()));
    CHECK(node_a.center().is_more_right_than(edge_ab.center()));
    CHECK(edge_ab.center().is_more_right_than(node_b.center()));
  }
}
