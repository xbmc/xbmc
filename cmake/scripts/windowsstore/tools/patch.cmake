set(_doc "Patch command line executable")
set(_patch_path )

#cmake can't handle ENV{PROGRAMFILES(X86)} so
#use a hack where we append it ourselves
set(_patch_path
  "$ENV{LOCALAPPDATA}/Programs/Git/bin"
  "$ENV{LOCALAPPDATA}/Programs/Git/usr/bin"
  "$ENV{APPDATA}/Programs/Git/bin"
  "$ENV{APPDATA}/Programs/Git/usr/bin"
  "$ENV{PROGRAMFILES}/Git/bin"
  "$ENV{PROGRAMFILES}/Git/usr/bin"
  "$ENV{PROGRAMFILES} (x86)/Git/bin"
  "$ENV{PROGRAMFILES} (x86)/Git/usr/bin"
  )

# First search the PATH
find_program(PATCH_EXECUTABLE
  NAME patch
  PATHS ${_patch_path}
  DOC ${_doc}
  NO_DEFAULT_PATH
  )

if(PATCH_EXECUTABLE AND NOT TARGET Patch::patch AND NOT PATCH_EXECUTABLE MATCHES Strawberry)
  add_executable(Patch::patch IMPORTED)
  set_property(TARGET Patch::patch PROPERTY IMPORTED_LOCATION ${PATCH_EXECUTABLE})
endif()

unset(_patch_path)
unset(_doc)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Patch
                                  REQUIRED_VARS PATCH_EXECUTABLE)

if(PATCH_FOUND)
  message(STATUS "patch utility found at ${PATCH_EXECUTABLE}")
else()
  set(PATCH_ARCHIVE_NAME "patch-2.7.6-bin")
  set(PATCH_ARCHIVE "${PATCH_ARCHIVE_NAME}.zip")
  set(PATCH_URL "${KODI_MIRROR}/build-deps/win32/${PATCH_ARCHIVE}")
  set(PATCH_DOWNLOAD ${BUILD_DIR}/download/${PATCH_ARCHIVE})

  # download the archive containing patch.exe
  message(STATUS "Downloading patch utility from ${PATCH_URL}...")
  file(DOWNLOAD "${PATCH_URL}" "${PATCH_DOWNLOAD}" STATUS PATCH_DL_STATUS LOG PATCH_LOG SHOW_PROGRESS)
  list(GET PATCH_DL_STATUS 0 PATCH_RETCODE)
  if(NOT PATCH_RETCODE EQUAL 0)
    message(FATAL_ERROR "ERROR downloading ${PATCH_URL} - status: ${PATCH_DL_STATUS} log: ${PATCH_LOG}")
  endif()

  # extract the archive containing patch.exe
  execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzvf ${PATCH_DOWNLOAD}
                  WORKING_DIRECTORY ${BUILD_DIR})

  # make sure the extraction worked and that patch.exe is there
  set(PATCH_PATH ${BUILD_DIR}/${PATCH_ARCHIVE_NAME})
  set(PATCH_BINARY_PATH ${PATCH_PATH}/bin/patch.exe)
  if(NOT EXISTS ${PATCH_PATH} OR NOT EXISTS ${PATCH_BINARY_PATH})
    message(FATAL_ERROR "ERROR extracting patch utility from ${PATCH_PATH}")
  endif()

  # copy patch.exe into the output directory
  file(INSTALL ${PATCH_BINARY_PATH} DESTINATION ${ADDON_DEPENDS_PATH}/bin)
  # copy patch depends
  file(GLOB PATCH_BINARIES ${PATCH_PATH}/bin/*.dll)
  if(NOT "${PATCH_BINARIES}" STREQUAL "")
    file(INSTALL ${PATCH_BINARIES} DESTINATION ${ADDON_DEPENDS_PATH}/bin)
  endif()

  # make sure that cmake can find the copied patch.exe
  find_program(PATCH_EXECUTABLE NAMES patch patch.exe)
  find_package_handle_standard_args(Patch
                                    REQUIRED_VARS PATCH_EXECUTABLE)
  if(NOT PATCH_FOUND)
    message(FATAL_ERROR "ERROR installing patch utility from ${PATCH_BINARY_PATH} to ${ADDON_DEPENDS_PATH}/bin")
  endif()
endif()
