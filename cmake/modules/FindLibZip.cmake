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

# Check for existing lib
find_package(libzip CONFIG QUIET
                    HINTS ${DEPENDS_PATH}/lib
                    ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

if(NOT LIBZIP_FOUND OR libzip_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
  # Check for dependencies
  find_package(GnuTLS REQUIRED)

  # Eventually we will want Find modules for the following deps
  # bzip2 
  # ZLIB

  set(CMAKE_ARGS -DBUILD_DOC=OFF
                 -DBUILD_EXAMPLES=OFF
                 -DBUILD_REGRESS=OFF
                 -DBUILD_SHARED_LIBS=OFF
                 -DBUILD_TOOLS=OFF)

  set(LIBZIP_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})

  BUILD_DEP_TARGET()
else()
  # we only do this because we use find_package_handle_standard_args for config time output
  # and it isnt capable of handling TARGETS, so we have to extract the info
  get_target_property(_ZIP_CONFIGURATIONS libzip::zip IMPORTED_CONFIGURATIONS)
  foreach(_zip_config IN LISTS _ZIP_CONFIGURATIONS)
    # Some non standard config (eg None on Debian)
    # Just set to RELEASE var so select_library_configurations can continue to work its magic
    string(TOUPPER ${_zip_config} _zip_config_UPPER)
    if((NOT ${_zip_config_UPPER} STREQUAL "RELEASE") AND
       (NOT ${_zip_config_UPPER} STREQUAL "DEBUG"))
      get_target_property(ZIP_LIBRARY_RELEASE libzip::zip IMPORTED_LOCATION_${_zip_config_UPPER})
    else()
      get_target_property(ZIP_LIBRARY_${_zip_config_UPPER} libzip::zip IMPORTED_LOCATION_${_zip_config_UPPER})
    endif()
  endforeach()

  get_target_property(ZIP_INCLUDE_DIR libzip::zip INTERFACE_INCLUDE_DIRECTORIES)
  set(LIBZIP_VERSION ${libzip_VERSION})

  include(SelectLibraryConfigurations)
  select_library_configurations(LIBZIP)
  unset(LIBZIP_LIBRARIES)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibZip
                                  REQUIRED_VARS LIBZIP_LIBRARY LIBZIP_INCLUDE_DIR
                                  VERSION_VAR LIBZIP_VERSION)

if(LIBZIP_FOUND)
  # cmake target and not building internal
  if(TARGET libzip::zip AND NOT TARGET libzip)

    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS libzip::zip)

    # ToDo: When we correctly import dependencies cmake targets for the following
    # BZip2::BZip2, LibLZMA::LibLZMA, GnuTLS::GnuTLS, Nettle::Nettle,ZLIB::ZLIB
    # For now, we just override 
    set_target_properties(libzip::zip PROPERTIES
                                      INTERFACE_LINK_LIBRARIES "")
  else()
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)

    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${LIBZIP_INCLUDE_DIR}"
                                                                     IMPORTED_LOCATION "${LIBZIP_LIBRARY}")

    if(TARGET libzip)
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} libzip)
    endif()
  endif()
else()
  if(LibZip_FIND_REQUIRED)
    message(FATAL_ERROR "LibZip not found.")
  endif()
endif()
