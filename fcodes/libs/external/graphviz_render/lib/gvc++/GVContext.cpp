#include "GVContext.h"
#include <gvc/gvc.h>

namespace GVC {

GVContext::GVContext(const lt_symlist_t *builtins, bool demand_loading)
    : m_gvc(gvContextPlugins(builtins, demand_loading)) {}

GVContext::GVContext() : m_gvc(gvContext()) {}

GVContext::~GVContext() {
  if (!m_gvc) {
    return;
  }
  gvFreeContext(m_gvc);
}

std::string_view GVContext::buildDate() const { return gvcBuildDate(m_gvc); }

std::string_view GVContext::version() const { return gvcVersion(m_gvc); }

} // namespace GVC
