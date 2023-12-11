#.rst:
# FindPatch
# ----------
# Finds patch executable
# Windows platforms will download patch zip from mirrors if not found.
#
# This will define the following variables::
#
# PATCH_EXECUTABLE - patch executable

if(CMAKE_HOST_WIN32)

  find_package(Git)
  if(Git_FOUND)
    get_filename_component(GIT_DIR ${GIT_EXECUTABLE} DIRECTORY)
  endif()

  find_program(PATCH_EXE NAMES patch.exe HINTS "${GIT_DIR}/.." PATH_SUFFIXES usr/bin)
else()
  # Freebsd patch is insufficient (too old) look for gnu patch first
  find_program(PATCH_EXE NAMES gpatch patch)
endif()

if(CMAKE_HOST_WIN32 AND NOT PATCH_EXE)
  set(PATCH_ARCHIVE_NAME "patch-2.7.6-bin")
  set(PATCH_ARCHIVE "${PATCH_ARCHIVE_NAME}.zip")
  set(PATCH_URL "${KODI_MIRROR}/build-deps/win32/${PATCH_ARCHIVE}")
  set(PATCH_DOWNLOAD ${TARBALL_DIR}/${PATCH_ARCHIVE})

  # download the archive containing patch.exe
  message(STATUS "Downloading patch utility from ${PATCH_URL}...")
  file(DOWNLOAD "${PATCH_URL}" "${PATCH_DOWNLOAD}" STATUS PATCH_DL_STATUS LOG PATCH_LOG SHOW_PROGRESS)
  list(GET PATCH_DL_STATUS 0 PATCH_RETCODE)
  if(NOT PATCH_RETCODE EQUAL 0)
    message(FATAL_ERROR "ERROR downloading ${PATCH_URL} - status: ${PATCH_DL_STATUS} log: ${PATCH_LOG}")
  endif()

  # CORE_BUILD_DIR may not exist as yet, so create just in case
  if(NOT EXISTS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR})
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR})
  endif()

  # extract the archive containing patch.exe
  execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzvf ${PATCH_DOWNLOAD}
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR})

  # make sure the extraction worked and that patch.exe is there
  set(PATCH_PATH ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/${PATCH_ARCHIVE_NAME})
  if(NOT EXISTS "${PATCH_PATH}/bin/patch.exe")
    message(FATAL_ERROR "ERROR extracting patch utility from ${PATCH_PATH}")
  endif()

  # copy patch.exe into the output directory
  file(INSTALL "${PATCH_PATH}/bin/patch.exe" DESTINATION ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/bin)
  # copy patch depends
  file(GLOB PATCH_BINARIES ${PATCH_PATH}/bin/*.dll)
  if(NOT "${PATCH_BINARIES}" STREQUAL "")
    file(INSTALL ${PATCH_BINARIES} DESTINATION ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/bin)
  endif()

  # make sure that cmake can find the copied patch.exe
  find_program(PATCH_EXE NAMES patch.exe HINTS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/bin REQUIRED)
endif()

if(PATCH_EXE)
  execute_process(COMMAND "${PATCH_EXE}" --version
                  OUTPUT_VARIABLE PATCH_VERSION
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REGEX MATCH "[^\n]*patch [^\n]*" PATCH_VERSION "${PATCH_VERSION}")
  string(REGEX REPLACE ".*patch (.*)" "\\1" PATCH_VERSION "${PATCH_VERSION}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Patch REQUIRED_VARS PATCH_EXE VERSION_VAR PATCH_VERSION)

if(PATCH_FOUND)
  set(PATCH_EXECUTABLE "${PATCH_EXE}")
  mark_as_advanced(PATCH_EXE)
endif()
