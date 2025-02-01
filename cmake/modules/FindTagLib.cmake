#.rst:
# FindTagLib
# ----------
# Finds the TagLib library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::TagLib   - The TagLib library
#

macro(buildTagLib)

  find_package(Utfcpp REQUIRED QUIET)

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

  add_dependencies(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC} ZLIB::ZLIB)
  add_dependencies(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC} utf8cpp::utf8cpp)
  set(TAGLIB_LINK_LIBRARIES "ZLIB::ZLIB")
endmacro()

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC taglib)

  SETUP_BUILD_VARS()

  # Taglib installs a shell script for all platforms. This can provide version universally
  find_program(TAGLIB-CONFIG NAMES taglib-config taglib-config.cmd
                             HINTS ${DEPENDS_PATH}/bin)

  if(TAGLIB-CONFIG)
    execute_process(COMMAND "${TAGLIB-CONFIG}" --version
                    OUTPUT_VARIABLE TAGLIBCONFIG_VER
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()

  if((TAGLIBCONFIG_VER VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_TAGLIB) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_TAGLIB))
    # Build Taglib
    buildTagLib()
  else()
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWS_STORE))
      if(TagLib_FIND_VERSION)
        if(TagLib_FIND_VERSION_EXACT)
          set(TagLib_FIND_SPEC "=${TagLib_FIND_VERSION_COMPLETE}")
        else()
          set(TagLib_FIND_SPEC ">=${TagLib_FIND_VERSION_COMPLETE}")
        endif()
      endif()
      pkg_check_modules(TAGLIB taglib${TagLib_FIND_SPEC} QUIET)
    endif()

    find_path(TAGLIB_INCLUDE_DIR NAMES taglib/tag.h
                                 HINTS ${DEPENDS_PATH}/include ${TAGLIB_INCLUDEDIR}
                                 ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})
    find_library(TAGLIB_LIBRARY_RELEASE NAMES tag
                                        HINTS ${DEPENDS_PATH}/lib ${TAGLIB_LIBDIR}
                                        ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})
    find_library(TAGLIB_LIBRARY_DEBUG NAMES tagd
                                      HINTS ${DEPENDS_PATH}/lib ${TAGLIB_LIBDIR}
                                      ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(TAGLIB)
  unset(TAGLIB_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(TagLib
                                    REQUIRED_VARS TAGLIB_LIBRARY TAGLIB_INCLUDE_DIR
                                    VERSION_VAR TAGLIB_VERSION)

  if(TagLib_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
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
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${TAGLIB_INCLUDE_DIR}")

    # if pkg-config returns link libs add to TARGET.
    if(TAGLIB_LINK_LIBRARIES)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                         INTERFACE_LINK_LIBRARIES "${TAGLIB_LINK_LIBRARIES}")
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
