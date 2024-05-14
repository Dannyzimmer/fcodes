#include <string>

#include <catch2/catch.hpp>
#include <fmt/format.h>

#include <cgraph/cgraph.h>
#include <gvc/gvc.h>

#include "svg_analyzer.h"

TEST_CASE("subgraph layout in directed and undirected graphs with different "
          "layout engines") {
  const auto directed_graph = GENERATE(false, true);

  const std::string graph_type = directed_graph ? "digraph" : "graph";
  const std::string edge_op = directed_graph ? "->" : "--";

  const auto num_subgraphs = GENERATE(1, 2);

  const std::string dot =
      num_subgraphs == 1
          ? fmt::format("{} {{subgraph s0 {{a {} b}}}}", graph_type, edge_op)
          : fmt::format(
                "{} {{subgraph s0 {{a {} b}}; subgraph s1 {{c {} d }}}}",
                graph_type, edge_op, edge_op);
  INFO("DOT source: " << dot);

  auto g = agmemread(dot.c_str());
  REQUIRE(g != nullptr);

  const std::string engine = GENERATE("dot",      //
                                      "neato",    //
                                      "fdp",      //
                                      "sfdp",     //
                                      "circo",    //
                                      "twopi",    //
                                      "osage",    //
                                      "patchwork" //
  );
  INFO("Layout engine: " << engine);

  auto gvc = gvContextPlugins(lt_preloaded_symbols, false);

  for (Agraph_t *subg = agfstsubg(g); subg; subg = agnxtsubg(subg)) {
    {
      const auto rc = gvLayout(gvc, subg, engine.c_str());
      REQUIRE(rc == 0);
    }

    char *result = nullptr;
    unsigned length = 0;
    {
      const auto rc = gvRenderData(gvc, subg, "svg", &result, &length);
      REQUIRE(rc == 0);
    }
    REQUIRE(result != nullptr);
    REQUIRE(length > 0);

    SVGAnalyzer svg_analyzer{result};

    const std::size_t num_nodes = 2;
    const std::size_t num_edges = engine == "patchwork" ? 0 : 1;
    const std::size_t num_arrowheads = directed_graph ? num_edges : 0;

    const std::size_t num_svgs = 1;
    const std::size_t num_groups = 1 + num_nodes + num_edges;
    const std::size_t num_ellipses = engine == "patchwork" ? 0 : num_nodes;
    const std::size_t num_polygons =
        1 + (engine == "patchwork" ? num_nodes : 0) + num_arrowheads;
    const std::size_t num_paths = num_edges;
    const std::size_t num_titles = 1 + num_nodes + num_edges;

    CHECK(svg_analyzer.num_svgs() == num_svgs);
    CHECK(svg_analyzer.num_groups() == num_groups);
    CHECK(svg_analyzer.num_circles() == 0);
    CHECK(svg_analyzer.num_ellipses() == num_ellipses);
    CHECK(svg_analyzer.num_lines() == 0);
    CHECK(svg_analyzer.num_paths() == num_paths);
    CHECK(svg_analyzer.num_polygons() == num_polygons);
    CHECK(svg_analyzer.num_polylines() == 0);
    CHECK(svg_analyzer.num_rects() == 0);
    CHECK(svg_analyzer.num_titles() == num_titles);

    gvFreeRenderData(result);
  }

  for (Agraph_t *subg = agfstsubg(g); subg; subg = agnxtsubg(subg)) {
    gvFreeLayout(gvc, subg);
  }

  agclose(g);
  gvFreeContext(gvc);
}
