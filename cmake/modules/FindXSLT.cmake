#.rst:
# FindXSLT
# --------
# Finds the XSLT library
#
# This will define the following target:
#
#   XSLT::XSLT - The XSLT library

if(NOT TARGET XSLT::XSLT)

  find_package(LibXml2 REQUIRED)
  find_package(PkgConfig)

  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_XSLT libxslt QUIET)
  endif()

  find_path(XSLT_INCLUDE_DIR NAMES libxslt/xslt.h
                             HINTS ${PC_XSLT_INCLUDEDIR})
  find_library(XSLT_LIBRARY NAMES xslt libxslt
                            HINTS ${PC_XSLT_LIBDIR})

  set(XSLT_VERSION ${PC_XSLT_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(XSLT
                                    REQUIRED_VARS XSLT_LIBRARY XSLT_INCLUDE_DIR
                                    VERSION_VAR XSLT_VERSION)

  if(XSLT_FOUND)
    add_library(XSLT::XSLT UNKNOWN IMPORTED)
    set_target_properties(XSLT::XSLT PROPERTIES
                                     IMPORTED_LOCATION "${XSLT_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES "${XSLT_INCLUDE_DIR}"
                                     INTERFACE_COMPILE_DEFINITIONS HAVE_LIBXSLT=1)

    target_link_libraries(XSLT::XSLT INTERFACE LibXml2::LibXml2)
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP XSLT::XSLT)
  endif()
endif()
