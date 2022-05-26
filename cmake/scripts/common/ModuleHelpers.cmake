# This script provides helper functions for FindModules

# Parse and set variables from VERSION dependency file
# Arguments:
#   module_name name of the library (currently must match tools/depends/target/${module_name})
# On return:
#   ARCHIVE will be set to parent scope
#   MODULENAME_VER will be set to parent scope (eg FFMPEG_VER, DAV1D_VER)
#   MODULENAME_BASE_URL will be set to parent scope if exists in VERSION file (eg FFMPEG_BASE_URL)
function(get_versionfile_data module_name)
  string(TOUPPER ${module_name} UPPER_MODULE_NAME)

  # Dependency path
  set(MODULE_PATH "${CMAKE_SOURCE_DIR}/tools/depends/target/${module_name}")
  if(NOT EXISTS "${MODULE_PATH}/${UPPER_MODULE_NAME}-VERSION")
    MESSAGE(FATAL_ERROR "${UPPER_MODULE_NAME}-VERSION does not exist at ${MODULE_PATH}.")
  else()
    set(${UPPER_MODULE_NAME}_FILE "${MODULE_PATH}/${UPPER_MODULE_NAME}-VERSION")
  endif()

  file(STRINGS ${${UPPER_MODULE_NAME}_FILE} ${UPPER_MODULE_NAME}_LNAME REGEX "^[ \t]*LIBNAME=")
  file(STRINGS ${${UPPER_MODULE_NAME}_FILE} ${UPPER_MODULE_NAME}_VER REGEX "^[ \t]*VERSION=")
  file(STRINGS ${${UPPER_MODULE_NAME}_FILE} ${UPPER_MODULE_NAME}_ARCHIVE REGEX "^[ \t]*ARCHIVE=")
  file(STRINGS ${${UPPER_MODULE_NAME}_FILE} ${UPPER_MODULE_NAME}_BASE_URL REGEX "^[ \t]*BASE_URL=")
  if(WIN32 OR WINDOWS_STORE)
    file(STRINGS ${${UPPER_MODULE_NAME}_FILE} ${UPPER_MODULE_NAME}_BYPRODUCT REGEX "^[ \t]*BYPRODUCT_WIN=")
  else()
    file(STRINGS ${${UPPER_MODULE_NAME}_FILE} ${UPPER_MODULE_NAME}_BYPRODUCT REGEX "^[ \t]*BYPRODUCT=")
  endif()

  # Tarball Hash
  file(STRINGS ${${UPPER_MODULE_NAME}_FILE} ${UPPER_MODULE_NAME}_HASH_SHA256 REGEX "^[ \t]*SHA256=")
  file(STRINGS ${${UPPER_MODULE_NAME}_FILE} ${UPPER_MODULE_NAME}_HASH_SHA512 REGEX "^[ \t]*SHA512=")

  string(REGEX REPLACE ".*LIBNAME=([^ \t]*).*" "\\1" ${UPPER_MODULE_NAME}_LNAME "${${UPPER_MODULE_NAME}_LNAME}")
  string(REGEX REPLACE ".*VERSION=([^ \t]*).*" "\\1" ${UPPER_MODULE_NAME}_VER "${${UPPER_MODULE_NAME}_VER}")
  string(REGEX REPLACE ".*ARCHIVE=([^ \t]*).*" "\\1" ${UPPER_MODULE_NAME}_ARCHIVE "${${UPPER_MODULE_NAME}_ARCHIVE}")
  string(REGEX REPLACE ".*BASE_URL=([^ \t]*).*" "\\1" ${UPPER_MODULE_NAME}_BASE_URL "${${UPPER_MODULE_NAME}_BASE_URL}")
  if(WIN32 OR WINDOWS_STORE)
    string(REGEX REPLACE ".*BYPRODUCT_WIN=([^ \t]*).*" "\\1" ${UPPER_MODULE_NAME}_BYPRODUCT "${${UPPER_MODULE_NAME}_BYPRODUCT}")
  else()
    string(REGEX REPLACE ".*BYPRODUCT=([^ \t]*).*" "\\1" ${UPPER_MODULE_NAME}_BYPRODUCT "${${UPPER_MODULE_NAME}_BYPRODUCT}")
  endif()

  string(REGEX REPLACE "\\$\\(LIBNAME\\)" "${${UPPER_MODULE_NAME}_LNAME}" ${UPPER_MODULE_NAME}_ARCHIVE "${${UPPER_MODULE_NAME}_ARCHIVE}")
  string(REGEX REPLACE "\\$\\(VERSION\\)" "${${UPPER_MODULE_NAME}_VER}" ${UPPER_MODULE_NAME}_ARCHIVE "${${UPPER_MODULE_NAME}_ARCHIVE}")

  set(${UPPER_MODULE_NAME}_ARCHIVE ${${UPPER_MODULE_NAME}_ARCHIVE} PARENT_SCOPE)

  if(${UPPER_MODULE_NAME}_BYPRODUCT)
    # strip the extension, if debug, add DEBUG_POSTFIX and then add the extension back
    if(DEFINED ${UPPER_MODULE_NAME}_DEBUG_POSTFIX)
      set(_POSTFIX ${${UPPER_MODULE_NAME}_DEBUG_POSTFIX})
    else()
      set(_POSTFIX ${DEBUG_POSTFIX})
    endif()

    # Only add debug postfix if platform or module supply a DEBUG_POSTFIX
    if(DEFINED _POSTFIX AND NOT _POSTFIX STREQUAL "")
      string(REGEX REPLACE "\\.[^.]*$" "" ${UPPER_MODULE_NAME}_BYPRODUCT_DEBUG ${${UPPER_MODULE_NAME}_BYPRODUCT})
      if(WIN32 OR WINDOWS_STORE)
        set(${UPPER_MODULE_NAME}_BYPRODUCT_DEBUG "${${UPPER_MODULE_NAME}_BYPRODUCT_DEBUG}${_POSTFIX}.lib")
      else()
        set(${UPPER_MODULE_NAME}_BYPRODUCT_DEBUG "${${UPPER_MODULE_NAME}_BYPRODUCT_DEBUG}${_POSTFIX}.a")
      endif()
      # Set Debug library names
      set(${UPPER_MODULE_NAME}_LIBRARY_DEBUG ${DEPENDS_PATH}/lib/${${UPPER_MODULE_NAME}_BYPRODUCT_DEBUG} PARENT_SCOPE)
    endif()
    set(${UPPER_MODULE_NAME}_LIBRARY_RELEASE ${DEPENDS_PATH}/lib/${${UPPER_MODULE_NAME}_BYPRODUCT} PARENT_SCOPE)
    set(${UPPER_MODULE_NAME}_LIBRARY ${DEPENDS_PATH}/lib/${${UPPER_MODULE_NAME}_BYPRODUCT} PARENT_SCOPE)
  endif()

  set(${UPPER_MODULE_NAME}_INCLUDE_DIR ${DEPENDS_PATH}/include PARENT_SCOPE)
  set(${UPPER_MODULE_NAME}_VER ${${UPPER_MODULE_NAME}_VER} PARENT_SCOPE)

  if (${UPPER_MODULE_NAME}_BASE_URL)
    set(${UPPER_MODULE_NAME}_BASE_URL ${${UPPER_MODULE_NAME}_BASE_URL} PARENT_SCOPE)
  else()
    set(${UPPER_MODULE_NAME}_BASE_URL "http://mirrors.kodi.tv/build-deps/sources" PARENT_SCOPE)
  endif()
  set(${UPPER_MODULE_NAME}_BYPRODUCT ${${UPPER_MODULE_NAME}_BYPRODUCT} PARENT_SCOPE)

  if (${UPPER_MODULE_NAME}_HASH_SHA256)
    set(${UPPER_MODULE_NAME}_HASH ${${UPPER_MODULE_NAME}_HASH_SHA256} PARENT_SCOPE)
  elseif(${UPPER_MODULE_NAME}_HASH_SHA512)
    set(${UPPER_MODULE_NAME}_HASH ${${UPPER_MODULE_NAME}_HASH_SHA512} PARENT_SCOPE)
  endif()
endfunction()

# Macro to factor out the repetitive URL setup
macro(SETUP_BUILD_VARS)
  get_versionfile_data(${MODULE_LC})
  string(TOUPPER ${MODULE_LC} MODULE)

  # allow user to override the download URL with a local tarball
  # needed for offline build envs
  if(${MODULE}_URL)
    get_filename_component(${MODULE}_URL "${${MODULE}_URL}" ABSOLUTE)
  else()
    set(${MODULE}_URL ${${MODULE}_BASE_URL}/${${MODULE}_ARCHIVE})
  endif()
  if(VERBOSE)
    message(STATUS "${MODULE}_URL: ${${MODULE}_URL}")
  endif()

  # unset all build_dep_target variables to insure clean state
  unset(CMAKE_ARGS)
  unset(PATCH_COMMAND)
  unset(CONFIGURE_COMMAND)
  unset(BUILD_COMMAND)
  unset(INSTALL_COMMAND)
  unset(BUILD_IN_SOURCE)
  unset(BUILD_BYPRODUCTS)
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
macro(BUILD_DEP_TARGET)
  include(ExternalProject)

  if(CMAKE_ARGS)
    set(CMAKE_ARGS CMAKE_ARGS ${CMAKE_ARGS}
                             -DCMAKE_INSTALL_PREFIX=${DEPENDS_PATH}
                             -DCMAKE_INSTALL_LIBDIR=lib)
    if(CMAKE_TOOLCHAIN_FILE)
      list(APPEND CMAKE_ARGS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
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
        list(APPEND CMAKE_ARGS "-DCMAKE_BUILD_TYPE=$<CONFIG>")
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

  if(BUILD_BYPRODUCTS)
    set(BUILD_BYPRODUCTS BUILD_BYPRODUCTS ${BUILD_BYPRODUCTS})
  else()
    if(${MODULE}_BYPRODUCT)
      set(BUILD_BYPRODUCTS BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/${${MODULE}_BYPRODUCT})
    endif()
  endif()

  externalproject_add(${MODULE_LC}
                      URL ${${MODULE}_URL}
                      URL_HASH ${${MODULE}_HASH}
                      DOWNLOAD_DIR ${TARBALL_DIR}
                      DOWNLOAD_NAME ${${MODULE}_ARCHIVE}
                      PREFIX ${CORE_BUILD_DIR}/${MODULE_LC}
                      INSTALL_DIR ${DEPENDS_PATH}
                      ${CMAKE_ARGS}
                      ${PATCH_COMMAND}
                      ${CONFIGURE_COMMAND}
                      ${BUILD_COMMAND}
                      ${INSTALL_COMMAND}
                      ${BUILD_BYPRODUCTS}
                      ${BUILD_IN_SOURCE})

  set_target_properties(${MODULE_LC} PROPERTIES FOLDER "External Projects")
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


