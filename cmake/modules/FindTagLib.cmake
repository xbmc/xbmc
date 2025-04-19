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

  macro(buildTagLib)  
    # Darwin systems use a system tbd that isnt found as a static lib
    # Other platforms when using ENABLE_INTERNAL_TAGLIB, we want the static lib
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
      # Requires cmake 3.24 for ZLIB_USE_STATIC_LIBS to actually do something
      set(ZLIB_USE_STATIC_LIBS ON)
    endif()
    find_package(ZLIB REQUIRED ${SEARCH_QUIET})
    find_package(Utfcpp REQUIRED ${SEARCH_QUIET})
  
    set(TAGLIB_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
  
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
  
    add_dependencies(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC} LIBRARY::Zlib)
    add_dependencies(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC} LIBRARY::Utfcpp)
    set(TAGLIB_LINK_LIBRARIES "LIBRARY::Zlib")
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

  # Check for existing TAGLIB. If version >= TAGLIB-VERSION file version, dont build
  # Taglib 2.0+ provides cmake configs
  find_package(taglib ${CONFIG_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} CONFIG ${SEARCH_QUIET}
                      HINTS ${DEPENDS_PATH}/lib/cmake
                      ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})

  # cmake config may not be available (taglib 1.x series)
  # fallback to pkgconfig for non windows platforms
  if(NOT taglib_FOUND)
    find_package(PkgConfig ${SEARCH_QUIET})

    if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWSSTORE))
      pkg_check_modules(taglib taglib${PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} ${SEARCH_QUIET} IMPORTED_TARGET)

    else()
      # Taglib installs a shell script for all platforms. This can provide version universally
      find_program(TAGLIB-CONFIG NAMES taglib-config taglib-config.cmd
                                 HINTS ${DEPENDS_PATH}/bin)
    
      if(TAGLIB-CONFIG)
        execute_process(COMMAND "${TAGLIB-CONFIG}" --version
                        OUTPUT_VARIABLE taglib_VERSION
                        OUTPUT_STRIP_TRAILING_WHITESPACE)
      endif()
    endif()
  endif()

  if((taglib_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_TAGLIB) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_TAGLIB) OR
     (DEFINED ${CMAKE_FIND_PACKAGE_NAME}_FORCE_BUILD))
    # Build Taglib
    buildTagLib()
  else()
    if(TARGET TagLib::tag)
      # This is for the case where a distro provides a non standard (Debug/Release) config type
      # eg Debian's config file is taglib-targets-none.cmake
      # convert this back to either DEBUG/RELEASE or just RELEASE
      # we only do this because we use find_package_handle_standard_args for config time output
      # and it isnt capable of handling TARGETS, so we have to extract the info
      get_target_property(_TAGLIB_CONFIGURATIONS TagLib::tag IMPORTED_CONFIGURATIONS)
      foreach(_taglib_config IN LISTS _TAGLIB_CONFIGURATIONS)
        # Some non standard config (eg None on Debian)
        # Just set to RELEASE var so select_library_configurations can continue to work its magic
        string(TOUPPER ${_taglib_config} _taglib_config_UPPER)
        if((NOT ${_taglib_config_UPPER} STREQUAL "RELEASE") AND
           (NOT ${_taglib_config_UPPER} STREQUAL "DEBUG"))
          get_target_property(TAGLIB_LIBRARY_RELEASE TagLib::tag IMPORTED_LOCATION_${_taglib_config_UPPER})
        else()
          get_target_property(TAGLIB_LIBRARY_${_taglib_config_UPPER} TagLib::tag IMPORTED_LOCATION_${_taglib_config_UPPER})
        endif()
      endforeach()

      get_target_property(TAGLIB_INCLUDE_DIR TagLib::tag INTERFACE_INCLUDE_DIRECTORIES)
    elseif(TARGET PkgConfig::taglib)
      # First item is the full path of the library file found
      # pkg_check_modules does not populate a variable of the found library explicitly
      list(GET taglib_LINK_LIBRARIES 0 TAGLIB_LIBRARY_RELEASE)

      get_target_property(TAGLIB_INCLUDE_DIR PkgConfig::taglib INTERFACE_INCLUDE_DIRECTORIES)
    endif()
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(TAGLIB)
  unset(TAGLIB_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(TagLib
                                    REQUIRED_VARS TAGLIB_LIBRARY TAGLIB_INCLUDE_DIR
                                    VERSION_VAR TAGLIB_VERSION)

  if(TagLib_FOUND)
    if(TARGET TagLib::tag AND NOT TARGET taglib)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS TagLib::tag)
    elseif(TARGET PkgConfig::TAGLIB AND NOT TARGET taglib)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::TAGLIB)
    else()
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       INTERFACE_INCLUDE_DIRECTORIES "${TAGLIB_INCLUDE_DIR}"
                                                                       INTERFACE_LINK_LIBRARIES "${TAGLIB_LINK_LIBRARIES}")

      if(TAGLIB_LIBRARY_RELEASE)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                         IMPORTED_CONFIGURATIONS RELEASE
                                                                         IMPORTED_LOCATION_RELEASE "${TAGLIB_LIBRARY_RELEASE}")
      endif()
      if(TAGLIB_LIBRARY_DEBUG)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                         IMPORTED_LOCATION_DEBUG "${TAGLIB_LIBRARY_DEBUG}")
        set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                              IMPORTED_CONFIGURATIONS DEBUG)
      endif()
    endif()

    if(TARGET taglib)
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} taglib)
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
      if(NOT TARGET taglib)
        buildTagLib()
        set_target_properties(taglib PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends taglib)
    endif()
  else()
    if(TagLib_FIND_REQUIRED)
      message(FATAL_ERROR "TagLib not found. You may want to try -DENABLE_INTERNAL_TAGLIB=ON")
    endif()
  endif()
endif()
