#.rst:
# FindXSLT
# --------
# Finds the XSLT library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::XSLT - The XSLT library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  find_package(LibXml2 REQUIRED ${SEARCH_QUIET})
  find_package(PkgConfig ${SEARCH_QUIET})

  if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWS_STORE))
    pkg_check_modules(PC_XSLT libxslt ${SEARCH_QUIET})
  endif()

  find_path(XSLT_INCLUDE_DIR NAMES libxslt/xslt.h
                             HINTS ${DEPENDS_PATH}/include ${PC_XSLT_INCLUDEDIR}
                             ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})
  find_library(XSLT_LIBRARY NAMES xslt libxslt
                            HINTS ${DEPENDS_PATH}/lib ${PC_XSLT_LIBDIR}
                            ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

  set(XSLT_VERSION ${PC_XSLT_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(XSLT
                                    REQUIRED_VARS XSLT_LIBRARY XSLT_INCLUDE_DIR
                                    VERSION_VAR XSLT_VERSION)

  if(XSLT_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${XSLT_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${XSLT_INCLUDE_DIR}"
                                                                     INTERFACE_COMPILE_DEFINITIONS HAVE_LIBXSLT)

    target_link_libraries(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} INTERFACE LibXml2::LibXml2)
  endif()
endif()
