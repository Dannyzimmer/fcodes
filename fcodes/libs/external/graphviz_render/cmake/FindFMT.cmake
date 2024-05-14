find_path(FMT_INCLUDE_DIR fmt/core.h)

find_library(FMT_LIBRARY NAMES fmt)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FMT DEFAULT_MSG FMT_LIBRARY FMT_INCLUDE_DIR)

mark_as_advanced(FMT_INCLUDE_DIR FMT_LIBRARY)

set(FMT_INCLUDE_DIRS ${FMT_INCLUDE_DIR})
set(FMT_LIBRARIES ${FMT_LIBRARY})
