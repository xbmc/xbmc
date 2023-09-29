#.rst:
# FindTinyXML2
# -----------
# Finds the TinyXML2 library
#
# This will define the following target:
#
#   tinyxml2::tinyxml2   - The TinyXML2 library

macro(buildTinyXML2)
  set(TINYXML2_VERSION ${${MODULE}_VER})
  set(TINYXML2_DEBUG_POSTFIX d)

  find_package(Patch MODULE REQUIRED)

  if(UNIX)
    # ancient patch (Apple/freebsd) fails to patch tinyxml2 CMakeLists.txt file due to it being crlf encoded
    # Strip crlf before applying patches.
    # Freebsd fails even harder and requires both .patch and CMakeLists.txt to be crlf stripped
    # possibly add requirement for freebsd on gpatch? Wouldnt need to copy/strip the patch file then
    set(PATCH_COMMAND sed -ie s|\\r\$|| ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${MODULE_LC}/src/${MODULE_LC}/CMakeLists.txt
              COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/tools/depends/target/tinyxml2/001-debug-pdb.patch ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${MODULE_LC}/src/${MODULE_LC}/001-debug-pdb.patch
              COMMAND sed -ie s|\\r\$|| ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${MODULE_LC}/src/${MODULE_LC}/001-debug-pdb.patch
              COMMAND ${PATCH_EXECUTABLE} -p1 -i ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${MODULE_LC}/src/${MODULE_LC}/001-debug-pdb.patch)
  else()
    set(PATCH_COMMAND ${PATCH_EXECUTABLE} -p1 -i ${CMAKE_SOURCE_DIR}/tools/depends/target/tinyxml2/001-debug-pdb.patch)
  endif()

  if(CMAKE_GENERATOR MATCHES "Visual Studio" OR CMAKE_GENERATOR STREQUAL Xcode)
    # Multiconfig generators fail due to file(GENERATE tinyxml.pc) command.
    # This patch makes it generate a distinct named pc file for each build type and rename
    # pc file on install
    list(APPEND PATCH_COMMAND COMMAND ${PATCH_EXECUTABLE} -p1 -i ${CMAKE_SOURCE_DIR}/tools/depends/target/tinyxml2/002-multiconfig-gen-pkgconfig.patch)
  endif()

  set(CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
                 -DCMAKE_CXX_EXTENSIONS=${CMAKE_CXX_EXTENSIONS}
                 -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
                 -Dtinyxml2_BUILD_TESTING=OFF)

  BUILD_DEP_TARGET()
endmacro()

if(NOT TARGET tinyxml2::tinyxml2)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC tinyxml2)

  SETUP_BUILD_VARS()

  find_package(TINYXML2 CONFIG QUIET
                               HINTS ${DEPENDS_PATH}/lib/cmake
                               ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

  # Check for existing TINYXML2. If version >= TINYXML2-VERSION file version, dont build
  # A corner case, but if a linux/freebsd user WANTS to build internal tinyxml2, build anyway
  if((TINYXML2_VERSION VERSION_LESS ${${MODULE}_VER} AND ENABLE_INTERNAL_TINYXML2) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_TINYXML2))

    buildTinyXML2()
  else()
    # This is the fallback case where linux distro's dont ship cmake config files
    # use the old find_library way. Only do this if we didnt find a cmake config 
    # in the event of the version < depends version
    if(NOT TARGET tinyxml2::tinyxml2)
      if(PKG_CONFIG_FOUND)
        pkg_check_modules(PC_TINYXML2 tinyxml2 QUIET)
      endif()

      find_path(TINYXML2_INCLUDE_DIR NAMES tinyxml2.h
                                     HINTS ${DEPENDS_PATH}/include ${PC_TINYXML2_INCLUDEDIR}
                                     ${${CORE_PLATFORM_LC}_SEARCH_CONFIG}
                                     NO_CACHE)
      find_library(TINYXML2_LIBRARY_RELEASE NAMES tinyxml2
                                            HINTS ${DEPENDS_PATH}/lib ${PC_TINYXML2_LIBDIR}
                                            ${${CORE_PLATFORM_LC}_SEARCH_CONFIG}
                                            NO_CACHE)
      find_library(TINYXML2_LIBRARY_DEBUG NAMES tinyxml2d
                                          HINTS ${DEPENDS_PATH}/lib ${PC_TINYXML2_LIBDIR}
                                          ${${CORE_PLATFORM_LC}_SEARCH_CONFIG}
                                          NO_CACHE)

      set(TINYXML2_VERSION ${PC_TINYXML2_VERSION})
    endif()
  endif()

  if(TARGET tinyxml2::tinyxml2)
    # This is for the case where a distro provides a non standard (Debug/Release) config type
    # eg Debian's config file is tinyxml2ConfigTargets-none.cmake
    # convert this back to either DEBUG/RELEASE or just RELEASE
    # we only do this because we use find_package_handle_standard_args for config time output
    # and it isnt capable of handling TARGETS, so we have to extract the info
    get_target_property(_TINYXML2_CONFIGURATIONS tinyxml2::tinyxml2 IMPORTED_CONFIGURATIONS)
    foreach(_tinyxml2_config IN LISTS _TINYXML2_CONFIGURATIONS)
      # Some non standard config (eg None on Debian)
      # Just set to RELEASE var so select_library_configurations can continue to work its magic
      string(TOUPPER ${_tinyxml2_config} _tinyxml2_config_UPPER)
      if((NOT ${_tinyxml2_config_UPPER} STREQUAL "RELEASE") AND
         (NOT ${_tinyxml2_config_UPPER} STREQUAL "DEBUG"))
        get_target_property(TINYXML2_LIBRARY_RELEASE tinyxml2::tinyxml2 IMPORTED_LOCATION_${_tinyxml2_config_UPPER})
      else()
        get_target_property(TINYXML2_LIBRARY_${_tinyxml2_config_UPPER} tinyxml2::tinyxml2 IMPORTED_LOCATION_${_tinyxml2_config_UPPER})
      endif()
    endforeach()

    # Need this, as we may only get the existing TARGET from system and not build or use pkg-config
    get_target_property(TINYXML2_INCLUDE_DIR tinyxml2::tinyxml2 INTERFACE_INCLUDE_DIRECTORIES)
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(TINYXML2)
  unset(TINYXML2_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(TinyXML2
                                    REQUIRED_VARS TINYXML2_LIBRARY TINYXML2_INCLUDE_DIR
                                    VERSION_VAR TINYXML2_VERSION)

  if(TinyXML2_FOUND)
    if(NOT TARGET tinyxml2::tinyxml2)
      add_library(tinyxml2::tinyxml2 UNKNOWN IMPORTED)
      if(TINYXML2_LIBRARY_RELEASE)
        set_target_properties(tinyxml2::tinyxml2 PROPERTIES
                                                 IMPORTED_CONFIGURATIONS RELEASE
                                                 IMPORTED_LOCATION_RELEASE "${TINYXML2_LIBRARY_RELEASE}")
      endif()
      if(TINYXML2_LIBRARY_DEBUG)
        set_target_properties(tinyxml2::tinyxml2 PROPERTIES
                                                 IMPORTED_CONFIGURATIONS DEBUG
                                                 IMPORTED_LOCATION_DEBUG "${TINYXML2_LIBRARY_DEBUG}")
      endif()
      set_target_properties(tinyxml2::tinyxml2 PROPERTIES
                                               INTERFACE_INCLUDE_DIRECTORIES "${TINYXML2_INCLUDE_DIR}")
    endif()

    if(TARGET tinyxml2)
      add_dependencies(tinyxml2::tinyxml2 tinyxml2)
    endif()

    # Add internal build target when a Multi Config Generator is used
    # We cant add a dependency based off a generator expression for targeted build types,
    # https://gitlab.kitware.com/cmake/cmake/-/issues/19467
    # therefore if the find heuristics only find the library, we add the internal build
    # target to the project to allow user to manually trigger for any build type they need
    # in case only a specific build type is actually available (eg Release found, Debug Required)
    # This is mainly targeted for windows who required different runtime libs for different
    # types, and they arent compatible
    if(_multiconfig_generator)
      if(NOT TARGET tinyxml2)
        buildTinyXML2()
        set_target_properties(tinyxml2 PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends tinyxml2)
    endif()

  endif()

  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP tinyxml2::tinyxml2)
endif()
