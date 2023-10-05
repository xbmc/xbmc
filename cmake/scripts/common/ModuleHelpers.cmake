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
  set(MODULE_PATH "${PROJECTSOURCE}/tools/depends/${LIB_TYPE}/${MODULE_LC}")

  if(NOT EXISTS "${MODULE_PATH}/${MODULE}-VERSION")
    MESSAGE(FATAL_ERROR "${MODULE}-VERSION does not exist at ${MODULE_PATH}.")
  else()
    set(${MODULE}_FILE "${MODULE_PATH}/${MODULE}-VERSION")
  endif()

  file(STRINGS ${${MODULE}_FILE} ${MODULE}_LNAME REGEX "^[ \t]*LIBNAME=")
  file(STRINGS ${${MODULE}_FILE} ${MODULE}_VER REGEX "^[ \t]*VERSION=")
  file(STRINGS ${${MODULE}_FILE} ${MODULE}_ARCHIVE REGEX "^[ \t]*ARCHIVE=")
  file(STRINGS ${${MODULE}_FILE} ${MODULE}_BASE_URL REGEX "^[ \t]*BASE_URL=")
  if(WIN32 OR WINDOWS_STORE)
    file(STRINGS ${${MODULE}_FILE} ${MODULE}_BYPRODUCT REGEX "^[ \t]*BYPRODUCT_WIN=")
  else()
    file(STRINGS ${${MODULE}_FILE} ${MODULE}_BYPRODUCT REGEX "^[ \t]*BYPRODUCT=")
  endif()

  # Tarball Hash
  file(STRINGS ${${MODULE}_FILE} ${MODULE}_HASH_SHA256 REGEX "^[ \t]*SHA256=")
  file(STRINGS ${${MODULE}_FILE} ${MODULE}_HASH_SHA512 REGEX "^[ \t]*SHA512=")

  string(REGEX REPLACE ".*LIBNAME=([^ \t]*).*" "\\1" ${MODULE}_LNAME "${${MODULE}_LNAME}")
  string(REGEX REPLACE ".*VERSION=([^ \t]*).*" "\\1" ${MODULE}_VER "${${MODULE}_VER}")
  string(REGEX REPLACE ".*ARCHIVE=([^ \t]*).*" "\\1" ${MODULE}_ARCHIVE "${${MODULE}_ARCHIVE}")
  string(REGEX REPLACE ".*BASE_URL=([^ \t]*).*" "\\1" ${MODULE}_BASE_URL "${${MODULE}_BASE_URL}")
  if(WIN32 OR WINDOWS_STORE)
    string(REGEX REPLACE ".*BYPRODUCT_WIN=([^ \t]*).*" "\\1" ${MODULE}_BYPRODUCT "${${MODULE}_BYPRODUCT}")
  else()
    string(REGEX REPLACE ".*BYPRODUCT=([^ \t]*).*" "\\1" ${MODULE}_BYPRODUCT "${${MODULE}_BYPRODUCT}")
  endif()

  string(REGEX REPLACE "\\$\\(LIBNAME\\)" "${${MODULE}_LNAME}" ${MODULE}_ARCHIVE "${${MODULE}_ARCHIVE}")
  string(REGEX REPLACE "\\$\\(VERSION\\)" "${${MODULE}_VER}" ${MODULE}_ARCHIVE "${${MODULE}_ARCHIVE}")

  set(${MODULE}_ARCHIVE ${${MODULE}_ARCHIVE} PARENT_SCOPE)

  set(${MODULE}_VER ${${MODULE}_VER} PARENT_SCOPE)

  if (${MODULE}_BASE_URL)
    set(${MODULE}_BASE_URL ${${MODULE}_BASE_URL} PARENT_SCOPE)
  else()
    set(${MODULE}_BASE_URL "http://mirrors.kodi.tv/build-deps/sources" PARENT_SCOPE)
  endif()
  set(${MODULE}_BYPRODUCT ${${MODULE}_BYPRODUCT} PARENT_SCOPE)

  # allow user to override the download URL hash with a local tarball hash
  # needed for offline build envs
  if (NOT DEFINED ${MODULE}_HASH)
    if (${MODULE}_HASH_SHA256)
      set(${MODULE}_HASH ${${MODULE}_HASH_SHA256} PARENT_SCOPE)
    elseif(${MODULE}_HASH_SHA512)
      set(${MODULE}_HASH ${${MODULE}_HASH_SHA512} PARENT_SCOPE)
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
  find_package(Patch MODULE REQUIRED)

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
  string(TOUPPER ${MODULE_LC} MODULE)

  # Fall through to target build module dir if not explicitly set
  if(NOT DEFINED LIB_TYPE)
    set(LIB_TYPE "target")
  endif()

  # Location for build type, native or target
  if(LIB_TYPE STREQUAL "target")
    set(DEP_LOCATION "${DEPENDS_PATH}")
  else()
    set(DEP_LOCATION "${NATIVEPREFIX}")
  endif()

  # PROJECTSOURCE used in native toolchain to provide core project sourcedir
  # to externalproject_add targets that have a different CMAKE_SOURCE_DIR (eg jsonschema/texturepacker in-tree)
  if(NOT PROJECTSOURCE)
    set(PROJECTSOURCE ${CMAKE_SOURCE_DIR})
  endif()

  if(NOT ${MODULE}_DISABLE_VERSION)
    # populate variables of data from VERSION file for MODULE
    get_versionfile_data()
  endif()

  # allow user to override the download URL with a local tarball
  # needed for offline build envs
  if(${MODULE}_URL)
    get_filename_component(${MODULE}_URL "${${MODULE}_URL}" ABSOLUTE)
  else()
    set(${MODULE}_URL ${${MODULE}_BASE_URL}/${${MODULE}_ARCHIVE})
  endif()
  if(VERBOSE)
    message(STATUS "MODULE: ${MODULE}")
    message(STATUS "LIB_TYPE: ${LIB_TYPE}")
    message(STATUS "DEP_LOCATION: ${DEP_LOCATION}")
    message(STATUS "PROJECTSOURCE: ${PROJECTSOURCE}")
    message(STATUS "${MODULE}_URL: ${${MODULE}_URL}")
  endif()
  unset(LIB_TYPE)
endmacro()

macro(CLEAR_BUILD_VARS)
  # unset all generic variables to insure clean state between macro calls
  # Potentially an issue with scope when a macro is used inside a dep that uses a macro
  unset(PROJECTSOURCE)
  unset(LIB_TYPE)
  unset(BUILD_NAME)
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
  unset(${MODULE}_LIST_SEPARATOR)
  unset(${MODULE}_GENERATOR)
  unset(${MODULE}_GENERATOR_PLATFORM)
  unset(${MODULE}_INSTALL_PREFIX)
  unset(${MODULE}_TOOLCHAIN_FILE)
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
                             -DCMAKE_INSTALL_LIBDIR=lib
                             -DPROJECTSOURCE=${PROJECTSOURCE}
                             "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")

    # We dont have a toolchain for windows, so manually add all the cmake
    # build arguments we may want
    # We can disable adding them with WIN_DISABLE_PROJECT_FLAGS. This is potentially required
    # for host build tools (eg flatc) that may be a different arch to the core app
    if(WIN32 OR WINDOWS_STORE)
      if(NOT DEFINED WIN_DISABLE_PROJECT_FLAGS)
        list(APPEND CMAKE_ARGS "-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS} $<$<CONFIG:Debug>:${CMAKE_C_FLAGS_DEBUG}> $<$<CONFIG:Release>:${CMAKE_C_FLAGS_RELEASE}> ${${MODULE}_C_FLAGS}"
                               "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} $<$<CONFIG:Debug>:${CMAKE_CXX_FLAGS_DEBUG}> $<$<CONFIG:Release>:${CMAKE_CXX_FLAGS_RELEASE}> ${${MODULE}_CXX_FLAGS}"
                               "-DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS} $<$<CONFIG:Debug>:${CMAKE_EXE_LINKER_FLAGS_DEBUG}> $<$<CONFIG:Release>:${CMAKE_EXE_LINKER_FLAGS_RELEASE}> ${${MODULE}_EXE_LINKER_FLAGS}")
      endif()
    endif()

    if(${MODULE}_INSTALL_PREFIX)
      list(APPEND CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${${MODULE}_INSTALL_PREFIX})
    else()
      list(APPEND CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${DEP_LOCATION})
    endif()

    if(DEFINED ${MODULE}_TOOLCHAIN_FILE)
      list(APPEND CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${${MODULE}_TOOLCHAIN_FILE})
    elseif(CMAKE_TOOLCHAIN_FILE)
      list(APPEND CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
    endif()

    # Set build type for dep build.
    # if MODULE has set a manual build type, use it, otherwise use project build type
    if(${MODULE}_BUILD_TYPE)
      list(APPEND CMAKE_ARGS "-DCMAKE_BUILD_TYPE=${${MODULE}_BUILD_TYPE}")
      # Build_type is forced, so unset the opposite <MODULE>_LIBRARY_<TYPE> to only give
      # select_library_configurations one library name to choose from
      if(${MODULE}_BUILD_TYPE STREQUAL "Release")
        unset(${MODULE}_LIBRARY_DEBUG)
      else()
        unset(${MODULE}_LIBRARY_RELEASE)
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
      if(NOT ${MODULE}_GENERATOR)
        set(${MODULE}_GENERATOR CMAKE_GENERATOR "Unix Makefiles")
      endif()
    elseif(MSVC)
      if(NOT ${MODULE}_GENERATOR)
        set(${MODULE}_GENERATOR CMAKE_GENERATOR "${CMAKE_GENERATOR}")
      endif()
      if(NOT ${MODULE}_GENERATOR_PLATFORM)
        set(${MODULE}_GENERATOR_PLATFORM CMAKE_GENERATOR_PLATFORM ${CMAKE_GENERATOR_PLATFORM})
      endif()
    endif()
  endif()

  if(PATCH_COMMAND)
    set(PATCH_COMMAND PATCH_COMMAND ${PATCH_COMMAND})
  endif()

  if(CONFIGURE_COMMAND)
    if(NOT CMAKE_ARGS AND DEP_BUILDENV)
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

  # Set Library names.
  if(DEFINED ${MODULE}_DEBUG_POSTFIX)
    set(_POSTFIX ${${MODULE}_DEBUG_POSTFIX})
    string(REGEX REPLACE "\\.[^.]*$" "" _LIBNAME ${${MODULE}_BYPRODUCT})
    string(REGEX REPLACE "^.*\\." "" _LIBEXT ${${MODULE}_BYPRODUCT})
    set(${MODULE}_LIBRARY_DEBUG ${DEP_LOCATION}/lib/${_LIBNAME}${${MODULE}_DEBUG_POSTFIX}.${_LIBEXT})
  endif()
  # set <MODULE>_LIBRARY_RELEASE for use of select_library_configurations
  # any modules that dont use select_library_configurations, we set <MODULE>_LIBRARY
  # No harm in having either set for both potential paths
  set(${MODULE}_LIBRARY_RELEASE ${DEP_LOCATION}/lib/${${MODULE}_BYPRODUCT})
  set(${MODULE}_LIBRARY ${${MODULE}_LIBRARY_RELEASE})

  if(NOT ${MODULE}_INCLUDE_DIR)
    set(${MODULE}_INCLUDE_DIR ${DEP_LOCATION}/include)
  endif()

  if(BUILD_BYPRODUCTS)
    set(BUILD_BYPRODUCTS BUILD_BYPRODUCTS ${BUILD_BYPRODUCTS})
  else()
    if(DEFINED ${MODULE}_LIBRARY_DEBUG)
      if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.20")
        set(BUILD_BYPRODUCTS BUILD_BYPRODUCTS "$<IF:$<CONFIG:Debug,RelWithDebInfo>,${${MODULE}_LIBRARY_DEBUG},${${MODULE}_LIBRARY_RELEASE}>")
      else()
        if(DEFINED CMAKE_BUILD_TYPE)
          if(NOT CMAKE_BUILD_TYPE STREQUAL "Release" AND DEFINED ${MODULE}_LIBRARY_DEBUG)
            set(BUILD_BYPRODUCTS BUILD_BYPRODUCTS "${${MODULE}_LIBRARY_DEBUG}")
          else()
            set(BUILD_BYPRODUCTS BUILD_BYPRODUCTS "${${MODULE}_LIBRARY}")
          endif()
        else()
          message(FATAL_ERROR "MultiConfig Generator usage requires CMake >= 3.20.0 - Generator Expressions in BYPRODUCT option")
        endif()
      endif()
    else()
      set(BUILD_BYPRODUCTS BUILD_BYPRODUCTS "${${MODULE}_LIBRARY}")
    endif()
  endif()

  if(NOT BUILD_NAME)
    set(BUILD_NAME ${MODULE_LC})
  endif()

  if(NOT INSTALL_DIR)
    set(INSTALL_DIR ${DEP_LOCATION})
  endif()

  # Allow a target to supply in-tree source location. eg TexturePacker, JsonSchemaBuilder
  if(${MODULE}_SOURCE_DIR)
    set(BUILD_DOWNLOAD_STEPS SOURCE_DIR "${${MODULE}_SOURCE_DIR}")
  else()
    set(BUILD_DOWNLOAD_STEPS URL ${${MODULE}_URL}
                             URL_HASH ${${MODULE}_HASH}
                             DOWNLOAD_DIR ${TARBALL_DIR}
                             DOWNLOAD_NAME ${${MODULE}_ARCHIVE})
  endif()

  externalproject_add(${BUILD_NAME}
                      ${BUILD_DOWNLOAD_STEPS}
                      PREFIX ${CORE_BUILD_DIR}/${BUILD_NAME}
                      INSTALL_DIR ${INSTALL_DIR}
                      ${${MODULE}_LIST_SEPARATOR}
                      ${CMAKE_ARGS}
                      ${${MODULE}_GENERATOR}
                      ${${MODULE}_GENERATOR_PLATFORM}
                      ${PATCH_COMMAND}
                      ${CONFIGURE_COMMAND}
                      ${BUILD_COMMAND}
                      ${INSTALL_COMMAND}
                      ${BUILD_BYPRODUCTS}
                      ${BUILD_IN_SOURCE})

  set_target_properties(${BUILD_NAME} PROPERTIES FOLDER "External Projects")

  CLEAR_BUILD_VARS()
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

# Custom property that we can track to allow us to notify to dependency find modules
# that a dependency of that find module is being built, and therefore that higher level
# dependency should also be built regardless of success in lib searches
define_property(TARGET PROPERTY LIB_BUILD
                       BRIEF_DOCS "This target will be compiling the library"
                       FULL_DOCS "This target will be compiling the library")
