#.rst:
# FindLibZip
# -----------
# Finds the LibZip library
#
# The following imported target will be created:
#
#   ${APP_NAME_LC}::LibZip - The LibZip library

include(cmake/scripts/common/ModuleHelpers.cmake)

set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libzip)
SETUP_BUILD_VARS()

SETUP_FIND_SPECS()

SEARCH_EXISTING_PACKAGES()

if("${libzip_VERSION}" VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")
  # Check for dependencies
  find_package(GnuTLS REQUIRED ${SEARCH_QUIET})
  find_package(ZLIB REQUIRED ${SEARCH_QUIET})

  # Eventually we will want Find modules for the following deps
  # bzip2 

  set(CMAKE_ARGS -DBUILD_DOC=OFF
                 -DBUILD_EXAMPLES=OFF
                 -DBUILD_REGRESS=OFF
                 -DBUILD_SHARED_LIBS=OFF
                 -DBUILD_TOOLS=OFF)

  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})

  BUILD_DEP_TARGET()

  # Todo: Gnutls dependency
  add_dependencies(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} LIBRARY::ZLIB)

  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES LIBRARY::ZLIB)
endif()

if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
  # cmake target and not building internal
  if(TARGET libzip::zip AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})

    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS libzip::zip)

    # ToDo: When we correctly import dependencies cmake targets for the following
    # BZip2::BZip2, LibLZMA::LibLZMA, GnuTLS::GnuTLS, Nettle::Nettle
    # For now, we just override 
    set_target_properties(libzip::zip PROPERTIES
                                      INTERFACE_LINK_LIBRARIES "LIBRARY::ZLIB")
  else()
    SETUP_BUILD_TARGET()

    add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
  endif()
else()
  if(LibZip_FIND_REQUIRED)
    message(FATAL_ERROR "LibZip not found.")
  endif()
endif()
