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
endif()

# bail if we can't parse versions
if(NOT DEFINED APP_VERSION_MAJOR OR NOT DEFINED APP_VERSION_MINOR)
  message(FATAL_ERROR "Could not determine app version! make sure that ${APP_ROOT}/version.txt exists")
endif()

### copy all the addon binding header files to include/kodi
# make sure include/kodi exists and is empty
set(KODI_LIB_DIR ${DEPENDS_PATH}/lib/kodi)
if(NOT EXISTS "${KODI_LIB_DIR}/")
  file(MAKE_DIRECTORY ${KODI_LIB_DIR})
endif()

set(KODI_INCLUDE_DIR ${DEPENDS_PATH}/include/kodi)
if(NOT EXISTS "${KODI_INCLUDE_DIR}/")
  file(MAKE_DIRECTORY ${KODI_INCLUDE_DIR})
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

# kodi-config.cmake.in (further down) expects a "prefix" variable
get_filename_component(prefix "${DEPENDS_PATH}" ABSOLUTE)

# generate the proper kodi-config.cmake file
configure_file(${APP_ROOT}/project/cmake/kodi-config.cmake.in ${KODI_LIB_DIR}/kodi-config.cmake @ONLY)
# copy cmake helpers to lib/kodi
file(COPY ${APP_ROOT}/project/cmake/scripts/common/addon-helpers.cmake ${APP_ROOT}/project/cmake/scripts/common/addoptions.cmake DESTINATION ${KODI_LIB_DIR})

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
  file(COPY ${APP_ROOT}/${header} DESTINATION ${KODI_INCLUDE_DIR})

  # auto-generate header files for backwards comaptibility to xbmc with deprecation warning
  get_filename_component(headerfile ${header} NAME)
  file(WRITE ${XBMC_INCLUDE_DIR}/${headerfile}
"#pragma once
#define DEPRECATION_WARNING \"Including xbmc/${headerfile} has been deprecated, please use kodi/${headerfile}\"
#ifdef _MSC_VER
  #pragma message(\"WARNING: \" DEPRECATION_WARNING)
#else
  #warning DEPRECATION_WARNING
#endif
#include \"kodi/${headerfile}\"")
endforeach()