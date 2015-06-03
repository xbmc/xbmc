# parse version.txt to get the version info
if(EXISTS "${APP_ROOT}/version.txt")
  file(STRINGS "${APP_ROOT}/version.txt" versions)
  foreach (version ${versions})
    if(version MATCHES "^VERSION_.*")
      string(REGEX MATCH "^[^ ]+" version_name ${version})
      string(REPLACE "${version_name} " "" version_value ${version})
      set(APP_${version_name} "${version_value}")
    else()
      string(REGEX MATCH "^[^ ]+" name ${version})
      string(REPLACE "${name} " "" value ${version})
      set(${name} "${value}")
    endif()
  endforeach()
  string(TOLOWER ${APP_NAME} APP_NAME_LC)
  string(TOUPPER ${APP_NAME} APP_NAME_UC)
endif()

# bail if we can't parse versions
if(NOT DEFINED APP_VERSION_MAJOR OR NOT DEFINED APP_VERSION_MINOR)
  message(FATAL_ERROR "Could not determine app version! make sure that ${APP_ROOT}/version.txt exists")
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

# we still need XBMC_INCLUDE_DIR and XBMC_LIB_DIR for backwards compatibility to xbmc
set(XBMC_LIB_DIR ${DEPENDS_PATH}/lib/xbmc)
if(NOT EXISTS "${XBMC_LIB_DIR}/")
  file(MAKE_DIRECTORY ${XBMC_LIB_DIR})
endif()
set(XBMC_INCLUDE_DIR ${DEPENDS_PATH}/include/xbmc)
if(NOT EXISTS "${XBMC_INCLUDE_DIR}/")
  file(MAKE_DIRECTORY ${XBMC_INCLUDE_DIR})
endif()

# make sure C++11 is always set
if(NOT WIN32)
  string(REGEX MATCH "-std=(gnu|c)\\+\\+11" cxx11flag "${CMAKE_CXX_FLAGS}")
  if(NOT cxx11flag)
    set(CXX11_SWITCH "-std=c++11")
  endif()
endif()

# generate the proper kodi-config.cmake file
configure_file(${APP_ROOT}/project/cmake/kodi-config.cmake.in ${APP_LIB_DIR}/kodi-config.cmake @ONLY)

# copy cmake helpers to lib/kodi
file(COPY ${APP_ROOT}/project/cmake/scripts/common/addon-helpers.cmake
          ${APP_ROOT}/project/cmake/scripts/common/addoptions.cmake
     DESTINATION ${APP_LIB_DIR})

# generate xbmc-config.cmake for backwards compatibility to xbmc
configure_file(${APP_ROOT}/project/cmake/xbmc-config.cmake.in ${XBMC_LIB_DIR}/xbmc-config.cmake @ONLY)

### copy all the addon binding header files to include/kodi
# parse addon-bindings.mk to get the list of header files to copy
file(STRINGS ${APP_ROOT}/xbmc/addons/addon-bindings.mk bindings)
string(REPLACE "\n" ";" bindings "${bindings}")
foreach(binding ${bindings})
  string(REPLACE " =" ";" binding "${binding}")
  string(REPLACE "+=" ";" binding "${binding}")
  list(GET binding 1 header)
  # copy the header file to include/kodi
  file(COPY ${APP_ROOT}/${header} DESTINATION ${APP_INCLUDE_DIR})

  # auto-generate header files for backwards compatibility to xbmc with deprecation warning
  # but only do it if the file doesn't already exist
  get_filename_component(headerfile ${header} NAME)
  if (NOT EXISTS "${XBMC_INCLUDE_DIR}/${headerfile}")
    file(WRITE ${XBMC_INCLUDE_DIR}/${headerfile}
"#pragma once
#define DEPRECATION_WARNING \"Including xbmc/${headerfile} has been deprecated, please use kodi/${headerfile}\"
#ifdef _MSC_VER
  #pragma message(\"WARNING: \" DEPRECATION_WARNING)
#else
  #warning DEPRECATION_WARNING
#endif
#include \"kodi/${headerfile}\"")
  endif()
endforeach()

### on windows we need a "patch" binary to be able to patch 3rd party sources
if(WIN32)
  find_program(PATCH_FOUND NAMES patch patch.exe)
  if(PATCH_FOUND)
    message(STATUS "patch utility found at ${PATCH_FOUND}")
  else()
    set(PATCH_ARCHIVE_NAME "patch-2.5.9-7-bin-1")
    set(PATCH_ARCHIVE "${PATCH_ARCHIVE_NAME}.zip")
    set(PATCH_URL "http://mirrors.xbmc.org/build-deps/win32/${PATCH_ARCHIVE}")
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
