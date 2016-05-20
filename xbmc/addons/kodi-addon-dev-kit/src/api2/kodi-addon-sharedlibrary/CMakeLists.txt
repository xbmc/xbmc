project(kodi-addon-sharedlibrary-api2)

cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(C CXX)

include(../dev-kit-api-level.cmake)
include(../../../UseMultiArch.cmake)

set(kodi-addon-sharedlibrary_NAME kodi-addon-sharedlibrary)
set(kodi-addon-sharedlibrary_DESCRIPTION "Kodi add-on API 3 development kit for shared libraries")
set(kodi-addon-sharedlibrary_API_LEVEL=${ADDON_API_LEVEL})
set(kodi-addon-sharedlibrary_VERSION ${ADDON_API_VERSION})

add_definitions(-DUSE_DEMUX)
add_definitions(-DADDON_API_LEVEL=${ADDON_API_LEVEL})

set(SOURCES InterProcess.cpp
            AddonLib.cpp
            ErrorCodeNames.cpp
            addon/AddonLib_Addon_General.cpp
            addon/AddonLib_Addon_Network.cpp
            addon/AddonLib_Addon_SoundPlay.cpp
            addon/AddonLib_Addon_VFSUtils.cpp
            audioengine/AddonLib_AudioEngine_General.cpp
            audioengine/AddonLib_AudioEngine_Stream.cpp
            gui/AddonLib_GUI_General.cpp
            gui/AddonLib_GUI_ControlButton.cpp
            gui/AddonLib_GUI_ControlEdit.cpp
            gui/AddonLib_GUI_ControlFadeLabel.cpp
            gui/AddonLib_GUI_ControlImage.cpp
            gui/AddonLib_GUI_ControlLabel.cpp
            gui/AddonLib_GUI_ControlProgress.cpp
            gui/AddonLib_GUI_ControlRadioButton.cpp
            gui/AddonLib_GUI_ControlRendering.cpp
            gui/AddonLib_GUI_ControlSettingsSlider.cpp
            gui/AddonLib_GUI_ControlSlider.cpp
            gui/AddonLib_GUI_ControlSpin.cpp
            gui/AddonLib_GUI_ControlTextBox.cpp
            gui/AddonLib_GUI_DialogContextMenu.cpp
            gui/AddonLib_GUI_DialogExtendedProgress.cpp
            gui/AddonLib_GUI_DialogFileBrowser.cpp
            gui/AddonLib_GUI_DialogKeyboard.cpp
            gui/AddonLib_GUI_DialogNumeric.cpp
            gui/AddonLib_GUI_DialogOK.cpp
            gui/AddonLib_GUI_DialogProgress.cpp
            gui/AddonLib_GUI_DialogSelect.cpp
            gui/AddonLib_GUI_DialogTextViewer.cpp
            gui/AddonLib_GUI_DialogYesNo.cpp
            gui/AddonLib_GUI_ListItem.cpp
            gui/AddonLib_GUI_Window.cpp
            inputstream/AddonLib_InputStream.cpp
            peripheral/AddonLib_Peripheral.cpp
            player/AddonLib_Player_InfoTagMusic.cpp
            player/AddonLib_Player_InfoTagVideo.cpp
            player/AddonLib_Player_Player.cpp
            player/AddonLib_Player_PlayList.cpp
            pvr/AddonLib_PVR_General.cpp
            pvr/AddonLib_PVR_Stream.cpp
            pvr/AddonLib_PVR_Transfer.cpp
            pvr/AddonLib_PVR_Trigger.cpp)
            
set(HEADERS InterProcess.h
            ErrorCodeNames.h
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
            ../../../include/kodi/libKODI_adsp.h
            ../../../include/kodi/libKODI_audioengine.h
            ../../../include/kodi/libKODI_guilib.h
            ../../../include/kodi/libKODI_inputstream.h
            ../../../include/kodi/libKODI_peripheral.h
            ../../../include/kodi/libXBMC_addon.h
            ../../../include/kodi/libXBMC_codec.h
            ../../../include/kodi/libXBMC_pvr.h
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
  list(APPEND SOURCES dlfcn-win32.cpp)
  list(APPEND HEADERS dlfcn-win32.h)
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++11")
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
  configure_file(kodi-addon-sharedlibrary-api2.pc.in ${CMAKE_BINARY_DIR}/kodi-addon-sharedlibrary-api2.pc @ONLY)
  install(FILES ${CMAKE_BINARY_DIR}/kodi-addon-sharedlibrary-api2.pc
          DESTINATION ${CMAKE_INSTALL_LIBDIR_NOARCH}/pkgconfig)
ENDIF(NOT WIN32)

# config mode
configure_file (kodi-addon-sharedlibrary-api2-config.cmake.in
                ${CMAKE_BINARY_DIR}/kodi-addon-sharedlibrary-api2-config.cmake @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/kodi-addon-sharedlibrary-api2-config.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR_NOARCH}/kodi)
