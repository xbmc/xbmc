# parse version.txt and libKODI_guilib.h to get the version and API info
include(${CORE_SOURCE_DIR}/project/cmake/scripts/common/Macros.cmake)
core_find_versions()

# in case we need to download something, set KODI_MIRROR to the default if not alread set
if(NOT DEFINED KODI_MIRROR)
  set(KODI_MIRROR "http://mirrors.kodi.tv")
endif()

### copy all the addon binding header files to include/kodi
# make sure include/kodi exists and is empty
set(APP_LIB_DIR ${DEPENDS_PATH}/lib/${APP_NAME_LC})
if(NOT EXISTS "${APP_LIB_DIR}/")
  file(MAKE_DIRECTORY ${APP_LIB_DIR})
endif()

set(APP_INCLUDE_DIR ${DEPENDS_PATH}/include/${APP_NAME_LC})
if(NOT EXISTS "${APP_INCLUDE_DIR}/")
  file(MAKE_DIRECTORY ${APP_INCLUDE_DIR})
endif()

# make sure C++11 is always set
if(NOT WIN32)
  string(REGEX MATCH "-std=(gnu|c)\\+\\+11" cxx11flag "${CMAKE_CXX_FLAGS}")
  if(NOT cxx11flag)
    set(CXX11_SWITCH "-std=c++11")
  endif()
endif()

# generate the proper KodiConfig.cmake file
configure_file(${CORE_SOURCE_DIR}/project/cmake/KodiConfig.cmake.in ${APP_LIB_DIR}/KodiConfig.cmake @ONLY)

# copy cmake helpers to lib/kodi
file(COPY ${CORE_SOURCE_DIR}/project/cmake/scripts/common/AddonHelpers.cmake
          ${CORE_SOURCE_DIR}/project/cmake/scripts/common/AddOptions.cmake
     DESTINATION ${APP_LIB_DIR})

### copy all the addon binding header files to include/kodi
# parse addon-bindings.mk to get the list of header files to copy
file(STRINGS ${CORE_SOURCE_DIR}/xbmc/addons/addon-bindings.mk bindings)
string(REPLACE "\n" ";" bindings "${bindings}")
foreach(binding ${bindings})
  string(REPLACE " =" ";" binding "${binding}")
  string(REPLACE "+=" ";" binding "${binding}")
  list(GET binding 1 header)
  # copy the header file to include/kodi
  file(COPY ${CORE_SOURCE_DIR}/${header} DESTINATION ${APP_INCLUDE_DIR})
endforeach()

### on windows we need a "patch" binary to be able to patch 3rd party sources
if(WIN32)
  find_program(PATCH_FOUND NAMES patch patch.exe)
  if(PATCH_FOUND)
    message(STATUS "patch utility found at ${PATCH_FOUND}")
  else()
    set(PATCH_ARCHIVE_NAME "patch-2.5.9-7-bin-3")
    set(PATCH_ARCHIVE "${PATCH_ARCHIVE_NAME}.zip")
    set(PATCH_URL "${KODI_MIRROR}/build-deps/win32/${PATCH_ARCHIVE}")
    set(PATCH_DOWNLOAD ${BUILD_DIR}/download/${PATCH_ARCHIVE})

    # download the archive containing patch.exe
    message(STATUS "Downloading patch utility from ${PATCH_URL}...")
    file(DOWNLOAD "${PATCH_URL}" "${PATCH_DOWNLOAD}" STATUS PATCH_DL_STATUS LOG PATCH_LOG SHOW_PROGRESS)
    list(GET PATCH_DL_STATUS 0 PATCH_RETCODE)
    if(NOT ${PATCH_RETCODE} EQUAL 0)
      message(FATAL_ERROR "ERROR downloading ${PATCH_URL} - status: ${PATCH_DL_STATUS} log: ${PATCH_LOG}")
    endif()

    # extract the archive containing patch.exe
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzvf ${PATCH_DOWNLOAD}
                    WORKING_DIRECTORY ${BUILD_DIR})

    # make sure the extraction worked and that patch.exe is there
    set(PATCH_PATH ${BUILD_DIR}/${PATCH_ARCHIVE_NAME})
    set(PATCH_BINARY_PATH ${PATCH_PATH}/bin/patch.exe)
    if(NOT EXISTS ${PATCH_PATH} OR NOT EXISTS ${PATCH_BINARY_PATH})
      message(FATAL_ERROR "ERROR extracting patch utility from ${PATCH_DOWNLOAD_DIR}")
    endif()

    # copy patch.exe into the output directory
    file(INSTALL ${PATCH_BINARY_PATH} DESTINATION ${DEPENDS_PATH}/bin)

    # make sure that cmake can find the copied patch.exe
    find_program(PATCH_FOUND NAMES patch patch.exe)
    if(NOT PATCH_FOUND)
      message(FATAL_ERROR "ERROR installing patch utility from ${PATCH_BINARY_PATH} to ${DEPENDS_PATH}/bin")
    endif()
  endif()
endif()
