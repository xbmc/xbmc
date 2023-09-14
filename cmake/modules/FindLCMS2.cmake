#.rst:
# FindLCMS2
# -----------
# Finds the LCMS Color Management library
#
# This will define the following target:
#
#   LCMS2::LCMS2 - The LCMS Color Management library

if(NOT TARGET LCMS2::LCMS2)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_LCMS2 lcms2>=2.10 QUIET)
  endif()

  find_path(LCMS2_INCLUDE_DIR NAMES lcms2.h
                              HINTS ${PC_LCMS2_INCLUDEDIR}
                              NO_CACHE)
  find_library(LCMS2_LIBRARY NAMES lcms2 liblcms2
                             HINTS ${PC_LCMS2_LIBDIR}
                             NO_CACHE)

  set(LCMS2_VERSION ${PC_LCMS2_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LCMS2
                                    REQUIRED_VARS LCMS2_LIBRARY LCMS2_INCLUDE_DIR
                                    VERSION_VAR LCMS2_VERSION)

  if(LCMS2_FOUND)
    add_library(LCMS2::LCMS2 UNKNOWN IMPORTED)
    set_target_properties(LCMS2::LCMS2 PROPERTIES
                                       IMPORTED_LOCATION "${LCMS2_LIBRARY}"
                                       INTERFACE_INCLUDE_DIRECTORIES "${LCMS2_INCLUDE_DIR}"
                                       INTERFACE_COMPILE_DEFINITIONS "HAVE_LCMS2=1;CMS_NO_REGISTER_KEYWORD=1")
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP LCMS2::LCMS2)
  endif()
endif()
