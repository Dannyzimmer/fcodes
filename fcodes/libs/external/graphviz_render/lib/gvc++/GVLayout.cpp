#include <cassert>
#include <memory>
#include <stdexcept>
#include <string>

#include "GVContext.h"
#include "GVLayout.h"
#include "GVRenderData.h"
#include <cgraph++/AGraph.h>
#include <gvc/gvc.h>

namespace GVC {

GVLayout::GVLayout(const std::shared_ptr<GVContext> &gvc,
                   const std::shared_ptr<CGraph::AGraph> &g,
                   const std::string &engine)
    : m_gvc(gvc), m_g(g) {
  if (gvLayoutDone(g->c_struct())) {
    gvFreeLayout(gvc->c_struct(), g->c_struct());
    throw std::runtime_error("Previous layout not yet destroyed");
  }
  const auto rc = gvLayout(gvc->c_struct(), g->c_struct(), engine.c_str());
  if (rc) {
    throw std::runtime_error("Layout failed");
  }
}

GVLayout::GVLayout(GVContext &&gvc, CGraph::AGraph &&g,
                   const std::string &engine)
    : GVLayout(std::make_shared<GVContext>(std::move(gvc)),
               std::make_shared<CGraph::AGraph>(std::move(g)), engine) {}

GVLayout::GVLayout(std::shared_ptr<GVContext> gvc, CGraph::AGraph &&g,
                   const std::string &engine)
    : GVLayout(gvc, std::make_shared<CGraph::AGraph>(std::move(g)), engine) {}

GVLayout::GVLayout(GVContext &&gvc, std::shared_ptr<CGraph::AGraph> g,
                   const std::string &engine)
    : GVLayout(std::make_shared<GVContext>(std::move(gvc)), g, engine) {}

GVLayout::~GVLayout() {
  // m_gvc and m_g always either both point to an object or both are null
  assert((m_gvc != nullptr && m_g != nullptr) ||
         (m_gvc == nullptr && m_g == nullptr));
  if (!m_gvc || !m_g) {
    return;
  }
  gvFreeLayout(m_gvc->c_struct(), m_g->c_struct());
}

GVRenderData GVLayout::render(const std::string &format) const {
  char *result = nullptr;
  unsigned int length = 0;
  const auto rc = gvRenderData(m_gvc->c_struct(), m_g->c_struct(),
                               format.c_str(), &result, &length);
  if (rc) {
    if (result) {
      gvFreeRenderData(result);
    }
    throw std::runtime_error("Rendering failed");
  }

  return GVRenderData(result, length);
}

} // namespace GVC
