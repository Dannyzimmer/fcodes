# Header checks
include(CheckIncludeFile)

check_include_file( fcntl.h             HAVE_FCNTL_H            )
check_include_file( search.h            HAVE_SEARCH_H           )
check_include_file( sys/inotify.h       HAVE_SYS_INOTIFY_H      )
check_include_file( sys/ioctl.h         HAVE_SYS_IOCTL_H        )
check_include_file( sys/mman.h          HAVE_SYS_MMAN_H         )
check_include_file( sys/select.h        HAVE_SYS_SELECT_H       )
check_include_file( sys/time.h          HAVE_SYS_TIME_H         )
check_include_file( sys/types.h         HAVE_SYS_TYPES_H        )
check_include_file( X11/Intrinsic.h     HAVE_X11_INTRINSIC_H    )
check_include_file( X11/Xaw/Text.h      HAVE_X11_XAW_TEXT_H     )
check_include_file( getopt.h            HAVE_GETOPT_H           )

# Function checks
include(CheckFunctionExists)

check_function_exists( dl_iterate_phdr  HAVE_DL_ITERATE_PHDR )
check_function_exists( drand48          HAVE_DRAND48         )
check_function_exists( lrand48          HAVE_LRAND48         )
check_function_exists( memrchr          HAVE_MEMRCHR         )
check_function_exists( setenv           HAVE_SETENV          )
check_function_exists( setmode          HAVE_SETMODE         )
check_function_exists( sincos           HAVE_SINCOS          )
check_function_exists( srand48          HAVE_SRAND48         )

# Library checks
set( HAVE_ANN       ${ANN_FOUND}        )
if(with_expat AND EXPAT_FOUND)
  set(HAVE_EXPAT 1)
endif()
set( HAVE_LIBGD     ${GD_FOUND}         )
set( HAVE_GS        ${GS_FOUND}         )
set( HAVE_GTS       ${GTS_FOUND}        )
if(with_zlib AND ZLIB_FOUND)
  set(HAVE_ZLIB 1)
endif()
set(HAVE_PANGOCAIRO ${PANGOCAIRO_FOUND})
set(HAVE_POPPLER    ${POPPLER_FOUND}   )
set(HAVE_RSVG       ${RSVG_FOUND}      )
set(HAVE_WEBP       ${WEBP_FOUND}      )

# Values
if(WIN32)

  set( BROWSER            start                                   )

elseif(APPLE)

  set( BROWSER            open                                    )
  set( DARWIN             1                                       )
  set( DARWIN_DYLIB       1                                       )

else()

  set( BROWSER            xdg-open                                )

endif()

set(DEFAULT_DPI 96)

# Write check results to config.h header
configure_file(config-cmake.h.in config.h)
