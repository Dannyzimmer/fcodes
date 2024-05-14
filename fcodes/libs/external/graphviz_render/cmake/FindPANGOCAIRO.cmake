include(FindPackageHandleStandardArgs)

if(WIN32)
  find_path(PANGOCAIRO_INCLUDE_DIR pango/pangocairo.h PATH_SUFFIXES pango-1.0)
  find_path(GLIB_INCLUDE_DIR glib.h PATH_SUFFIXES glib-2.0)
  find_path(GLIBCONFIG_INCLUDE_DIR glibconfig.h
            PATH_SUFFIXES glib-2.0/include lib/glib-2.0/include)
  find_path(HARFBUZZ_INCLUDE_DIR hb.h PATH_SUFFIXES harfbuzz)

  find_library(GLIB_LIBRARY NAMES glib-2.0)
  find_library(GOBJECT_LIBRARY NAMES gobject-2.0)
  find_library(PANGO_LIBRARY NAMES pango-1.0)
  find_library(PANGOCAIRO_LIBRARY NAMES pangocairo-1.0)
  find_library(HARFBUZZ_LIBRARY NAMES harfbuzz)

  find_program(GLIB_RUNTIME_LIBRARY NAMES glib-2.dll libglib-2.0-0.dll)
  find_program(GOBJECT_RUNTIME_LIBRARY NAMES gobject-2.dll libgobject-2.0-0.dll)
  find_program(HARFBUZZ_RUNTIME_LIBRARY NAMES libharfbuzz-0.dll)
  find_program(PANGO_RUNTIME_LIBRARY NAMES pango-1.dll libpango-1.0-0.dll)
  find_program(PANGOCAIRO_RUNTIME_LIBRARY
               NAMES pangocairo-1.dll libpangocairo-1.0-0.dll)
  find_program(PANGOFT_RUNTIME_LIBRARY
               NAMES pangoft2-1.dll libpangoft2-1.0-0.dll)
  find_program(PANGOWIN_RUNTIME_LIBRARY
               NAMES pangowin32-1.dll libpangowin32-1.0-0.dll)

  find_package_handle_standard_args(PANGOCAIRO DEFAULT_MSG
    GLIB_INCLUDE_DIR
    GLIBCONFIG_INCLUDE_DIR
    PANGOCAIRO_INCLUDE_DIR
    HARFBUZZ_INCLUDE_DIR

    GLIB_LIBRARY
    GOBJECT_LIBRARY
    PANGO_LIBRARY
    PANGOCAIRO_LIBRARY
    HARFBUZZ_LIBRARY

    GLIB_RUNTIME_LIBRARY
    GOBJECT_RUNTIME_LIBRARY
    HARFBUZZ_RUNTIME_LIBRARY
    PANGO_RUNTIME_LIBRARY
    PANGOCAIRO_RUNTIME_LIBRARY
    PANGOFT_RUNTIME_LIBRARY
    PANGOWIN_RUNTIME_LIBRARY
  )

  set(PANGOCAIRO_INCLUDE_DIRS
    ${GLIB_INCLUDE_DIR}
    ${GLIBCONFIG_INCLUDE_DIR}
    ${PANGOCAIRO_INCLUDE_DIR}
    ${HARFBUZZ_INCLUDE_DIR}
  )

  set(PANGOCAIRO_LIBRARIES
    glib-2.0
    gobject-2.0
    pango-1.0
    pangocairo-1.0
  )

  set(PANGOCAIRO_LINK_LIBRARIES
    ${GLIB_LIBRARY}
    ${GOBJECT_LIBRARY}
    ${PANGO_LIBRARY}
    ${PANGOCAIRO_LIBRARY}
    ${HARFBUZZ_LIBRARY}
  )

  set(PANGOCAIRO_RUNTIME_LIBRARIES
    ${GLIB_RUNTIME_LIBRARY}
    ${GOBJECT_RUNTIME_LIBRARY}
    ${HARFBUZZ_RUNTIME_LIBRARY}
    ${PANGO_RUNTIME_LIBRARY}
    ${PANGOCAIRO_RUNTIME_LIBRARY}
    ${PANGOFT_RUNTIME_LIBRARY}
    ${PANGOWIN_RUNTIME_LIBRARY}
  )
else()
  find_package(PkgConfig)
  pkg_check_modules(PANGOCAIRO pangocairo)

  find_package_handle_standard_args(PANGOCAIRO DEFAULT_MSG
    PANGOCAIRO_INCLUDE_DIRS
    PANGOCAIRO_LIBRARIES
    PANGOCAIRO_LINK_LIBRARIES
  )
endif()
