#.rst:
# FindBrotli
# ----------
# Finds the Brotli library
#
# This will define the following target ALIAS:
#
#   LIBRARY::Brotli   - The Brotli library
#
# The following IMPORTED targets are made
#
#   brotli::brotlicommon - The brotlicommon library
#   brotli::brotlidec - The brotlidec library
#

if(NOT TARGET LIBRARY::${CMAKE_FIND_PACKAGE_NAME})

  macro(buildmacroBrotli)

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/01-all-disable-exe.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/02-all-cmake-install-config.patch")

    generate_patchcommand("${patches}")

    set(CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF
                   -DBROTLI_DISABLE_TESTS=ON
                   -DBROTLI_DISABLE_EXE=ON)

    BUILD_DEP_TARGET()

    # Retrieve suffix of platform byproduct to apply to second brotli library
    string(REGEX REPLACE "^.*\\." "" _LIBEXT ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPRODUCT})
    if(NOT (WIN32 OR WINDOWS_STORE))
      set(_PREFIX "lib")
    endif()

    set(BROTLICOMMON_LIBRARY_RELEASE "${DEP_LOCATION}/lib/${_PREFIX}brotlicommon.${_LIBEXT}")
    set(BROTLIDEC_LIBRARY_RELEASE "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY}")

    # Todo: debug postfix libs for windows
    #       Will require patching nghttp2, as they do not use debug postfix for differentiation

  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC brotli)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  # Check for existing Brotli. If version >= BROTLI-VERSION file version, dont build
  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND Brotli_FIND_REQUIRED)
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  else()
    if(TARGET brotli::brotlicommon AND TARGET brotli::brotlidec)

      # This is for the case where a distro provides a non standard (Debug/Release) config type
      # convert this back to either DEBUG/RELEASE or just RELEASE
      # we only do this because we use find_package_handle_standard_args for config time output
      # and it isnt capable of handling TARGETS, so we have to extract the info
      set(brotli_targets brotli::brotlicommon
                         brotli::brotlidec)
      foreach(_target ${brotli_targets})

        string(REPLACE "brotli::" "" _target_name ${_target})
        string(TOUPPER ${_target_name} _target_name_UPPER)

        get_target_property(${_target_name_UPPER}_CONFIGURATIONS ${_target} IMPORTED_CONFIGURATIONS)
        foreach(_${_target_name_UPPER}_config IN LISTS ${_target_name_UPPER}_CONFIGURATIONS)
          # Some non standard config (eg None on Debian)
          # Just set to RELEASE var so select_library_configurations can continue to work its magic
          string(TOUPPER ${_${_target_name_UPPER}_config} _${_target_name_UPPER}_config_UPPER)
          if((NOT ${_${_target_name_UPPER}_config_UPPER} STREQUAL "RELEASE") AND
             (NOT ${_${_target_name_UPPER}_config_UPPER} STREQUAL "DEBUG"))
            get_target_property(${_target_name_UPPER}_LIBRARY_RELEASE ${_target} IMPORTED_LOCATION_${_${_target_name_UPPER}_config_UPPER})
          else()
            get_target_property(${_target_name_UPPER}_LIBRARY_${_${_target_name_UPPER}_config_UPPER} ${_target} IMPORTED_LOCATION_${_${_target_name_UPPER}_config_UPPER})
          endif()
        endforeach()
      endforeach()

      get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR brotli::brotlicommon INTERFACE_INCLUDE_DIRECTORIES)

    elseif(TARGET PkgConfig::brotlicommon AND TARGET PkgConfig::brotlidec)
      # First item is the full path of the library file found
      # pkg_check_modules does not populate a variable of the found library explicitly
      list(GET brotlicommon_LINK_LIBRARIES 0 BROTLICOMMON_LIBRARY_RELEASE)

      get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR PkgConfig::brotlidec INTERFACE_INCLUDE_DIRECTORIES)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION ${brotlicommon_VERSION})

      # First item is the full path of the library file found
      # pkg_check_modules does not populate a variable of the found library explicitly
      list(GET brotlidec_LINK_LIBRARIES 0 BROTLIDEC_LIBRARY_RELEASE)
    endif()
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(BROTLIDEC)
  unset(BROTLIDEC_LIBRARIES)

  include(SelectLibraryConfigurations)
  select_library_configurations(BROTLICOMMON)
  unset(BROTLICOMMON_LIBRARIES)

  if(NOT VERBOSE_FIND)
     set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
   endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Brotli
                                    REQUIRED_VARS BROTLICOMMON_LIBRARY BROTLIDEC_LIBRARY ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR
                                    VERSION_VAR ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION)

  if(Brotli_FOUND)
    if((TARGET brotli::brotlicommon AND TARGET brotli::brotlidec) AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS brotli::brotlidec)
    elseif(TARGET PkgConfig::brotlicommon AND TARGET PkgConfig::brotlidec AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::brotlidec)
    else()
      add_library(LIBRARY::brotlicommon UNKNOWN IMPORTED)
      set_target_properties(LIBRARY::brotlicommon PROPERTIES
                                                  IMPORTED_LOCATION "${BROTLICOMMON_LIBRARY}"
                                                  INTERFACE_INCLUDE_DIRECTORIES "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR}")
  
      add_library(LIBRARY::brotlidec UNKNOWN IMPORTED)
      set_target_properties(LIBRARY::brotlidec PROPERTIES
                                               IMPORTED_LOCATION "${BROTLIDEC_LIBRARY}"
                                               INTERFACE_LINK_LIBRARIES LIBRARY::brotlicommon
                                               INTERFACE_INCLUDE_DIRECTORIES "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR}")
  
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS LIBRARY::brotlidec)

      add_dependencies(LIBRARY::brotlidec ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})

      # We are building as a requirement, so set LIB_BUILD property to allow calling
      # modules to know we will be building, and they will want to rebuild as well.
      # Property must be set on actual TARGET and not the ALIAS
      set_target_properties(${aliased_target} PROPERTIES LIB_BUILD ON)
    endif()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(Brotli_FIND_REQUIRED)
      message(FATAL_ERROR "Brotli libraries were not found.")
    endif()
  endif()
endif()
