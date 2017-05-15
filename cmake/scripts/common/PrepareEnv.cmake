# parse version.txt and versions.h to get the version and API info
include(${CORE_SOURCE_DIR}/cmake/scripts/common/Macros.cmake)
core_find_versions()

# in case we need to download something, set KODI_MIRROR to the default if not already set
if(NOT DEFINED KODI_MIRROR)
  set(KODI_MIRROR "http://mirrors.kodi.tv")
endif()

### copy all the addon binding header files to include/kodi
# make sure include/kodi exists and is empty
set(APP_LIB_DIR ${ADDON_DEPENDS_PATH}/lib/${APP_NAME_LC})
if(NOT EXISTS "${APP_LIB_DIR}/")
  file(MAKE_DIRECTORY ${APP_LIB_DIR})
endif()

set(APP_DATA_DIR ${ADDON_DEPENDS_PATH}/share/${APP_NAME_LC})
if(NOT EXISTS "${APP_DATA_DIR}/")
  file(MAKE_DIRECTORY ${APP_DATA_DIR})
endif()

set(APP_INCLUDE_DIR ${ADDON_DEPENDS_PATH}/include/${APP_NAME_LC})
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
configure_file(${CORE_SOURCE_DIR}/cmake/KodiConfig.cmake.in ${APP_LIB_DIR}/KodiConfig.cmake @ONLY)

# copy cmake helpers to lib/kodi
file(COPY ${CORE_SOURCE_DIR}/cmake/scripts/common/AddonHelpers.cmake
          ${CORE_SOURCE_DIR}/cmake/scripts/common/AddOptions.cmake
     DESTINATION ${APP_LIB_DIR})

### copy all the addon binding header files to include/kodi
# parse addon-bindings.mk to get the list of header files to copy
core_file_read_filtered(bindings ${CORE_SOURCE_DIR}/xbmc/addons/addon-bindings.mk)
foreach(header ${bindings})
  # copy the header file to include/kodi
  configure_file(${CORE_SOURCE_DIR}/${header} ${APP_INCLUDE_DIR} COPYONLY)
endforeach()

### processing additional tools required by the platform
if(EXISTS ${CORE_SOURCE_DIR}/cmake/scripts/${CORE_SYSTEM_NAME}/tools/)
  file(GLOB platform_tools ${CORE_SOURCE_DIR}/cmake/scripts/${CORE_SYSTEM_NAME}/tools/*.cmake)
  foreach(platform_tool ${platform_tools})
    get_filename_component(platform_tool_name ${platform_tool} NAME_WE)
    message(STATUS "Processing ${CORE_SYSTEM_NAME} specific tool: ${platform_tool_name}")

    # include the file
    include(${platform_tool})
  endforeach()
endif()
