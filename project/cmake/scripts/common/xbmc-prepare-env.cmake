# parse version.txt to get the version info
if(EXISTS "${XBMCROOT}/version.txt")
  file(STRINGS "${XBMCROOT}/version.txt" versions)
  foreach (version ${versions})
    string(REGEX MATCH "^[^ ]+" version_name ${version})
    string(REPLACE "${version_name} " "" version_value ${version})
    set(APP_${version_name} "${version_value}")
  endforeach()
endif()

# bail if we can't parse versions
if(NOT DEFINED APP_VERSION_MAJOR OR NOT DEFINED APP_VERSION_MINOR)
  message(FATAL_ERROR "Could not determine app version! make sure that ${XBMCROOT}/version.txt exists")
endif()

### copy all the addon binding header files to include/xbmc
# make sure include/xbmc exists and is empty
set(XBMC_LIB_DIR ${DEPENDS_PATH}/lib/xbmc)
if(NOT EXISTS "${XBMC_LIB_DIR}/")
  file(MAKE_DIRECTORY ${XBMC_LIB_DIR})
endif()

set(XBMC_INCLUDE_DIR ${DEPENDS_PATH}/include/xbmc)
if(NOT EXISTS "${XBMC_INCLUDE_DIR}/")
  file(MAKE_DIRECTORY ${XBMC_INCLUDE_DIR})
endif()

# xbmc-config.cmake.in (further down) expects a "prefix" variable
get_filename_component(prefix "${DEPENDS_PATH}" ABSOLUTE)

# generate the proper xbmc-config.cmake file
configure_file(${XBMCROOT}/project/cmake/xbmc-config.cmake.in ${XBMC_LIB_DIR}/xbmc-config.cmake @ONLY)
# copy cmake helpers to lib/xbmc
file(COPY ${XBMCROOT}/project/cmake/scripts/common/xbmc-addon-helpers.cmake ${XBMCROOT}/project/cmake/scripts/common/addoptions.cmake DESTINATION ${XBMC_LIB_DIR})

### copy all the addon binding header files to include/xbmc
# parse addon-bindings.mk to get the list of header files to copy
file(STRINGS ${XBMCROOT}/xbmc/addons/addon-bindings.mk bindings)
string(REPLACE "\n" ";" bindings "${bindings}")
foreach(binding ${bindings})
  string(REPLACE " =" ";" binding "${binding}")
  string(REPLACE "+=" ";" binding "${binding}")
  list(GET binding 1 header)
  # copy the header file to include/xbmc
  file(COPY ${XBMCROOT}/${header} DESTINATION ${XBMC_INCLUDE_DIR})
endforeach()