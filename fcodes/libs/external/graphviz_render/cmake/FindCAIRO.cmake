include(FindPackageHandleStandardArgs)

if(WIN32)
  find_path(
    CAIRO_INCLUDE_DIR cairo.h
    PATH_SUFFIXES cairo
  )

  find_library(CAIRO_LIBRARY NAMES cairo)

  find_program(CAIRO_RUNTIME_LIBRARY NAMES cairo.dll libcairo-2.dll)
  find_program(EXPAT_RUNTIME_LIBRARY NAMES expat.dll libexpat-1.dll)
  find_program(FONTCONFIG_RUNTIME_LIBRARY
               NAMES fontconfig.dll libfontconfig-1.dll)
  find_program(PIXMAN_RUNTIME_LIBRARY NAMES pixman-1.dll libpixman-1-0.dll)

  find_package_handle_standard_args(CAIRO DEFAULT_MSG
    CAIRO_INCLUDE_DIR

    CAIRO_LIBRARY

    CAIRO_RUNTIME_LIBRARY
    EXPAT_RUNTIME_LIBRARY
    FONTCONFIG_RUNTIME_LIBRARY
    PIXMAN_RUNTIME_LIBRARY
  )

  set(CAIRO_INCLUDE_DIRS ${CAIRO_INCLUDE_DIR})

  set(CAIRO_LIBRARIES ${CAIRO_LIBRARY})

  set(CAIRO_LINK_LIBRARIES ${CAIRO_LIBRARY})

  set(CAIRO_RUNTIME_LIBRARIES
    ${CAIRO_RUNTIME_LIBRARY}
    ${EXPAT_RUNTIME_LIBRARY}
    ${FONTCONFIG_RUNTIME_LIBRARY}
    ${PIXMAN_RUNTIME_LIBRARY}
  )
else()
  find_package(PkgConfig)
  pkg_check_modules(CAIRO cairo)

  find_package_handle_standard_args(CAIRO DEFAULT_MSG
    CAIRO_INCLUDE_DIRS
    CAIRO_LIBRARIES
    CAIRO_LINK_LIBRARIES
  )
endif()
