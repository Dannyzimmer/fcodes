#include <cstdlib>
#include <string>

#include <catch2/catch.hpp>
#include <fmt/format.h>

#include <cgraph/cgraph.h>
#include <gvc/gvc.h>
#include <pack/pack.h>

#include "svg_analyzer.h"

TEST_CASE("take an input graph, compute its connected components, lay out each "
          "using neato, pack each layout into a single graph, and render that "
          "graph in SVG") {
  const auto directed_graph = GENERATE(false, true);

  const std::string graph_type = directed_graph ? "digraph" : "graph";
  const std::string edge_op = directed_graph ? "->" : "--";

  const auto num_subgraphs = GENERATE(1, 2);

  const std::string dot =
      num_subgraphs == 1
          ? fmt::format("{} {{a {} b}}", graph_type, edge_op)
          : fmt::format("{} {{a {} b; c {} d}}", graph_type, edge_op, edge_op);
  INFO("DOT source: " << dot);

  auto *g = agmemread(dot.c_str());
  REQUIRE(g != nullptr);

  aginit(g, AGRAPH, "Agraphinfo_t", sizeof(Agraphinfo_t), true);
  aginit(g, AGNODE, "Agnodeinfo_t", sizeof(Agnodeinfo_t), true);

  int ncc = 0;
  auto **cc = ccomps(g, &ncc, NULL);
  REQUIRE(ncc == num_subgraphs);

  auto *gvc = gvContextPlugins(lt_preloaded_symbols, false);

  for (int i = 0; i < ncc; i++) {
    graph_t *sg = cc[i];
    const auto nedges = nodeInduce(sg);
    REQUIRE(nedges == 1);
    gvLayout(gvc, sg, "neato");
  }
  pack_graph(ncc, cc, g, 0);

  char *result = nullptr;
  unsigned length = 0;
  {
    const auto rc = gvRenderData(gvc, g, "svg", &result, &length);
    REQUIRE(rc == 0);
  }
  REQUIRE(result != nullptr);
  REQUIRE(length > 0);

  SVGAnalyzer svg_analyzer{result};

  const std::size_t num_nodes = 2 * num_subgraphs;
  const std::size_t num_edges = num_subgraphs;
  const std::size_t num_arrowheads = directed_graph ? num_edges : 0;

  const std::size_t num_svgs = 1;
  const std::size_t num_groups = 1 + num_nodes + num_edges;
  const std::size_t num_ellipses = num_nodes;
  const std::size_t num_polygons = 1 + num_arrowheads;
  const std::size_t num_paths = num_edges;
  const std::size_t num_titles = num_nodes + num_edges;

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

  for (int i = 0; i < ncc; i++) {
    graph_t *sg = cc[i];
    gvFreeLayout(gvc, sg);
    agdelete(g, sg);
  }

  agclose(g);

  gvFreeContext(gvc);

  free(cc);
}
