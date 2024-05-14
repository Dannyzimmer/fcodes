#include <stdexcept>
#include <string>

#include "AGraph.h"

namespace CGraph {

AGraph::AGraph(const std::string &dot) {
  const auto g = agmemread(dot.c_str());
  if (!g) {
    throw std::runtime_error("Could not read graph");
  }
  m_g = g;
}

AGraph::~AGraph() {
  if (m_g) {
    agclose(m_g);
  }
}

} // namespace CGraph
