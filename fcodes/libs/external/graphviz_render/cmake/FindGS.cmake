find_path(GS_INCLUDE_DIR ghostscript/iapi.h)
find_library(GS_LIBRARY NAMES gs)
find_program(GS_RUNTIME_LIBRARY NAMES gs.dll libgs-10.dll)

include(FindPackageHandleStandardArgs)
if(WIN32)
  find_package_handle_standard_args(GS DEFAULT_MSG
                                    GS_LIBRARY GS_INCLUDE_DIR
                                    GS_RUNTIME_LIBRARY)
else()
  find_package_handle_standard_args(GS DEFAULT_MSG
                                    GS_LIBRARY GS_INCLUDE_DIR)
endif()

mark_as_advanced(GS_INCLUDE_DIR GS_LIBRARY GS_RUNTIME_LIBRARY)

set(GS_INCLUDE_DIRS ${GS_INCLUDE_DIR})
set(GS_LIBRARIES ${GS_LIBRARY})
set(GS_RUNTIME_LIBRARIES ${GS_RUNTIME_LIBRARY})
