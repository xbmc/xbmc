#.rst:
# FindTagLib
# ----------
# Finds the TagLib library
#
# This will define the following target:
#
#   TagLib::TagLib   - The TagLib library
#

macro(buildTagLib)
  # Suppress mismatch warning, see https://cmake.org/cmake/help/latest/module/FindPackageHandleStandardArgs.html
  set(FPHSA_NAME_MISMATCHED 1)

  # Darwin systems use a system tbd that isnt found as a static lib
  # Other platforms when using ENABLE_INTERNAL_TAGLIB, we want the static lib
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # Requires cmake 3.24 for ZLIB_USE_STATIC_LIBS to actually do something
    set(ZLIB_USE_STATIC_LIBS ON)
  endif()
  find_package(ZLIB REQUIRED)
  unset(FPHSA_NAME_MISMATCHED)

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC taglib)

  SETUP_BUILD_VARS()

  set(TAGLIB_VERSION ${${MODULE}_VER})

  if(WIN32 OR WINDOWS_STORE)
    set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/001-cmake-pdb-debug.patch")
    generate_patchcommand("${patches}")

    if(WINDOWS_STORE)
      set(EXTRA_ARGS -DPLATFORM_WINRT=ON)
    endif()
  endif()

  # Debug postfix only used for windows
  if(WIN32 OR WINDOWS_STORE)
    set(TAGLIB_DEBUG_POSTFIX "d")
  endif()

  set(CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF
                 -DBUILD_EXAMPLES=OFF
                 -DBUILD_TESTING=OFF
                 -DBUILD_BINDINGS=OFF
                 ${EXTRA_ARGS})

  BUILD_DEP_TARGET()

  add_dependencies(${MODULE_LC} ZLIB::ZLIB)
endmacro()

if(NOT TARGET TagLib::TagLib)
  if(ENABLE_INTERNAL_TAGLIB)
    buildTagLib()
  else()

    if(PKG_CONFIG_FOUND)
      pkg_check_modules(PC_TAGLIB taglib>=1.9.0 QUIET)
    endif()

    find_path(TAGLIB_INCLUDE_DIR NAMES taglib/tag.h
                                 HINTS ${DEPENDS_PATH}/include ${PC_TAGLIB_INCLUDEDIR}
                                 ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG}
                                 NO_CACHE)
    find_library(TAGLIB_LIBRARY_RELEASE NAMES tag
                                        HINTS ${DEPENDS_PATH}/lib ${PC_TAGLIB_LIBDIR}
                                        ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG}
                                        NO_CACHE)
    find_library(TAGLIB_LIBRARY_DEBUG NAMES tagd
                                      HINTS ${DEPENDS_PATH}/lib ${PC_TAGLIB_LIBDIR}
                                      ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG}
                                      NO_CACHE)
    set(TAGLIB_VERSION ${PC_TAGLIB_VERSION})

    set(TAGLIB_LINK_LIBS ${PC_TAGLIB_LIBRARIES})
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(TAGLIB)
  unset(TAGLIB_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(TagLib
                                    REQUIRED_VARS TAGLIB_LIBRARY TAGLIB_INCLUDE_DIR
                                    VERSION_VAR TAGLIB_VERSION)

  if(TagLib_FOUND)
    add_library(TagLib::TagLib UNKNOWN IMPORTED)
    if(TAGLIB_LIBRARY_RELEASE)
      set_target_properties(TagLib::TagLib PROPERTIES
                                           IMPORTED_CONFIGURATIONS RELEASE
                                           IMPORTED_LOCATION_RELEASE "${TAGLIB_LIBRARY_RELEASE}")
    endif()
    if(TAGLIB_LIBRARY_DEBUG)
      set_target_properties(TagLib::TagLib PROPERTIES
                                           IMPORTED_CONFIGURATIONS DEBUG
                                           IMPORTED_LOCATION_DEBUG "${TAGLIB_LIBRARY_DEBUG}")
    endif()
    set_target_properties(TagLib::TagLib PROPERTIES
                                         INTERFACE_INCLUDE_DIRECTORIES "${TAGLIB_INCLUDE_DIR}")

    # if pkg-config returns link libs at to TARGET. For internal build, we use ZLIB::Zlib
    # dependency explicitly
    if(TAGLIB_LINK_LIBS)
        set_target_properties(TagLib::TagLib PROPERTIES
                                             INTERFACE_LINK_LIBRARIES "${TAGLIB_LINK_LIBS}")
    endif()

    if(TARGET taglib)
      add_dependencies(TagLib::TagLib taglib)
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


    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP TagLib::TagLib)
  else()
    if(TagLib_FIND_REQUIRED)
      message(FATAL_ERROR "TagLib not found.")
    endif()
  endif()
  mark_as_advanced(TAGLIB_INCLUDE_DIR TAGLIB_LIBRARY)
endif()

