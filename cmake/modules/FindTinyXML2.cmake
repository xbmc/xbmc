#.rst:
# FindTinyXML2
# -----------
# Finds the TinyXML2 library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::TinyXML2   - The TinyXML2 library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildmacroTinyXML2)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX d)
  
    find_package(Patch MODULE REQUIRED ${SEARCH_QUIET})
  
    if(UNIX)
      # ancient patch (Apple/freebsd) fails to patch tinyxml2 CMakeLists.txt file due to it being crlf encoded
      # Strip crlf before applying patches.
      # Freebsd fails even harder and requires both .patch and CMakeLists.txt to be crlf stripped
      # possibly add requirement for freebsd on gpatch? Wouldnt need to copy/strip the patch file then
      set(PATCH_COMMAND sed -ie s|\\r\$|| ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME}/src/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME}/CMakeLists.txt
                COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/001-debug-pdb.patch ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME}/src/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME}/001-debug-pdb.patch
                COMMAND sed -ie s|\\r\$|| ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME}/src/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME}/001-debug-pdb.patch
                COMMAND ${PATCH_EXECUTABLE} -p1 -i ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME}/src/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME}/001-debug-pdb.patch)
    else()
      set(PATCH_COMMAND ${PATCH_EXECUTABLE} -p1 -i ${CMAKE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/001-debug-pdb.patch)
    endif()
  
    if(CMAKE_GENERATOR MATCHES "Visual Studio" OR CMAKE_GENERATOR STREQUAL Xcode)
      # Multiconfig generators fail due to file(GENERATE tinyxml.pc) command.
      # This patch makes it generate a distinct named pc file for each build type and rename
      # pc file on install
      list(APPEND PATCH_COMMAND COMMAND ${PATCH_EXECUTABLE} -p1 -i ${CMAKE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/002-multiconfig-gen-pkgconfig.patch)
    endif()
  
    set(CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
                   -DCMAKE_CXX_EXTENSIONS=${CMAKE_CXX_EXTENSIONS}
                   -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
                   -Dtinyxml2_BUILD_TESTING=OFF)
  
    BUILD_DEP_TARGET()
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC tinyxml2)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  # Check for existing TINYXML2. If version >= TINYXML2-VERSION file version, dont build
  # A corner case, but if a linux/freebsd user WANTS to build internal tinyxml2, build anyway
  if((${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_TINYXML2) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_TINYXML2))
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  else()
    if(TARGET tinyxml2::tinyxml2)
      # This is for the case where a distro provides a non standard (Debug/Release) config type
      # eg Debian's config file is tinyxml2ConfigTargets-none.cmake
      # convert this back to either DEBUG/RELEASE or just RELEASE
      # we only do this because we use find_package_handle_standard_args for config time output
      # and it isnt capable of handling TARGETS, so we have to extract the info
      get_target_property(_TINYXML2_CONFIGURATIONS tinyxml2::tinyxml2 IMPORTED_CONFIGURATIONS)
      if(_TINYXML2_CONFIGURATIONS)
        foreach(_tinyxml2_config IN LISTS _TINYXML2_CONFIGURATIONS)
          # Some non standard config (eg None on Debian)
          # Just set to RELEASE var so select_library_configurations can continue to work its magic
          string(TOUPPER ${_tinyxml2_config} _tinyxml2_config_UPPER)
          if((NOT ${_tinyxml2_config_UPPER} STREQUAL "RELEASE") AND
             (NOT ${_tinyxml2_config_UPPER} STREQUAL "DEBUG"))
            get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE tinyxml2::tinyxml2 IMPORTED_LOCATION_${_tinyxml2_config_UPPER})
          else()
            get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_${_tinyxml2_config_UPPER} tinyxml2::tinyxml2 IMPORTED_LOCATION_${_tinyxml2_config_UPPER})
          endif()
        endforeach()
      else()
        get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE tinyxml2::tinyxml2 IMPORTED_LOCATION)
      endif()

      # Need this, as we may only get the existing TARGET from system and not build or use pkg-config
      get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR tinyxml2::tinyxml2 INTERFACE_INCLUDE_DIRECTORIES)
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      # First item is the full path of the library file found
      # pkg_check_modules does not populate a variable of the found library explicitly
      list(GET ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_LINK_LIBRARIES 0 ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE)

      get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} INTERFACE_INCLUDE_DIRECTORIES)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION})
    endif()
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(${${CMAKE_FIND_PACKAGE_NAME}_MODULE})
  unset(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARIES)

  if(NOT VERBOSE_FIND)
     set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
   endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(TinyXML2
                                    REQUIRED_VARS ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR
                                    VERSION_VAR ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION)

  if(TinyXML2_FOUND)
    if(TARGET tinyxml2::tinyxml2 AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS tinyxml2::tinyxml2)
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    else()
      SETUP_BUILD_TARGET()

      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(TinyXML2_FIND_REQUIRED)
      message(FATAL_ERROR "TinyXML2 libraries were not found. You may want to try -DENABLE_INTERNAL_TINYXML2=ON")
    endif()
  endif()
endif()
