#.rst:
# FindXSLT
# --------
# Finds the XSLT library
#
# This will will define the following variables::
#
# XSLT_FOUND - system has XSLT
# XSLT_INCLUDE_DIRS - the XSLT include directory
# XSLT_LIBRARIES - the XSLT libraries
# XSLT_DEFINITIONS - the XSLT definitions
#
# and the following imported targets::
#
#   XSLT::XSLT   - The XSLT library

find_package(LibXml2 REQUIRED)

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_XSLT libxslt QUIET)
endif()

find_path(XSLT_INCLUDE_DIR NAMES libxslt/xslt.h
                           PATHS ${PC_XSLT_INCLUDEDIR})
find_library(XSLT_LIBRARY NAMES xslt libxslt
                          PATHS ${PC_XSLT_LIBDIR})

set(XSLT_VERSION ${PC_XSLT_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(XSLT
                                  REQUIRED_VARS XSLT_LIBRARY XSLT_INCLUDE_DIR
                                  VERSION_VAR XSLT_VERSION)

if(XSLT_FOUND)
  set(XSLT_LIBRARIES ${XSLT_LIBRARY} ${LIBXML2_LIBRARIES})
  set(XSLT_INCLUDE_DIRS ${XSLT_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR})
  set(XSLT_DEFINITIONS -DHAVE_LIBXSLT=1)

  if(NOT TARGET XSLT::XSLT)
    add_library(XSLT::XSLT UNKNOWN IMPORTED)
    set_target_properties(XSLT::XSLT PROPERTIES
                                     IMPORTED_LOCATION "${XSLT_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES "${XSLT_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR}"
                                     INTERFACE_COMPILE_DEFINITIONS HAVE_LIBXSLT=1
                                     ITERFACE_LINK_LIBRARIES "${LIBXML2_LIBRARIES}")
  endif()
endif()

mark_as_advanced(XSLT_INCLUDE_DIR XSLT_LIBRARY)
