#include <utility>

#include <catch2/catch.hpp>

#include <gvc++/GVContext.h>

TEST_CASE("GVContext can be constructed without built-in plugins and its "
          "underlying C data structure can be retrieved") {
  GVC::GVContext gvc;
  REQUIRE(gvc.c_struct() != nullptr);
}

TEST_CASE("GVContext can be constructed with built-in plugins and its "
          "underlying C data structure can be retrieved") {
  const auto demand_loading = false;
  GVC::GVContext gvc{lt_preloaded_symbols, demand_loading};
  REQUIRE(gvc.c_struct() != nullptr);
}

TEST_CASE("GVContext can be move constructed") {
  GVC::GVContext gvc;
  const auto c_ptr = gvc.c_struct();
  REQUIRE(c_ptr != nullptr);
  auto other{std::move(gvc)};
  REQUIRE(other.c_struct() == c_ptr);
}

TEST_CASE("GVContext can be move assigned") {
  GVC::GVContext gvc;
  const auto c_ptr = gvc.c_struct();
  REQUIRE(c_ptr != nullptr);
  auto other = GVC::GVContext();
  const auto other_c_ptr = other.c_struct();
  REQUIRE(other_c_ptr != nullptr);
  REQUIRE(other_c_ptr != c_ptr);
  other = std::move(gvc);
  REQUIRE(other.c_struct() == c_ptr);
}
