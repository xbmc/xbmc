# This script provides helper functions for FindModules

# Parse and set variables from VERSION dependency file
# On return:
#   MODULENAME_ARCHIVE will be set to parent scope
#   MODULENAME_VER will be set to parent scope (eg FFMPEG_VER, DAV1D_VER)
#   MODULENAME_BASE_URL will be set to parent scope if exists in VERSION file (eg FFMPEG_BASE_URL)
#   MODULENAME_HASH will be set if either SHA256 or SHA512 exists in VERSION file
#   MODULENAME_BYPRODUCT will be set to parent scope
function(get_versionfile_data)

  # Dependency path
  set(MODULE_PATH "${PROJECTSOURCE}/tools/depends/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_LIB_TYPE}/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_MODULE_LOCATION}")

  if(NOT EXISTS "${MODULE_PATH}/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_MODULE_VERSION}-VERSION")
    MESSAGE(FATAL_ERROR "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_MODULE_VERSION}-VERSION does not exist at ${MODULE_PATH}.")
  else()
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_FILE "${MODULE_PATH}/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_MODULE_VERSION}-VERSION")
  endif()

  file(STRINGS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_FILE} ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LNAME REGEX "^[ \t]*LIBNAME=")
  file(STRINGS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_FILE} ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER REGEX "^[ \t]*VERSION=")
  file(STRINGS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_FILE} ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_ARCHIVE REGEX "^[ \t]*ARCHIVE=")
  file(STRINGS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_FILE} ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BASE_URL REGEX "^[ \t]*BASE_URL=")
  file(STRINGS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_FILE} ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_FULL_URL REGEX "^[ \t]*FULL_URL=")

  if(WIN32 OR WINDOWS_STORE)
    file(STRINGS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_FILE} ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPRODUCT REGEX "^[ \t]*BYPRODUCT_WIN=")
  else()
    file(STRINGS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_FILE} ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPRODUCT REGEX "^[ \t]*BYPRODUCT=")
  endif()

  # Tarball Hash
  file(STRINGS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_FILE} ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_HASH_SHA256 REGEX "^[ \t]*SHA256=")
  file(STRINGS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_FILE} ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_HASH_SHA512 REGEX "^[ \t]*SHA512=")

  string(REGEX REPLACE ".*LIBNAME=([^ \t]*).*" "\\1" ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LNAME "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LNAME}")
  string(REGEX REPLACE ".*VERSION=([^ \t]*).*" "\\1" ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}")
  string(REGEX REPLACE ".*ARCHIVE=([^ \t]*).*" "\\1" ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_ARCHIVE "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_ARCHIVE}")
  string(REGEX REPLACE ".*BASE_URL=([^ \t]*).*" "\\1" ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BASE_URL "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BASE_URL}")
  string(REGEX REPLACE ".*FULL_URL=([^ \t]*).*" "\\1" ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_FULL_URL "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_FULL_URL}")
  if(WIN32 OR WINDOWS_STORE)
    string(REGEX REPLACE ".*BYPRODUCT_WIN=([^ \t]*).*" "\\1" ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPRODUCT "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPRODUCT}")
  else()
    string(REGEX REPLACE ".*BYPRODUCT=([^ \t]*).*" "\\1" ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPRODUCT "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPRODUCT}")
  endif()

  string(REGEX REPLACE "\\$\\(LIBNAME\\)" "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LNAME}" ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_ARCHIVE "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_ARCHIVE}")
  string(REGEX REPLACE "\\$\\(VERSION\\)" "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}" ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_ARCHIVE "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_ARCHIVE}")

  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_ARCHIVE ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_ARCHIVE} PARENT_SCOPE)

  # FULL_URL substitutes the following VERSION, BASE_URL
  # FULL_URL syntax: FULL_URL=$(BASE_URL)/archive/$(VERSION).tar.gz
  string(REGEX REPLACE "\\$\\(BASE_URL\\)" "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BASE_URL}" ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_FULL_URL "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_FULL_URL}")
  string(REGEX REPLACE "\\$\\(VERSION\\)" "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}" ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_FULL_URL "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_FULL_URL}")

  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_FULL_URL ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_FULL_URL} PARENT_SCOPE)

  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} PARENT_SCOPE)

  if (${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BASE_URL)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BASE_URL ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BASE_URL} PARENT_SCOPE)
  else()
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BASE_URL "${KODI_MIRROR}/build-deps/sources" PARENT_SCOPE)
  endif()
  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPRODUCT ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPRODUCT} PARENT_SCOPE)

  # allow user to override the download URL hash with a local tarball hash
  # needed for offline build envs
  if (NOT DEFINED ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_HASH)
    if (${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_HASH_SHA256)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_HASH ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_HASH_SHA256} PARENT_SCOPE)
    elseif(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_HASH_SHA512)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_HASH ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_HASH_SHA512} PARENT_SCOPE)
    endif()
  endif()
endfunction()

# Parse and set Version from VERSION dependency file
# Used for retrieving version numbers for dependency libs to allow setting
# a required version for find_package call
# On return:
#   LIB_MODULENAME_VER will be set to parent scope (eg LIB_FMT_VER)
function(get_libversion_data module libtype)

  # Dependency path
  set(LIB_MODULE_PATH "${CMAKE_SOURCE_DIR}/tools/depends/${libtype}/${module}")
  string(TOUPPER ${module} MOD_UPPER)

  if(NOT EXISTS "${LIB_MODULE_PATH}/${MOD_UPPER}-VERSION")
    MESSAGE(FATAL_ERROR "${MOD_UPPER}-VERSION does not exist at ${LIB_MODULE_PATH}.")
  else()
    set(${MOD_UPPER}_FILE "${LIB_MODULE_PATH}/${MOD_UPPER}-VERSION")
  endif()

  file(STRINGS ${${MOD_UPPER}_FILE} ${MOD_UPPER}_VER REGEX "^[ \t]*VERSION=")

  string(REGEX REPLACE ".*VERSION=([^ \t]*).*" "\\1" ${MOD_UPPER}_VER "${${MOD_UPPER}_VER}")

  set(LIB_${MOD_UPPER}_VER ${${MOD_UPPER}_VER} PARENT_SCOPE)
endfunction()

# Function to loop through list of patch files (full path)
# Sets to a PATCH_COMMAND variable and set to parent scope (caller)
# Used to test windows line endings and set appropriate patch commands
function(generate_patchcommand _patchlist)
  # find the path to the patch executable
  find_package(Patch MODULE REQUIRED ${SEARCH_QUIET})

  # Loop through patches and add to PATCH_COMMAND
  # for windows, check CRLF/LF state

  set(_count 0)
  foreach(patch ${_patchlist})
    if(WIN32 OR WINDOWS_STORE)
      PATCH_LF_CHECK(${patch})
    endif()
    if(${_count} EQUAL "0")
      set(_patch_command ${PATCH_EXECUTABLE} -p1 -i ${patch})
    else()
      list(APPEND _patch_command COMMAND ${PATCH_EXECUTABLE} -p1 -i ${patch})
    endif()

    math(EXPR _count "${_count}+1")
  endforeach()
  set(PATCH_COMMAND ${_patch_command} PARENT_SCOPE)
  unset(_count)
  unset(_patch_command)
endfunction()

# Macro to factor out the repetitive URL setup
macro(SETUP_BUILD_VARS)
  string(TOUPPER ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC} ${CMAKE_FIND_PACKAGE_NAME}_MODULE)

  if(DEFINED ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_MODULE_LOCATION)
    string(TOUPPER ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_MODULE_LOCATION} _MODULE_UPPER)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_MODULE_VERSION ${_MODULE_UPPER})
    unset(_MODULE_UPPER)
  else()
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_MODULE_LOCATION ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC})
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_MODULE_VERSION ${${CMAKE_FIND_PACKAGE_NAME}_MODULE})
  endif()

  if(NOT DEFINED ${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME)
    set(${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC})
  endif()

  if(NOT DEFINED ${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME_PC)
    set(${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME_PC ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
  endif()

  # Fall through to target build module dir if not explicitly set
  if(NOT DEFINED ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_LIB_TYPE)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_LIB_TYPE "target")
  endif()

  if(NOT ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME build-${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC})
  endif()

  # Location for build type, native or target
  if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_LIB_TYPE STREQUAL "target")
    set(DEP_LOCATION "${DEPENDS_PATH}")
  else()
    set(DEP_LOCATION "${NATIVEPREFIX}")
  endif()

  # PROJECTSOURCE used in native toolchain to provide core project sourcedir
  # to externalproject_add targets that have a different CMAKE_SOURCE_DIR (eg jsonschema/texturepacker in-tree)
  if(NOT PROJECTSOURCE)
    set(PROJECTSOURCE ${CMAKE_SOURCE_DIR})
  endif()

  if(NOT ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_DISABLE_VERSION)
    # populate variables of data from VERSION file for MODULE
    get_versionfile_data()
  endif()

  if(VERBOSE)
    message(STATUS "MODULE: ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}")
    message(STATUS "${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_LIB_TYPE: ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_LIB_TYPE}")
    message(STATUS "DEP_LOCATION: ${DEP_LOCATION}")
    message(STATUS "PROJECTSOURCE: ${PROJECTSOURCE}")
  endif()
endmacro()

macro(CLEAR_BUILD_VARS)
  # unset all generic variables to insure clean state between macro calls
  # Potentially an issue with scope when a macro is used inside a dep that uses a macro
  unset(PROJECTSOURCE)
  unset(INSTALL_DIR)
  unset(CMAKE_ARGS)
  unset(PATCH_COMMAND)
  unset(CONFIGURE_COMMAND)
  unset(BUILD_COMMAND)
  unset(INSTALL_COMMAND)
  unset(BUILD_IN_SOURCE)
  unset(BUILD_BYPRODUCTS)
  unset(WIN_DISABLE_PROJECT_FLAGS)

  # unset all module specific variables to insure clean state between macro calls
  # potentially an issue when a native and a target of the same module exists
  unset(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIST_SEPARATOR)
  unset(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_GENERATOR)
  unset(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_GENERATOR_PLATFORM)
  unset(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INSTALL_PREFIX)
  unset(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_TOOLCHAIN_FILE)
  unset(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_LIB_TYPE)
endmacro()

# Macro to create externalproject_add target
# 
# Common usage
#
# CMAKE_ARGS: cmake(required)
# PATCH_COMMAND: ALL(optional)
# CONFIGURE_COMMAND: autoconf(required), meson(required)
# BUILD_COMMAND: autoconf(required), meson(required), cmake(optional)
# INSTALL_COMMAND: autoconf(required), meson(required), cmake(optional)
# BUILD_IN_SOURCE: ALL(optional)
# BUILD_BYPRODUCTS: ALL(optional)
#
# Windows Specific
# WIN_DISABLE_PROJECT_FLAGS - Set to not use core compiler flags for externalproject_add target
#                             This removes CMAKE_C_FLAGS CMAKE_CXX_FLAGS CMAKE_EXE_LINKER_FLAGS
#                             from the externalproject_add target. Primarily used for HOST build
#                             tools that may have different arch/build requirements to the core app
#                             target (eg flatc)
#
macro(BUILD_DEP_TARGET)
  include(ExternalProject)

  # allow user to override the download URL with a local tarball
  # needed for offline build envs
  if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_URL)
    get_filename_component(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_URL "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_URL}" ABSOLUTE)
  elseif(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_FULL_URL)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_URL "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_FULL_URL}")
  else()
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_URL ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BASE_URL}/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_ARCHIVE})
  endif()

  if(VERBOSE)
    message(STATUS "${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_URL: ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_URL}")
  endif()

  # Remove cmake warning when Xcode generator used with "New" build system
  if(CMAKE_GENERATOR STREQUAL Xcode)
    # Policy CMP0114 is not set to NEW.  In order to support the Xcode "new build
    # system", this project must be updated to set policy CMP0114 to NEW.
    if(CMAKE_XCODE_BUILD_SYSTEM STREQUAL 12)
      cmake_policy(SET CMP0114 NEW)
    else()
      cmake_policy(SET CMP0114 OLD)
    endif()
  endif()

  if(CMAKE_ARGS)
    set(CMAKE_ARGS CMAKE_ARGS ${CMAKE_ARGS}
                             -DPROJECTSOURCE=${PROJECTSOURCE}
                             "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")

    # We dont have a toolchain for windows, so manually add all the cmake
    # build arguments we may want
    # We can disable adding them with WIN_DISABLE_PROJECT_FLAGS. This is potentially required
    # for host build tools (eg flatc) that may be a different arch to the core app
    if(WIN32 OR WINDOWS_STORE)
      if(NOT (DEFINED WIN_DISABLE_PROJECT_FLAGS OR
         ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_LIB_TYPE STREQUAL "native"))
        list(APPEND CMAKE_ARGS "-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS} $<$<CONFIG:Debug>:${CMAKE_C_FLAGS_DEBUG}> $<$<CONFIG:Release>:${CMAKE_C_FLAGS_RELEASE}> ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_C_FLAGS}"
                               "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} $<$<CONFIG:Debug>:${CMAKE_CXX_FLAGS_DEBUG}> $<$<CONFIG:Release>:${CMAKE_CXX_FLAGS_RELEASE}> ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_CXX_FLAGS}"
                               "-DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS} $<$<CONFIG:Debug>:${CMAKE_EXE_LINKER_FLAGS_DEBUG}> $<$<CONFIG:Release>:${CMAKE_EXE_LINKER_FLAGS_RELEASE}> ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_EXE_LINKER_FLAGS}")
      endif()
    else()
      # Enable a module to append to build flags
      if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_C_FLAGS)
        list(APPEND CMAKE_ARGS "-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_C_FLAGS}")
      endif()
      if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_CXX_FLAGS)
        list(APPEND CMAKE_ARGS "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_CXX_FLAGS}")
      endif()
      if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_EXE_LINKER_FLAGS)
        list(APPEND CMAKE_ARGS "-DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_EXE_LINKER_FLAGS}")
      endif()
    endif()

    if(DEFINED ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INSTALL_LIBDIR)
      list(APPEND CMAKE_ARGS -DCMAKE_INSTALL_LIBDIR=${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INSTALL_LIBDIR})
    else()
      list(APPEND CMAKE_ARGS -DCMAKE_INSTALL_LIBDIR=lib)
    endif()

    if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INSTALL_PREFIX)
      list(APPEND CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INSTALL_PREFIX})
    else()
      list(APPEND CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${DEP_LOCATION})
    endif()

    if(DEFINED ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_TOOLCHAIN_FILE)
      list(APPEND CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_TOOLCHAIN_FILE})
    elseif(CMAKE_TOOLCHAIN_FILE)
      list(APPEND CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
    endif()

    # Set build type for dep build.
    # if MODULE has set a manual build type, use it, otherwise use project build type
    if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_TYPE)
      list(APPEND CMAKE_ARGS "-DCMAKE_BUILD_TYPE=${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_TYPE}")
      # Build_type is forced, so unset the opposite <MODULE>_LIBRARY_<TYPE> to only give
      # select_library_configurations one library name to choose from
      if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_TYPE STREQUAL "Release")
        unset(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_DEBUG)
      else()
        unset(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE)
      endif()
    else()
      # single config generator (ie Make, Ninja)
      if(CMAKE_BUILD_TYPE)
        list(APPEND CMAKE_ARGS "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
      else()
        # Multi-config generators (eg VS, Xcode, Ninja Multi-Config) will not have CMAKE_BUILD_TYPE
        # Use config genex to generate the types
        # Potential issue if Build type isnt supported by lib project
        # eg lib supports Debug/Release, however users selects RelWithDebInfo in project
        list(APPEND CMAKE_ARGS "-DCMAKE_BUILD_TYPE=$<CONFIG>")
      endif()
    endif()

    # Xcode - Default sub projects to makefile builds. More consistent
    # Windows - Default to same generator version used in parent
    if(CMAKE_GENERATOR STREQUAL Xcode)
      if(NOT ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_GENERATOR)
        set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_GENERATOR CMAKE_GENERATOR "Unix Makefiles")
      endif()
    elseif(MSVC)
      # Only apply SYSTEM variables for target build libs
      if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_LIB_TYPE STREQUAL "target")
        if(DEFINED CMAKE_SYSTEM_VERSION)
          list(APPEND CMAKE_ARGS -DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION})
        endif()
        if(DEFINED CMAKE_SYSTEM_NAME)
          list(APPEND CMAKE_ARGS -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME})
        endif()
      endif()

      if(NOT ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_GENERATOR)
        set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_GENERATOR CMAKE_GENERATOR "${CMAKE_GENERATOR}")
      endif()
      if(NOT ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_GENERATOR_PLATFORM)
        set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_GENERATOR_PLATFORM CMAKE_GENERATOR_PLATFORM ${CMAKE_GENERATOR_PLATFORM})
      endif()
    endif()
  endif()

  if(PATCH_COMMAND)
    set(PATCH_COMMAND PATCH_COMMAND ${PATCH_COMMAND})
  endif()

  if(CONFIGURE_COMMAND)
    # DEP_BUILDENV is potentially populated in a toolchain file. We dont want to use it
    # for host tool builds, so make sure to check _LIB_TYPE
    if(NOT CMAKE_ARGS AND
      (DEP_BUILDENV AND NOT ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_LIB_TYPE STREQUAL "native"))
      # DEP_BUILDENV only used for non cmake externalproject_add builds
      # iterate through CONFIGURE_COMMAND looking for multiple COMMAND, we need to
      # add DEP_BUILDENV for each distinct COMMAND
      set(tmp_config_command ${DEP_BUILDENV})
      foreach(item ${CONFIGURE_COMMAND})
        list(APPEND tmp_config_command ${item})
        if(item STREQUAL "COMMAND")
          list(APPEND tmp_config_command ${DEP_BUILDENV})
        endif()
      endforeach()
      set(CONFIGURE_COMMAND CONFIGURE_COMMAND ${tmp_config_command})
      unset(tmp_config_command)
    else()
      set(CONFIGURE_COMMAND CONFIGURE_COMMAND ${CONFIGURE_COMMAND})
    endif()
  endif()

  if(BUILD_COMMAND)
    set(BUILD_COMMAND BUILD_COMMAND ${BUILD_COMMAND})
  endif()

  if(INSTALL_COMMAND)
    set(INSTALL_COMMAND INSTALL_COMMAND ${INSTALL_COMMAND})
  endif()

  if(BUILD_IN_SOURCE)
    set(BUILD_IN_SOURCE BUILD_IN_SOURCE ${BUILD_IN_SOURCE})
  endif()

  # Change extension of BYPRODUCT
  # eg, Macos uses dylib for shared libs, but all other unix platforms use .so
  if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPRODUCT_EXTENSION)
    string(REGEX REPLACE "\\.[^.]*$" "" _LIBNAME ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPRODUCT})
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPRODUCT "${_LIBNAME}.${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPRODUCT_EXTENSION}")
    unset(_LIBNAME)
  endif()

  # Defines if the build byproduct is a shared lib
  # This sets up extension changes, and output paths. Both can be overridden by the 
  # caller if required.
  # Unix platforms default to .so extension
  if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SHARED_LIB)
    if(WIN32 OR WINDOWS_STORE)
      if(NOT DEFINED ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LOCATION_POSTFIX)
        set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LOCATION_POSTFIX dll)
      endif()
      if(NOT DEFINED ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LOCATION_PATH)
        set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LOCATION_PATH bin)
      endif()
      if(NOT DEFINED ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_IMPLIB_PATH)
        set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_IMPLIB_PATH lib)
      endif()
    else()
      if(NOT DEFINED ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LOCATION_POSTFIX)
        set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LOCATION_POSTFIX so)
      endif()
      if(NOT DEFINED ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LOCATION_PATH)
        set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LOCATION_PATH lib)
      endif()
    endif()
  else()
    if(NOT DEFINED ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LOCATION_PATH)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LOCATION_PATH lib)
    endif()
  endif()

  # Set Library names.
  if(DEFINED ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX)
    set(_POSTFIX ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX})
    string(REGEX REPLACE "\\.[^.]*$" "" _LIBNAME ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPRODUCT})
    string(REGEX REPLACE "^.*\\." "" _LIBEXT ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPRODUCT})

    if(DEFINED ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SHARED_LIB)
      # Windows shared libs have both a .lib and a .dll artifact. 
      # IMPLIB_<CONFIG> is windows only for our use cases
      if(WIN32 OR WINDOWS_STORE)
        set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_IMPLIB_DEBUG ${DEP_LOCATION}/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_IMPLIB_PATH}/${_LIBNAME}${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX}.${_LIBEXT})
      endif()
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_DEBUG ${DEP_LOCATION}/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LOCATION_PATH}/${_LIBNAME}${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX}.${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LOCATION_POSTFIX})
    else()
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_DEBUG ${DEP_LOCATION}/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LOCATION_PATH}/${_LIBNAME}${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX}.${_LIBEXT})
    endif()
  endif()
  # set <MODULE>_LIBRARY_RELEASE for use of select_library_configurations
  # any modules that dont use select_library_configurations, we set <MODULE>_LIBRARY
  # No harm in having either set for both potential paths
  if(DEFINED ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SHARED_LIB)
    string(REGEX REPLACE "\\.[^.]*$" "" _LIBNAME ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPRODUCT})
    string(REGEX REPLACE "^.*\\." "" _LIBEXT ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPRODUCT})
    if(WIN32 OR WINDOWS_STORE)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_IMPLIB_RELEASE ${DEP_LOCATION}/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_IMPLIB_PATH}/${_LIBNAME}.${_LIBEXT})
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_IMPLIB ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_IMPLIB_RELEASE})
    endif()
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE ${DEP_LOCATION}/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LOCATION_PATH}/${_LIBNAME}.${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LOCATION_POSTFIX})
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE})
  else()
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE ${DEP_LOCATION}/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LOCATION_PATH}/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BYPRODUCT})
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE})
  endif()

  if(NOT ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR ${DEP_LOCATION}/include)
  endif()

  if(BUILD_BYPRODUCTS)
    set(BUILD_BYPRODUCTS BUILD_BYPRODUCTS ${BUILD_BYPRODUCTS})
  else()
    if(DEFINED ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_DEBUG)
      if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.20")
        set(BUILD_BYPRODUCTS BUILD_BYPRODUCTS "$<IF:$<CONFIG:Debug,RelWithDebInfo>,${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_DEBUG},${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE}>")
      else()
        if(DEFINED CMAKE_BUILD_TYPE)
          if(NOT CMAKE_BUILD_TYPE STREQUAL "Release" AND DEFINED ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_DEBUG)
            set(BUILD_BYPRODUCTS BUILD_BYPRODUCTS "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_DEBUG}")
          else()
            set(BUILD_BYPRODUCTS BUILD_BYPRODUCTS "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY}")
          endif()
        else()
          message(FATAL_ERROR "MultiConfig Generator usage requires CMake >= 3.20.0 - Generator Expressions in BYPRODUCT option")
        endif()
      endif()
    else()
      set(BUILD_BYPRODUCTS BUILD_BYPRODUCTS "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY}")
    endif()
  endif()

  if(NOT INSTALL_DIR)
    set(INSTALL_DIR ${DEP_LOCATION})
  endif()

  # Allow a target to supply in-tree source location. eg TexturePacker, JsonSchemaBuilder
  if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SOURCE_DIR)
    set(BUILD_DOWNLOAD_STEPS SOURCE_DIR "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SOURCE_DIR}")
  else()
    set(BUILD_DOWNLOAD_STEPS URL ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_URL}
                             URL_HASH ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_HASH}
                             DOWNLOAD_DIR ${TARBALL_DIR}
                             DOWNLOAD_NAME ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_ARCHIVE})
  endif()

  externalproject_add(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME}
                      ${BUILD_DOWNLOAD_STEPS}
                      PREFIX ${CORE_BUILD_DIR}/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME}
                      INSTALL_DIR ${INSTALL_DIR}
                      ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIST_SEPARATOR}
                      ${CMAKE_ARGS}
                      ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_GENERATOR}
                      ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_GENERATOR_PLATFORM}
                      ${PATCH_COMMAND}
                      ${CONFIGURE_COMMAND}
                      ${BUILD_COMMAND}
                      ${INSTALL_COMMAND}
                      ${BUILD_BYPRODUCTS}
                      ${BUILD_IN_SOURCE})

  set_target_properties(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} PROPERTIES FOLDER "External Projects")

  CLEAR_BUILD_VARS()

  # Set both <UPPER>_FOUND and <lower>_FOUND as we have some legacy macros/functions that
  # rely on <UPPER>_FOUND, but cmake has new policies that are heading towards <lower>_FOUND
  set(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND ON CACHE BOOL "${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND" FORCE)

  string(TOUPPER "${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}" _search_upper)
  set(${_search_upper}_FOUND ON CACHE BOOL "${_search_upper}_FOUND" FORCE)
  unset(_search_upper)
endmacro()

# Macro to test format of line endings of a patch
# Windows Specific
macro(PATCH_LF_CHECK patch)
  if(CMAKE_HOST_WIN32)
    # On Windows "patch.exe" can only handle CR-LF line-endings.
    # Our patches have LF-only line endings - except when they
    # have been checked out as part of a dependency hosted on Git
    # and core.autocrlf=true.
    file(READ ${ARGV0} patch_content_hex HEX)
    # Force handle LF-only line endings
    if(NOT patch_content_hex MATCHES "0d0a")
      if (NOT "--binary" IN_LIST PATCH_EXECUTABLE)
        list(APPEND PATCH_EXECUTABLE --binary)
      endif()
    else()
      if ("--binary" IN_LIST PATCH_EXECUTABLE)
        list(REMOVE_ITEM PATCH_EXECUTABLE --binary)
      endif()
    endif()
  endif()
  unset(patch_content_hex)
endmacro()

# Function to check a list of libraries to force the calling library to be
# rebuilt if one of its dependencies needs to be built.
#
# ToDo: all dep_list libs are considered REQUIRED for find_package call
#       Should we handle optional dependencies?
#
# caller: Parent lib
# dep_list: List of libraries to be checked for LIB_BUILD property
#
# Return: Set ${caller}_FORCE_BUILD variable to alert caller lib to force a rebuild
#
# ToDo: We break early on detection of first lib in dep_list requiring a build
#       We may want to run all find_package calls regardless for dependency inception
function(check_dependency_build caller dep_list)
    foreach(_dep ${dep_list})
      find_package(${_dep} REQUIRED ${SEARCH_QUIET})
      if(TARGET LIBRARY::${_dep})
        # Test if LIB_BUILD property is set on LIBRARY::${_dep} TARGET indicating
        # if it requires to be build (version update, or even just missing)
        get_target_property(${caller}_DEP_BUILD LIBRARY::${_dep} LIB_BUILD)

        if(${caller}_DEP_BUILD)
          # A dependency is to be rebuilt, set this lib to be built, no need to check any
          # other libs in DEPLIST
          set(${caller}_FORCE_BUILD ON PARENT_SCOPE)
          if(VERBOSE)
            message(WARNING "${caller} alerted to rebuild required by ${_dep}")
          endif()
          break()
        endif()
      endif()
    endforeach()
endfunction()

macro(SETUP_FIND_SPECS)
  # Setup Find version specs if requested by caller
  if(${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION)
    if(${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION_EXACT)
      set(CONFIG_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC "${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION_COMPLETE} EXACT")
      set(PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC "=${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION_COMPLETE}")
    else()
      set(CONFIG_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC "${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION_COMPLETE}")
      set(PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC ">=${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION_COMPLETE}")
    endif()
  endif()
endmacro()

# Macro to generate a TARGET based on information created by a BUILD_DEP_TARGET call
#
# Populates the following INTERFACE options for a target if they exist for ${CMAKE_FIND_PACKAGE_NAME}_MODULE
#   INTERFACE_INCLUDE_DIRECTORIES
#   INTERFACE_LINK_LIBRARIES
#   IMPORTED_LOCATION_RELEASE
#   IMPORTED_IMPLIB_RELEASE
#   IMPORTED_LOCATION_DEBUG
#   IMPORTED_IMPLIB_DEBUG
#
# Input:
#   ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SHARED_LIB (BOOL) - Library created as SHARED
#   ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INTERFACE_LIB (BOOL) - Library created as INTERFACE
#   Default: STATIC - Library created as INTERFACE
#
#   ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_TYPE
#   Default: ${APP_NAME_LC}
#   Option:  LIBRARY
#
macro(SETUP_BUILD_TARGET)
  if(DEFINED ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_TYPE)
    set(LIB_SCOPE ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_TYPE})
  else()
    set(LIB_SCOPE ${APP_NAME_LC})
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SHARED_LIB)
    add_library(${LIB_SCOPE}::${CMAKE_FIND_PACKAGE_NAME} SHARED IMPORTED)
  elseif(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INTERFACE_LIB)
    add_library(${LIB_SCOPE}::${CMAKE_FIND_PACKAGE_NAME} INTERFACE IMPORTED)
  else()
    add_library(${LIB_SCOPE}::${CMAKE_FIND_PACKAGE_NAME} STATIC IMPORTED)
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR)
    set_target_properties(${LIB_SCOPE}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                   INTERFACE_INCLUDE_DIRECTORIES "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR}")
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES)
    set_target_properties(${LIB_SCOPE}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                   INTERFACE_LINK_LIBRARIES "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES}")
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE)
    set_target_properties(${LIB_SCOPE}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                   IMPORTED_CONFIGURATIONS RELEASE
                                                                   IMPORTED_LOCATION_RELEASE "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE}")
    if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SHARED_LIB AND (WIN32 OR WINDOWS_STORE))
      set_target_properties(${LIB_SCOPE}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_IMPLIB_RELEASE "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_IMPLIB_RELEASE}")
    endif()
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_DEBUG)
    set_target_properties(${LIB_SCOPE}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                   IMPORTED_LOCATION_DEBUG "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_DEBUG}")
    set_property(TARGET ${LIB_SCOPE}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                        IMPORTED_CONFIGURATIONS DEBUG)

    if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SHARED_LIB AND (WIN32 OR WINDOWS_STORE))
      set_target_properties(${LIB_SCOPE}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_IMPLIB_DEBUG "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_IMPLIB_DEBUG}")
    endif()
  endif()
endmacro()

# Macro to add INTERFACE_COMPILE_DEFINITIONS to a TARGET
# if ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS is defined
#
# We use this to add HAVE_<LIB> defines to any TARGET type we use (pkgconfig, cmake or build)
#
macro(ADD_TARGET_COMPILE_DEFINITION)
  if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS)
    get_target_property(_ALIASTARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIASED_TARGET)
    if(_ALIASTARGET)
      set(LIB_TARGET ${_ALIASTARGET})
    else()
      set(LIB_TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
    endif()

    set_property(TARGET ${LIB_TARGET} APPEND PROPERTY
                                             INTERFACE_COMPILE_DEFINITIONS "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS}")
  endif()
endmacro()

# Add internal build target when a Multi Config Generator is used
# We cant add a dependency based off a generator expression for targeted build types,
# https://gitlab.kitware.com/cmake/cmake/-/issues/19467
# therefore if the find heuristics only find the library, we add the internal build
# target to the project to allow user to manually trigger for any build type they need
# in case only a specific build type is actually available (eg Release found, Debug Required)
# This is mainly targeted for windows who required different runtime libs for different
# types, and they arent compatible
macro(ADD_MULTICONFIG_BUILDMACRO)
  if(_multiconfig_generator)
    if(NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      cmake_language(EVAL CODE "
        buildmacro${CMAKE_FIND_PACKAGE_NAME}()
      ")
      set_target_properties(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE)
    endif()
    add_dependencies(build_internal_depends ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
  endif()
endmacro()

macro(SEARCH_EXISTING_PACKAGES)
  find_package(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} ${CONFIG_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} CONFIG ${SEARCH_QUIET}
                                                         HINTS ${DEPENDS_PATH}/share/cmake
                                                               ${DEPENDS_PATH}/lib/cmake
                                                         ${${CORE_SYSTEM_NAME}_SEARCH_CONFIG})

  # fallback to pkgconfig to cover all bases
  if(NOT ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    find_package(PkgConfig ${SEARCH_QUIET})

    if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWSSTORE))
      pkg_check_modules(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} ${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME_PC}${PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} ${SEARCH_QUIET} IMPORTED_TARGET)
    endif()
  endif()
endmacro()

# Custom property that we can track to allow us to notify to dependency find modules
# that a dependency of that find module is being built, and therefore that higher level
# dependency should also be built regardless of success in lib searches
define_property(TARGET PROPERTY LIB_BUILD
                       BRIEF_DOCS "This target will be compiling the library"
                       FULL_DOCS "This target will be compiling the library")


# Functions to generate meson cross files for a platform

macro(generate_mesoncrossfile)

  create_mesonbinaries()
  file(WRITE ${DEPENDS_PATH}/share/cross-file.meson "${meson_binaries_string}\n")

  create_mesonhostmachine()
  file(APPEND ${DEPENDS_PATH}/share/cross-file.meson "${meson_host_machine_string}\n")

  create_mesonproperties()
  file(APPEND ${DEPENDS_PATH}/share/cross-file.meson "${meson_properties_string}\n")

  create_mesonbuiltin()
  file(APPEND ${DEPENDS_PATH}/share/cross-file.meson "${meson_builtin_string}\n")

endmacro()

# Creates the [binaries] block of a meson cross file
# sets meson_binaries_string to PARENT_SCOPE
function(create_mesonbinaries)

  set(binariespairs "c" "CMAKE_C_COMPILER"
                    "cpp" "CMAKE_CXX_COMPILER"
                    "ar" "CMAKE_AR"
                    "cmake" "CMAKE_COMMAND")

  if(NOT "${CMAKE_STRIP}" STREQUAL "")
    list(APPEND binariespairs "strip" "CMAKE_STRIP")
  endif()

  if(PKG_CONFIG_EXECUTABLE)
    list(APPEND binariespairs "pkg-config" "PKG_CONFIG_EXECUTABLE")
  endif()

  # Get/set loop limit (Size - 1) from size of binariespairs list
  list(LENGTH binariespairs options_length)
  math(EXPR options_length "${options_length} - 1")

  foreach(option_arg RANGE 0 ${options_length} 2)
    math(EXPR cmake_arg "${option_arg} + 1")
    # meson option name
    list(GET binariespairs ${option_arg} meson_label_name)
    # cmake source variable name
    list(GET binariespairs ${cmake_arg} cmake_binary_name)

    set(input "${${cmake_binary_name}}")
    string(STRIP "${input}" input)

    string(PREPEND input "${meson_label_name} = '")
    string(APPEND input "'")
    string(APPEND output_string "${input}\n")
  endforeach()

  # Easiest to just prepend header at the end of the full string creation
  string(PREPEND output_string "[binaries]\n")
  set(meson_binaries_string ${output_string} PARENT_SCOPE)
endfunction()

# Creates the [host_machine] block of a meson cross file
# sets meson_host_machine_string to PARENT_SCOPE
function(create_mesonhostmachine)

  # Non-exhaustive list to map cmake CPU to meson cpu names
  # https://mesonbuild.com/Reference-tables.html#cpu-families
  string(TOUPPER ${CMAKE_C_COMPILER_ARCHITECTURE_ID} UPPER_C_ARCH)
  if("${UPPER_C_ARCH}" MATCHES "ARM64" OR "${UPPER_C_ARCH}" MATCHES "AARCH64")
    set(meson_cpu_family aarch64)
  elseif("${UPPER_C_ARCH}" MATCHES "ARMV.")
    set(meson_cpu_family arm)
  elseif("${UPPER_C_ARCH}" STREQUAL "x64" OR "${UPPER_C_ARCH}" STREQUAL "X86_64")
    set(meson_cpu_family x86_64)
  elseif("${UPPER_C_ARCH}" STREQUAL "X86" OR "${UPPER_C_ARCH}" MATCHES "I.86")
    set(meson_cpu_family x86)
  endif()

  # Non-exhaustive list to map cmake to meson os names
  # https://mesonbuild.com/Reference-tables.html#operating-system-names
  if(CMAKE_SYSTEM_NAME MATCHES "Android")
    set(meson_sys_name android)
  elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(meson_sys_name darwin)
  elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    set(meson_sys_name freebsd)
  elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(meson_sys_name linux)
  elseif(CMAKE_SYSTEM_NAME MATCHES "NetBSD")
    set(meson_sys_name netbsd)
  elseif(CMAKE_SYSTEM_NAME MATCHES Windows)
    set(meson_sys_name windows)
  endif()

  string(APPEND output_string "system = '${meson_sys_name}'\n")
  string(APPEND output_string "cpu_family = '${meson_cpu_family}'\n")

  # cpu is apparently unnecessary from a functional stand point, however the field
  # is still required. We have no easy way to populate, so for now just use an empty value
  string(APPEND output_string "cpu = ''\n")
  string(APPEND output_string "endian = 'little'\n")

  # Easiest to just prepend header at the end of the full string creation
  string(PREPEND output_string "[host_machine]\n")
  set(meson_host_machine_string ${output_string} PARENT_SCOPE)
endfunction()

# Creates the [properties] block of a meson cross file
# sets meson_properties_string to PARENT_SCOPE
function(create_mesonproperties)

  string(APPEND output_string "pkg_config_libdir = '${DEPENDS_PATH}/lib/pkgconfig'\n")

  # Easiest to just prepend header at the end of the full string creation
  string(PREPEND output_string "[properties]\n")
  set(meson_properties_string ${output_string} PARENT_SCOPE)
endfunction()

# Creates the [Built-in Options] block of a meson cross file
# sets meson_builtin_string to PARENT_SCOPE
function(create_mesonbuiltin)

  set(builtinpairs "c_args" "CMAKE_C_FLAGS"
                   "c_link_args" "CMAKE_EXE_LINKER_FLAGS"
                   "cpp_args" "CMAKE_CXX_FLAGS"
                   "cpp_link_args" "CMAKE_EXE_LINKER_FLAGS")

  # Get/set loop limit (Size - 1) from size of builtinpairs list
  list(LENGTH builtinpairs options_length)
  math(EXPR options_length "${options_length} - 1")

  foreach(option_arg RANGE 0 ${options_length} 2)
    math(EXPR cmake_flag_arg "${option_arg} + 1")
    # meson option name
    list(GET builtinpairs ${option_arg} meson_label_name)
    # cmake source variable name
    list(GET builtinpairs ${cmake_flag_arg} cmake_flag_name)

    set(input "${${cmake_flag_name}}")
    string(STRIP "${input}" input)

    # builtinpairs cmake source variables are specifically single strings, and not lists
    string(REPLACE " " "', '" tmp_string "${input}")
    string(PREPEND tmp_string "${meson_label_name} = ['")
    string(APPEND tmp_string "']")
    string(APPEND output_string "${tmp_string}\n")
  endforeach()

  string(APPEND output_string "default_library = 'static'\n")
  string(APPEND output_string "prefix = '${DEPENDS_PATH}'\n")
  string(APPEND output_string "libdir = 'lib'\n")
  string(APPEND output_string "bindir = 'bin'\n")
  string(APPEND output_string "includedir = 'include'\n")

  # Easiest to just prepend header at the end of the full string creation
  string(PREPEND output_string "[built-in options]\n")
  set(meson_builtin_string ${output_string} PARENT_SCOPE)
endfunction()
