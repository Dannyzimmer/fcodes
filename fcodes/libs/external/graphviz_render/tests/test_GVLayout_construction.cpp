#include <memory>

#include <catch2/catch.hpp>

#include <cgraph++/AGraph.h>
#include <gvc++/GVContext.h>
#include <gvc++/GVLayout.h>

TEST_CASE("Layout of a graph can use on-demand loaded plugins") {
  auto gvc = std::make_shared<GVC::GVContext>();

  auto dot = "graph {a}";
  auto g = std::make_shared<CGraph::AGraph>(dot);

  const auto layout = GVC::GVLayout(gvc, g, "dot");
}

TEST_CASE("Layout of a graph can use built-in plugins") {
  const auto demand_loading = false;
  auto gvc =
      std::make_shared<GVC::GVContext>(lt_preloaded_symbols, demand_loading);

  auto dot = "graph {a}";
  auto g = std::make_shared<CGraph::AGraph>(dot);

  const auto layout = GVC::GVLayout(gvc, g, "dot");
}

TEST_CASE("A layout can be moved assigned") {
  auto gvc = std::make_shared<GVC::GVContext>();

  auto dot = "graph {a}";
  auto g = std::make_shared<CGraph::AGraph>(dot);

  auto layout = GVC::GVLayout(gvc, g, "dot");

  const auto layout2 = std::move(layout);
}

TEST_CASE("A layout can be move constructed") {
  auto gvc = std::make_shared<GVC::GVContext>();

  auto dot = "graph {a}";
  auto g = std::make_shared<CGraph::AGraph>(dot);

  auto layout = GVC::GVLayout(gvc, g, "dot");
  auto other{std::move(layout)};
}

TEST_CASE("Layout of multiple graphs can use the same context") {
  const auto demand_loading = false;
  auto gvc =
      std::make_shared<GVC::GVContext>(lt_preloaded_symbols, demand_loading);

  auto dot1 = "graph {a}";
  auto g1 = std::make_shared<CGraph::AGraph>(dot1);

  const auto layout1 = GVC::GVLayout(gvc, g1, "dot");

  auto dot2 = "graph {b}";
  auto g2 = std::make_shared<CGraph::AGraph>(dot2);

  const auto layout2 = GVC::GVLayout(gvc, g2, "dot");
}

TEST_CASE("Multiple layouts of the same graph can use different contexts") {
  const auto demand_loading = false;
  auto gvc1 =
      std::make_shared<GVC::GVContext>(lt_preloaded_symbols, demand_loading);
  auto dot = "graph {a}";
  auto g = std::make_shared<CGraph::AGraph>(dot);

  // create a layout and automatically destroy it
  { const auto layout = GVC::GVLayout(gvc1, g, "dot"); }

  auto gvc2 =
      std::make_shared<GVC::GVContext>(lt_preloaded_symbols, demand_loading);

  // create another layout and automatically destroy it
  { const auto layout2 = GVC::GVLayout(gvc2, g, "dot"); }
}

TEST_CASE("Creating a second layout for the same graph without destroying the "
          "first throws an exception") {
  const auto demand_loading = false;
  auto gvc1 =
      std::make_shared<GVC::GVContext>(lt_preloaded_symbols, demand_loading);
  auto dot = "graph {a}";
  auto g = std::make_shared<CGraph::AGraph>(dot);

  // create a layout and keep it
  const auto layout = GVC::GVLayout(gvc1, g, "dot");

  auto gvc2 =
      std::make_shared<GVC::GVContext>(lt_preloaded_symbols, demand_loading);

  // create another layout
  REQUIRE_THROWS_AS(GVC::GVLayout(gvc2, g, "dot"), std::runtime_error);
}

TEST_CASE("Layout of a graph can use on-demand loaded plugins and pass the "
          "context and the graph as rvalue refs)") {
  auto gvc = GVC::GVContext();

  auto dot = "graph {}";
  auto g = CGraph::AGraph(dot);

  const auto layout = GVC::GVLayout(std::move(gvc), std::move(g), "dot");
}

TEST_CASE("Layout of a graph can use built-in plugins and pass the context and "
          "the graph as rvalue refs)") {
  const auto demand_loading = false;
  auto gvc = GVC::GVContext(lt_preloaded_symbols, demand_loading);

  auto dot = "graph {}";
  auto g = CGraph::AGraph(dot);

  const auto layout = GVC::GVLayout(std::move(gvc), std::move(g), "dot");
}

TEST_CASE("Layout of multiple graphs can use the same context and pass the "
          "graphs as rvalue refs") {
  const auto demand_loading = false;
  auto gvc = std::make_shared<GVC::GVContext>(
      GVC::GVContext(lt_preloaded_symbols, demand_loading));

  auto dot1 = "graph {}";
  auto g1 = CGraph::AGraph(dot1);

  const auto layout1 = GVC::GVLayout(gvc, std::move(g1), "dot");

  auto dot2 = "graph {}";
  auto g2 = CGraph::AGraph(dot2);

  const auto layout2 = GVC::GVLayout(gvc, std::move(g2), "dot");
}

TEST_CASE("Multiple layouts of the same graph can use different contexts "
          "passed as rvalue refs") {
  const auto demand_loading = false;
  auto gvc1 = GVC::GVContext(lt_preloaded_symbols, demand_loading);
  auto dot = "graph {}";
  auto g = std::make_shared<CGraph::AGraph>(dot);

  // create a layout and automatically destroy it
  { const auto layout1 = GVC::GVLayout(std::move(gvc1), g, "dot"); }

  auto gvc2 = GVC::GVContext(lt_preloaded_symbols, demand_loading);

  // create another layout and automatically destroy it
  { const auto layout2 = GVC::GVLayout(std::move(gvc2), g, "dot"); }
}

TEST_CASE("Layout with an unknown engine throws an exception") {
  auto dot = "digraph {}";
  auto g = std::make_shared<CGraph::AGraph>(dot);

  const auto demand_loading = false;
  auto gvc =
      std::make_shared<GVC::GVContext>(lt_preloaded_symbols, demand_loading);

  REQUIRE_THROWS_AS(GVC::GVLayout(gvc, g, "UNKNOWN_ENGINE"),
                    std::runtime_error);
}
