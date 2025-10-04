# FindPkgConfig
# --------
# Find the PkgConfig executable
#
# This will define the following target:
#
#   PkgConfig::PkgConfig - The PkgConfig executable
#   PKG_CONFIG_EXECUTABLE - Non TARGET cache variable of the executable

if(NOT TARGET PkgConfig::PkgConfig)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC pkgconf)
  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_LIB_TYPE native)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  # prioritise pkgconf for windows. we want to avoid pkg-config wrappers provided by things like strawberry perl
  if(WIN32 OR WINDOWS_STORE)
    set(search_pkgconf pkgconf)
  endif()

  # Check for existing bin.
  find_program(search_pkgconf_executable NAMES ${search_pkgconf} pkg-config
                                         HINTS ${NATIVEPREFIX}/bin)

  if(search_pkgconf_executable)
    execute_process(COMMAND "${search_pkgconf_executable}" --version
                    OUTPUT_VARIABLE ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()

  # This setup builds and installs pkgconf at configure time
  # This allows the pkgconf binary to be used during the rest of configure
  # Technically this can be used by unix platforms, but we specifically gate this to windows for now
  if((NOT search_pkgconf_executable) OR
     ((${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}) AND
      (WIN32 OR WINDOWS_STORE)))

    # Need the working directory to exist prior to the execute_process
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/pkgconf)
    # Configure
    execute_process(
      COMMAND ${CMAKE_COMMAND} ${CMAKE_SOURCE_DIR}/tools/depends/native/pkgconf
                               -DKODI_MIRROR=${KODI_MIRROR}
                               -DKODI_SOURCE_DIR=${CMAKE_SOURCE_DIR}
                               -DTARBALL_DIR=${TARBALL_DIR}
                               -DNATIVEPREFIX=${NATIVEPREFIX}
                               -DCMAKE_MODULE_PATH=${CMAKE_SOURCE_DIR}/cmake/modules/buildtools
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/pkgconf
      RESULT_VARIABLE _pkgconf_config_result
    )

    if(_pkgconf_config_result AND NOT _pkgconf_config_result EQUAL 0)
      message(FATAL_ERROR "pkgconf configuration failed: ${_pkgconf_config_result}")
    endif()

    # build
    execute_process(
      COMMAND ${CMAKE_COMMAND} --build .
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/pkgconf
      RESULT_VARIABLE _pkgconf_build_result
    )

    if(_pkgconf_build_result AND NOT _pkgconf_build_result EQUAL 0)
      message(FATAL_ERROR "pkgconf build failed: ${_pkgconf_build_result}")
    endif()

    set(search_pkgconf_executable ${NATIVEPREFIX}/bin/pkgconf${CMAKE_HOST_EXECUTABLE_SUFFIX})
    set(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
  endif()

  # Provide standardized success/failure messages
  find_package_handle_standard_args(PkgConfig
                                    REQUIRED_VARS search_pkgconf_executable
                                    VERSION_VAR ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION)

  if(PkgConfig_FOUND)
    add_executable(PkgConfig::PkgConfig IMPORTED)
    set_target_properties(PkgConfig::PkgConfig PROPERTIES
                                             IMPORTED_LOCATION "${search_pkgconf_executable}"
                                             FOLDER "External Projects")

    set(PKG_CONFIG_EXECUTABLE ${search_pkgconf_executable} CACHE FILEPATH "Pkgconfig/pkgconf executable")
    set(PKG_CONFIG_FOUND 1 CACHE BOOL "Pkgconfig/pkgconf Found")

    # We do this dance to utilise macros created in system FindPkgConfig
    set(_temp_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})
    unset(CMAKE_MODULE_PATH)

    include(FindPkgConfig)

    # Back to our normal module paths
    set(CMAKE_MODULE_PATH ${_temp_CMAKE_MODULE_PATH})
  endif()
endif()
