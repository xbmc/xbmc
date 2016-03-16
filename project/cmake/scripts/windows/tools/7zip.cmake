find_program(7ZIP_FOUND NAMES 7z 7za 7z.exe 7za.exe)
if(7ZIP_FOUND)
  message(STATUS "7-Zip utility found at ${7ZIP_FOUND}")
else()
  set(7ZIP_ARCHIVE_NAME "7z-9.20-bin")
  set(7ZIP_ARCHIVE "${7ZIP_ARCHIVE_NAME}.zip")
  set(7ZIP_URL "http://mirrors.xbmc.org/build-deps/win32/${7ZIP_ARCHIVE}")
  set(7ZIP_DOWNLOAD ${BUILD_DIR}/download/${7ZIP_ARCHIVE})

  # download the archive containing 7za.exe
  message(STATUS "Downloading 7-Zip utility from ${7ZIP_URL}...")
  file(DOWNLOAD "${7ZIP_URL}" "${7ZIP_DOWNLOAD}" STATUS 7ZIP_DL_STATUS LOG 7ZIP_LOG SHOW_PROGRESS)
  list(GET 7ZIP_DL_STATUS 0 7ZIP_RETCODE)
  if(NOT ${7ZIP_RETCODE} EQUAL 0)
    message(FATAL_ERROR "ERROR downloading ${7ZIP_URL} - status: ${7ZIP_DL_STATUS} log: ${7ZIP_LOG}")
  endif()

  # extract the archive containing 7za.exe
  execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzvf ${7ZIP_DOWNLOAD}
                  WORKING_DIRECTORY ${BUILD_DIR})

  # make sure the extraction worked and that 7za.exe is there
  set(7ZIP_PATH ${BUILD_DIR}/${7ZIP_ARCHIVE_NAME})
  set(7ZIP_BINARY_PATH ${7ZIP_PATH}/7za.exe)
  if(NOT EXISTS ${7ZIP_PATH} OR NOT EXISTS ${7ZIP_BINARY_PATH})
    message(FATAL_ERROR "ERROR extracting 7-Zip utility from ${7ZIP_PATH}")
  endif()

  # copy 7za.exe into the output directory
  file(INSTALL ${7ZIP_BINARY_PATH} DESTINATION ${DEPENDS_PATH}/bin)

  # make sure that cmake can find the copied 7za.exe
  find_program(7ZIP_FOUND NAMES 7z 7za 7z.exe 7za.exe)
  if(NOT 7ZIP_FOUND)
    message(FATAL_ERROR "ERROR installing 7-Zip utility from ${7ZIP_BINARY_PATH} to ${DEPENDS_PATH}/bin")
  endif()
endif()
