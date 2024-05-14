find_path(GD_INCLUDE_DIR gd.h)
find_library(GD_LIBRARY NAMES gd libgd)
find_program(GD_RUNTIME_LIBRARY libgd.dll)

include(FindPackageHandleStandardArgs)
if(WIN32)
  find_package_handle_standard_args(GD DEFAULT_MSG
                                    GD_LIBRARY GD_INCLUDE_DIR
                                    GD_RUNTIME_LIBRARY)
else()
  find_package_handle_standard_args(GD DEFAULT_MSG
                                    GD_LIBRARY GD_INCLUDE_DIR)
endif()

mark_as_advanced(GD_INCLUDE_DIR GD_LIBRARY GD_RUNTIME_LIBRARY)

set(GD_INCLUDE_DIRS ${GD_INCLUDE_DIR})
set(GD_LIBRARIES ${GD_LIBRARY})
set(GD_RUNTIME_LIBRARIES ${GD_RUNTIME_LIBRARY})

find_package(PkgConfig)
if(PkgConfig_FOUND)
  pkg_check_modules(GDLIB gdlib>=2.0.33)
endif()

if(GD_LIBRARY)
  find_program(GDLIB_CONFIG gdlib-config)
  if(GDLIB_CONFIG)
    message(STATUS "Found gdlib-config: ${GDLIB_CONFIG}")
    execute_process(COMMAND ${GDLIB_CONFIG} --features
                    RESULT_VARIABLE GDLIB_CONFIG_EXIT_STATUS
                    OUTPUT_VARIABLE GD_FEATURES
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT GDLIB_CONFIG_EXIT_STATUS EQUAL 0)
      message(FATAL_ERROR "Failed to query GD features")
    endif()
    message(STATUS "Detected GD features: ${GD_FEATURES}")
    string(REPLACE " " ";" GD_FEATURES_LIST ${GD_FEATURES})
    if("GD_PNG" IN_LIST GD_FEATURES_LIST)
      set(HAVE_GD_PNG 1)
    endif()
    if("GD_JPEG" IN_LIST GD_FEATURES_LIST)
      set(HAVE_GD_JPEG 1)
    endif()
    if("GD_XPM" IN_LIST GD_FEATURES_LIST)
      set(HAVE_GD_XPM 1)
    endif()
    if("GD_FONTCONFIG" IN_LIST GD_FEATURES_LIST)
      set(HAVE_GD_FONTCONFIG 1)
    endif()
    if("GD_FREETYPE" IN_LIST GD_FEATURES_LIST)
      set(HAVE_GD_FREETYPE 1)
    endif()
    if("GD_GIF" IN_LIST GD_FEATURES_LIST)
      set(HAVE_GD_GIF 1)
    endif()
  else()
    if(APPLE OR GDLIB_FOUND)
      # At time of writing, Macports does not package libgd. So assume the user
      # obtained this through Homebrew and hard code the options the Homebrew
      # package enables.
      set(HAVE_GD_PNG 1)
      set(HAVE_GD_JPEG 1)
      set(HAVE_GD_FONTCONFIG 1)
      set(HAVE_GD_FREETYPE 1)
      set(HAVE_GD_GIF 1)
    else()
      message(
        WARNING
        "gdlib-config/gdlib pkgconfig not found; skipping feature checks")
    endif()
  endif()
endif()
