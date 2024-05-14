find_path(SVGPP_INCLUDE_DIR svgpp/svgpp.hpp)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SVGPP DEFAULT_MSG SVGPP_INCLUDE_DIR)

set(SVGPP_INCLUDE_DIRS ${SVGPP_INCLUDE_DIR})
