#.rst:
# FindTagLib
# ----------
# Finds the TagLib library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::TagLib   - The TagLib library
#

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildmacroTagLib)
    # Darwin systems use a system tbd that isnt found as a static lib
    # Other platforms when using ENABLE_INTERNAL_TAGLIB, we want the static lib
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
      # Requires cmake 3.24 for ZLIB_USE_STATIC_LIBS to actually do something
      set(ZLIB_USE_STATIC_LIBS ON)
    endif()
    find_package(ZLIB REQUIRED ${SEARCH_QUIET})
    find_package(Utfcpp REQUIRED ${SEARCH_QUIET})
  
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
  
    if(WIN32 OR WINDOWS_STORE)
      set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/001-cmake-pdb-debug.patch")
      generate_patchcommand("${patches}")
  
      if(WINDOWS_STORE)
        set(EXTRA_ARGS -DPLATFORM_WINRT=ON)
      endif()
    endif()
  
    # Debug postfix only used for windows
    if(WIN32 OR WINDOWS_STORE)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX "d")
    endif()
  
    set(CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF
                   -DBUILD_EXAMPLES=OFF
                   -DBUILD_TESTING=OFF
                   -DBUILD_BINDINGS=OFF
                   ${EXTRA_ARGS})
  
    BUILD_DEP_TARGET()
  
    add_dependencies(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} LIBRARY::ZLIB
                                                                        LIBRARY::Utfcpp)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES "LIBRARY::ZLIB")
  endmacro()

  # If there is a potential this library can be built internally
  # Check its dependencies to allow forcing this lib to be built if one of its
  # dependencies requires being rebuilt
  if(ENABLE_INTERNAL_TAGLIB)
    # Dependency list of this find module for an INTERNAL build
    set(${CMAKE_FIND_PACKAGE_NAME}_DEPLIST Utfcpp)

    check_dependency_build(${CMAKE_FIND_PACKAGE_NAME} "${${CMAKE_FIND_PACKAGE_NAME}_DEPLIST}")
  endif()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC taglib)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  # Taglib 2.0+ provides cmake configs
  find_package(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} ${CONFIG_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} CONFIG ${SEARCH_QUIET}
                                                         HINTS ${DEPENDS_PATH}/lib/cmake
                                                         ${${CORE_SYSTEM_NAME}_SEARCH_CONFIG})

  # cmake config may not be available (taglib 1.x series)
  # fallback to pkgconfig for non windows platforms
  if(NOT ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    find_package(PkgConfig ${SEARCH_QUIET})

    if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWSSTORE))
      pkg_check_modules(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}${PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} ${SEARCH_QUIET} IMPORTED_TARGET)

    else()
      # Taglib installs a shell script for all platforms. This can provide version universally
      find_program(TAGLIB-CONFIG NAMES taglib-config taglib-config.cmd
                                 HINTS ${DEPENDS_PATH}/bin)
    
      if(TAGLIB-CONFIG)
        execute_process(COMMAND "${TAGLIB-CONFIG}" --version
                        OUTPUT_VARIABLE ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION
                        OUTPUT_STRIP_TRAILING_WHITESPACE)
      endif()
    endif()
  endif()

  if((${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_TAGLIB) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_TAGLIB) OR
     (DEFINED ${CMAKE_FIND_PACKAGE_NAME}_FORCE_BUILD))
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  else()
    if(TARGET TagLib::tag)
      # This is for the case where a distro provides a non standard (Debug/Release) config type
      # eg Debian's config file is taglib-targets-none.cmake
      # convert this back to either DEBUG/RELEASE or just RELEASE
      # we only do this because we use find_package_handle_standard_args for config time output
      # and it isnt capable of handling TARGETS, so we have to extract the info
      get_target_property(_TAGLIB_CONFIGURATIONS TagLib::tag IMPORTED_CONFIGURATIONS)
      if(_TAGLIB_CONFIGURATIONS)
        foreach(_taglib_config IN LISTS _TAGLIB_CONFIGURATIONS)
          # Some non standard config (eg None on Debian)
          # Just set to RELEASE var so select_library_configurations can continue to work its magic
          string(TOUPPER ${_taglib_config} _taglib_config_UPPER)
          if((NOT ${_taglib_config_UPPER} STREQUAL "RELEASE") AND
             (NOT ${_taglib_config_UPPER} STREQUAL "DEBUG"))
            get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE TagLib::tag IMPORTED_LOCATION_${_taglib_config_UPPER})
          else()
            get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_${_taglib_config_UPPER} TagLib::tag IMPORTED_LOCATION_${_taglib_config_UPPER})
          endif()
        endforeach()
      else()
        get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE TagLib::tag IMPORTED_LOCATION)
      endif()

      get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR TagLib::tag INTERFACE_INCLUDE_DIRECTORIES)
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      # First item is the full path of the library file found
      # pkg_check_modules does not populate a variable of the found library explicitly
      list(GET ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_LINK_LIBRARIES 0 ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE)

      get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} INTERFACE_INCLUDE_DIRECTORIES)
    endif()
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(${${CMAKE_FIND_PACKAGE_NAME}_MODULE})
  unset(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARIES)

  if(NOT VERBOSE_FIND)
     set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
   endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(TagLib
                                    REQUIRED_VARS ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR
                                    VERSION_VAR ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION)

  if(TagLib_FOUND)
    if(TARGET TagLib::tag AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS TagLib::tag)
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    else()
      SETUP_BUILD_TARGET()

      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(TagLib_FIND_REQUIRED)
      message(FATAL_ERROR "TagLib not found. You may want to try -DENABLE_INTERNAL_TAGLIB=ON")
    endif()
  endif()
endif()
