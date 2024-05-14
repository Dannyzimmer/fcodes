#include <cstring>

#include <catch2/catch.hpp>

#include <cgraph++/AGraph.h>
#include <gvc++/GVContext.h>
#include <gvc++/GVLayout.h>
#include <gvc++/GVRenderData.h>

TEST_CASE("Rendered SVG can be retrieved as a string_view or as a C string") {
  const auto demand_loading = false;
  auto gvc =
      std::make_shared<GVC::GVContext>(lt_preloaded_symbols, demand_loading);

  auto dot = "digraph {a}";
  auto g = std::make_shared<CGraph::AGraph>(dot);

  const auto layout = GVC::GVLayout(gvc, g, "dot");

  const auto result = layout.render("svg");
  // just check that we seem to have gotten the correct format
  REQUIRE(result.string_view().find("<!DOCTYPE svg") != std::string_view::npos);
  REQUIRE(result.string_view().compare(result.c_str()) == 0);
  REQUIRE(result.string_view().length() == result.length());
  REQUIRE(std::strlen(result.c_str()) == result.length());
}

TEST_CASE("Rendering in an unknown format throws an exception") {
  const auto demand_loading = false;
  auto gvc =
      std::make_shared<GVC::GVContext>(lt_preloaded_symbols, demand_loading);

  auto dot = "digraph {a}";
  auto g = std::make_shared<CGraph::AGraph>(dot);

  const auto layout = GVC::GVLayout(gvc, g, "dot");

  REQUIRE_THROWS_AS(layout.render("UNKNOWN_FORMAT"), std::runtime_error);
}
