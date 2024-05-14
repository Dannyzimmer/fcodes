#include <stdexcept>

#include <catch2/catch.hpp>

#include <cgraph++/AGraph.h>

TEST_CASE("AGraph can be constructed from DOT source and has an underlying C "
          "data structure") {
  CGraph::AGraph g{"graph {a}"};
  REQUIRE(g.c_struct() != nullptr);
}

TEST_CASE("AGraph constructed from an empty string throws an exception") {
  REQUIRE_THROWS_AS(CGraph::AGraph(""), std::runtime_error);
}

TEST_CASE("AGraph constructed from bad DOT source throws an exception") {
  REQUIRE_THROWS_AS(CGraph::AGraph("THIS_SHOULD_GENERATE_A_SYNTAX_ERROR"),
                    std::runtime_error);
}

TEST_CASE("AGraph can be move constructed") {
  CGraph::AGraph g{"graph {a}"};
  const auto c_ptr = g.c_struct();
  REQUIRE(c_ptr != nullptr);
  auto other{std::move(g)};
  REQUIRE(other.c_struct() == c_ptr);
}

TEST_CASE("AGraph can be move assigned") {
  CGraph::AGraph g{"graph {a}"};
  const auto c_ptr = g.c_struct();
  REQUIRE(c_ptr != nullptr);
  auto other = CGraph::AGraph("graph {b}");
  const auto other_c_ptr = other.c_struct();
  REQUIRE(other_c_ptr != nullptr);
  REQUIRE(other_c_ptr != c_ptr);
  other = std::move(g);
  REQUIRE(other.c_struct() == c_ptr);
}
