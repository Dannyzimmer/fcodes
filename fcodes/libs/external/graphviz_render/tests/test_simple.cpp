#include <string_view>

#include <catch2/catch.hpp>

#include <cgraph/cgraph.h>
#include <gvc/gvc.h>

TEST_CASE("digraph without any nodes") {
  auto gvc = gvContextPlugins(lt_preloaded_symbols, false);

  auto dot = "digraph {}";
  auto g = agmemread(dot);

  REQUIRE(g != nullptr);

  {
    const auto rc = gvLayout(gvc, g, "dot");

    REQUIRE(rc == 0);
  }

  char *result = nullptr;
  unsigned length = 0;
  {
    const auto rc = gvRenderData(gvc, g, "svg", &result, &length);

    REQUIRE(rc == 0);
  }

  REQUIRE(result != nullptr);
  REQUIRE(length > 0);

  REQUIRE(std::string_view(result, length).find("svg") !=
          std::string_view::npos);

  gvFreeRenderData(result);
  gvFreeLayout(gvc, g);
  agclose(g);
  gvFreeContext(gvc);
}
