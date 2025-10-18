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

# Check for existing lib
find_package(libzip ${CONFIG_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} CONFIG ${SEARCH_QUIET}
                    HINTS ${DEPENDS_PATH}/lib
                    ${${CORE_SYSTEM_NAME}_SEARCH_CONFIG})

if(libzip_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
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
else()
  # we only do this because we use find_package_handle_standard_args for config time output
  # and it isnt capable of handling TARGETS, so we have to extract the info
  get_target_property(_ZIP_CONFIGURATIONS libzip::zip IMPORTED_CONFIGURATIONS)
  if(_ZIP_CONFIGURATIONS)
    foreach(_zip_config IN LISTS _ZIP_CONFIGURATIONS)
      # Just set to RELEASE var so select_library_configurations can continue to work its magic
      string(TOUPPER ${_zip_config} _zip_config_UPPER)
      if((NOT ${_zip_config_UPPER} STREQUAL "RELEASE") AND
         (NOT ${_zip_config_UPPER} STREQUAL "DEBUG"))
        get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE libzip::zip IMPORTED_LOCATION_${_zip_config_UPPER})
      else()
        get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_${_zip_config_UPPER} libzip::zip IMPORTED_LOCATION_${_zip_config_UPPER})
      endif()
    endforeach()
  else()
    get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE libzip::zip IMPORTED_LOCATION)
  endif()

  get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR libzip::zip INTERFACE_INCLUDE_DIRECTORIES)
  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION ${libzip_VERSION})
endif()

include(SelectLibraryConfigurations)
select_library_configurations(${${CMAKE_FIND_PACKAGE_NAME}_MODULE})
unset(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARIES)

if(NOT VERBOSE_FIND)
   set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
 endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibZip
                                  REQUIRED_VARS ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR
                                  VERSION_VAR ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION)

if(LibZip_FOUND)
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
