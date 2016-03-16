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

### processing additional tools required by the platform
if(EXISTS ${APP_ROOT}/project/cmake/scripts/${CORE_SYSTEM_NAME}/tools/)
  file(GLOB platform_tools ${APP_ROOT}/project/cmake/scripts/${CORE_SYSTEM_NAME}/tools/*.cmake)
  foreach(platform_tool ${platform_tools})
    get_filename_component(platform_tool_name ${platform_tool} NAME_WE)
    message(STATUS "Processing ${CORE_SYSTEM_NAME} specific tool: ${platform_tool_name}")

    # include the file
    include(${platform_tool})
  endforeach()
endif()
