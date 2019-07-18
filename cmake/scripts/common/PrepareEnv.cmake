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

if(NOT CORE_SYSTEM_NAME)
  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CORE_SYSTEM_NAME "osx")
  else()
    string(TOLOWER ${CMAKE_SYSTEM_NAME} CORE_SYSTEM_NAME)
  endif()
endif()

set(PLATFORM_TAG ${CORE_SYSTEM_NAME})

if(CORE_SYSTEM_NAME STREQUAL android)
  if (CPU MATCHES "v7a")
    set(PLATFORM_TAG ${PLATFORM_TAG}-armv7)
  elseif (CPU MATCHES "arm64")
    set(PLATFORM_TAG ${PLATFORM_TAG}-aarch64)
  elseif (CPU MATCHES "i686")
    set(PLATFORM_TAG ${PLATFORM_TAG}-i686)
  else()
    message(FATAL_ERROR "Unsupported architecture")
  endif()
elseif(CORE_SYSTEM_NAME STREQUAL darwin_embedded)
  if (CPU MATCHES armv7)
    set(PLATFORM_TAG ${PLATFORM_TAG}-armv7)
  elseif (CPU MATCHES arm64)
    set(PLATFORM_TAG ${PLATFORM_TAG}-aarch64)
  else()
    message(FATAL_ERROR "Unsupported architecture")
  endif()
elseif(CORE_SYSTEM_NAME STREQUAL osx)
  set(PLATFORM_TAG ${PLATFORM_TAG}-${CPU})
elseif(CORE_SYSTEM_NAME STREQUAL windows)
  include(CheckSymbolExists)
  check_symbol_exists(_X86_ "Windows.h" _X86_)
  check_symbol_exists(_AMD64_ "Windows.h" _AMD64_)

  if(_X86_)
    set(PLATFORM_TAG ${PLATFORM_TAG}-i686)
  elseif(_AMD64_)
    set(PLATFORM_TAG ${PLATFORM_TAG}-x86_64)
  else()
    message(FATAL_ERROR "Unsupported architecture")
  endif()

  unset(_X86_)
  unset(_AMD64_)
endif()

# generate the proper KodiConfig.cmake file
configure_file(${CORE_SOURCE_DIR}/cmake/KodiConfig.cmake.in ${APP_LIB_DIR}/KodiConfig.cmake @ONLY)

# copy cmake helpers to lib/kodi
file(COPY ${CORE_SOURCE_DIR}/cmake/scripts/common/AddonHelpers.cmake
          ${CORE_SOURCE_DIR}/cmake/scripts/common/AddOptions.cmake
     DESTINATION ${APP_LIB_DIR})

### copy all the addon binding header files to include/kodi
include(${CORE_SOURCE_DIR}/xbmc/addons/AddonBindings.cmake)
file(COPY ${CORE_ADDON_BINDINGS_FILES} ${CORE_ADDON_BINDINGS_DIRS}/
     DESTINATION ${APP_INCLUDE_DIR}
     REGEX ".txt" EXCLUDE)

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
