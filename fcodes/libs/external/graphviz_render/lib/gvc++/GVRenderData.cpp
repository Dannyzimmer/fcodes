#include "GVRenderData.h"
#include <gvc/gvc.h>

namespace GVC {

GVRenderData::GVRenderData(char *data, std::size_t length)
    : m_data(data), m_length(length) {}
GVRenderData::~GVRenderData() { gvFreeRenderData(m_data); }

} // namespace GVC
