# -------------------------------------------------------------------------------
# Kodi add-on static library
# -------------------------------------------------------------------------------
# This static library is for primary communication as callbacks from add-on to
# kodi. Further becomes it used for independent binary add-on types to bring
# related functions after request from Kodi to add-on. All other calls from Kodi
# to add-on (e.g. PVR, ADSP, inputstream ...) becomes done by related headers.
#
# This library is only for general parts for all add-on types.
#
# The used API level is defined by the DevKitAPILevel.cmake file for cmake and
# by the definitions.hpp on related include directory. Whit this is on API level
# changes the amount of changed parts very low.
#

include(../DevKitAPILevel.cmake)

project(kodi-addon-sharedlibrary-api${ADDON_API_LEVEL})

cmake_minimum_required(VERSION 3.1.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

include(../../../UseMultiArch.cmake)

set(kodi-addon-sharedlibrary_NAME kodi-addon-sharedlibrary)
set(kodi-addon-sharedlibrary_DESCRIPTION "Kodi add-on API ${ADDON_API_LEVEL} development kit for shared libraries")
set(kodi-addon-sharedlibrary_API_LEVEL=${ADDON_API_LEVEL})
set(kodi-addon-sharedlibrary_VERSION ${ADDON_API_VERSION})

set(SOURCES Interface.cpp
            AddonLib.cpp
            ErrorCodeNames.cpp)

set(HEADERS Interface.h
            ErrorCodeNames.h
            ../../../include/kodi/api${ADDON_API_LEVEL}/AddonLib.hpp
            ../../../include/kodi/api${ADDON_API_LEVEL}/definitions-all.hpp
            ../../../include/kodi/api${ADDON_API_LEVEL}/definitions.hpp
            ../../../include/kodi/api${ADDON_API_LEVEL}/internal/AddonLib_internal.hpp
            ../../../include/kodi/kodi_adsp_dll.h
            ../../../include/kodi/kodi_adsp_types.h
            ../../../include/kodi/kodi_audiodec_dll.h
            ../../../include/kodi/kodi_audiodec_types.h
            ../../../include/kodi/kodi_audioengine_types.h
            ../../../include/kodi/kodi_inputstream_dll.h
            ../../../include/kodi/kodi_inputstream_types.h
            ../../../include/kodi/kodi_peripheral_callbacks.h
            ../../../include/kodi/kodi_peripheral_dll.h
            ../../../include/kodi/kodi_peripheral_types.h
            ../../../include/kodi/kodi_peripheral_utils.hpp
            ../../../include/kodi/kodi_vfs_types.h
            ../../../include/kodi/kodi_vfs_utils.hpp
            ../../../include/kodi/xbmc_addon_cpp_dll.h
            ../../../include/kodi/xbmc_addon_dll.h
            ../../../include/kodi/xbmc_addon_types.h
            ../../../include/kodi/xbmc_audioenc_dll.h
            ../../../include/kodi/xbmc_audioenc_types.h
            ../../../include/kodi/xbmc_codec_types.h
            ../../../include/kodi/xbmc_epg_types.h
            ../../../include/kodi/xbmc_pvr_dll.h
            ../../../include/kodi/xbmc_pvr_types.h
            ../../../include/kodi/xbmc_scr_dll.h
            ../../../include/kodi/xbmc_scr_types.h
            ../../../include/kodi/xbmc_vis_dll.h
            ../../../include/kodi/xbmc_vis_types.h)

if(WIN32)
  list(APPEND DEPLIBS ws2_32)
else()
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(BUILD_KODI_ADDON)
  list(APPEND INCLUDES ${KODI_INCLUDE_DIR})
else()
  list(APPEND INCLUDES ../../../../../
                       ../../../../../cores/VideoPlayer/DVDDemuxers)
endif()

include_directories(${INCLUDES}
                    ../../../include/
                    .)

add_library(kodi.addon.sharedlibrary.api${ADDON_API_LEVEL} STATIC ${SOURCES} ${HEADERS})
target_link_libraries(kodi.addon.sharedlibrary.api${ADDON_API_LEVEL} ${DEPLIBS})
target_compile_definitions(kodi.addon.sharedlibrary.api${ADDON_API_LEVEL}
                           PUBLIC
                           -DUSE_DEMUX
                           -DADDON_API_LEVEL=${ADDON_API_LEVEL})
set_target_properties(kodi.addon.sharedlibrary.api${ADDON_API_LEVEL} PROPERTIES OUTPUT_NAME kodi.addon.sharedlibrary.api${ADDON_API_LEVEL})

install(TARGETS kodi.addon.sharedlibrary.api${ADDON_API_LEVEL} DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Set this way for compatibility to old configure system of kodi
set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX})
set(libdir ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
set(includedir ${CMAKE_INSTALL_PREFIX}/include)
set(CXX ${CMAKE_CXX_COMPILER} ${CXX_STD0X_FLAGS})
set(CC ${CMAKE_C_COMPILER} ${C_STD99_FLAGS})
set(CXXFLAGS ${CMAKE_CXX_FLAGS})

IF(NOT WIN32)
  configure_file(kodi-addon-sharedlibrary-api${ADDON_API_LEVEL}.pc.in ${CMAKE_BINARY_DIR}/kodi-addon-sharedlibrary-api${ADDON_API_LEVEL}.pc @ONLY)
  install(FILES ${CMAKE_BINARY_DIR}/kodi-addon-sharedlibrary-api${ADDON_API_LEVEL}.pc
          DESTINATION ${CMAKE_INSTALL_LIBDIR_NOARCH}/pkgconfig)
ENDIF(NOT WIN32)

# config mode
configure_file (kodi-addon-sharedlibrary-api${ADDON_API_LEVEL}-config.cmake.in
                ${CMAKE_BINARY_DIR}/kodi-addon-sharedlibrary-api${ADDON_API_LEVEL}-config.cmake @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/kodi-addon-sharedlibrary-api${ADDON_API_LEVEL}-config.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR_NOARCH}/kodi)
