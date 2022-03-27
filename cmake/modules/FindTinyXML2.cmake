#.rst:
# FindTinyXML2
# -----------
# Finds the TinyXML2 library
#
#
# This will define the following target:
#
#   tinyxml2::tinyxml2   - The TinyXML2 library

if(NOT TARGET tinyxml2::tinyxml2)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC tinyxml2)

  SETUP_BUILD_VARS()

  # Check for existing TINYXML2. If version >= TINYXML2-VERSION file version, dont build
  find_package(TINYXML2 CONFIG)

  # Some linux distro's dont package cmake config files for TinyXML2
  # This means that they will fall into the below and we will run a pkg_check_modules
  # for one last search
  if(TINYXML2_VERSION VERSION_LESS ${${MODULE}_VER})

    if(ENABLE_INTERNAL_TINYXML2)
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
    else()
      # This is the fallback case where linux distro's dont ship cmake config files
      # use the old find_library way

      if(PKG_CONFIG_FOUND)
        pkg_check_modules(PC_TINYXML2 tinyxml2 QUIET)
      endif()

      find_path(TINYXML2_INCLUDE_DIR tinyxml2.h
                                    PATHS ${PC_TINYXML2_INCLUDEDIR})
      find_library(TINYXML2_LIBRARY_RELEASE NAMES tinyxml2
                                           PATHS ${PC_TINYXML2_LIBDIR})
      find_library(TINYXML2_LIBRARY_DEBUG NAMES tinyxml2d
                                         PATHS ${PC_TINYXML2_LIBDIR})

      set(TINYXML2_VERSION ${PC_TINYXML2_VERSION})

    endif()

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

    if(TARGET tinyxml2)
      add_dependencies(tinyxml2::tinyxml2 tinyxml2)
    endif()
  endif()

  if(TARGET tinyxml2::tinyxml2)
    # Used to provide configure output of target information
    get_target_property(TINYXML2_LIBRARY_DEBUG tinyxml2::tinyxml2 IMPORTED_LOCATION_DEBUG)
    get_target_property(TINYXML2_LIBRARY_RELEASE tinyxml2::tinyxml2 IMPORTED_LOCATION_RELEASE)
    get_target_property(TINYXML2_INCLUDE_DIR tinyxml2::tinyxml2 INTERFACE_INCLUDE_DIRECTORIES)
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(TINYXML2)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(TinyXML2
                                    REQUIRED_VARS TINYXML2_LIBRARY TINYXML2_INCLUDE_DIR
                                    VERSION_VAR TINYXML2_VERSION)

  # Check whether we already have tinyxml2::tinyxml2 target added to dep property list
  get_property(CHECK_INTERNAL_DEPS GLOBAL PROPERTY INTERNAL_DEPS_PROP)
  list(FIND CHECK_INTERNAL_DEPS "tinyxml2::tinyxml2" TINYXML2_PROP_FOUND)

  # list(FIND) returns -1 if search item not found
  if(TINYXML2_PROP_FOUND STREQUAL "-1")
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP tinyxml2::tinyxml2)
  endif()

endif()
