project(kodi-addon-executable)

cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(C CXX)

include(../dev-kit-api-level.cmake)

#find_package(kodi)
find_package(kodiplatform REQUIRED)
find_package(p8-platform REQUIRED)

add_definitions(-DUSE_DEMUX)

list(APPEND LIB_SOURCES src/InterProcess.cpp
                        src/RequestPacket.cpp
                        src/ResponsePacket.cpp
                        src/SharedMemory.cpp
                        src/SharedMemoryPosix.cpp
                        src/SharedMemoryWindows.cpp
                        src/tools.cpp
                        src/main.cpp
                        src/addon/InterProcess_Addon_General.cpp
                        src/addon/InterProcess_Addon_Codec.cpp
                        src/addon/InterProcess_Addon_Network.cpp
                        src/addon/InterProcess_Addon_SoundPlay.cpp
                        src/addon/InterProcess_Addon_VFSUtils.cpp
                        src/audioengine/InterProcess_AudioEngine_General.cpp
                        src/audioengine/InterProcess_AudioEngine_Stream.cpp
                        src/gui/InterProcess_GUI_ControlButton.cpp
                        src/gui/InterProcess_GUI_ControlEdit.cpp
                        src/gui/InterProcess_GUI_ControlFadeLabel.cpp
                        src/gui/InterProcess_GUI_ControlImage.cpp
                        src/gui/InterProcess_GUI_ControlLabel.cpp
                        src/gui/InterProcess_GUI_ControlProgress.cpp
                        src/gui/InterProcess_GUI_ControlRadioButton.cpp
                        src/gui/InterProcess_GUI_ControlRendering.cpp
                        src/gui/InterProcess_GUI_ControlSettingsSlider.cpp
                        src/gui/InterProcess_GUI_ControlSlider.cpp
                        src/gui/InterProcess_GUI_ControlSpin.cpp
                        src/gui/InterProcess_GUI_ControlTextBox.cpp
                        src/gui/InterProcess_GUI_DialogExtendedProgress.cpp
                        src/gui/InterProcess_GUI_DialogFileBrowser.cpp
                        src/gui/InterProcess_GUI_DialogKeyboard.cpp
                        src/gui/InterProcess_GUI_DialogNumeric.cpp
                        src/gui/InterProcess_GUI_DialogOK.cpp
                        src/gui/InterProcess_GUI_DialogProgress.cpp
                        src/gui/InterProcess_GUI_DialogSelect.cpp
                        src/gui/InterProcess_GUI_DialogTextViewer.cpp
                        src/gui/InterProcess_GUI_DialogYesNo.cpp
                        src/gui/InterProcess_GUI_General.cpp
                        src/gui/InterProcess_GUI_ListItem.cpp
                        src/gui/InterProcess_GUI_Window.cpp
                        src/player/InterProcess_Player_InfoTagMusic.cpp
                        src/player/InterProcess_Player_InfoTagVideo.cpp
                        src/player/InterProcess_Player_Player.cpp
                        src/player/InterProcess_Player_PlayList.cpp
                        src/pvr/InterProcess_PVR_General.cpp
                        src/pvr/InterProcess_PVR_Transfer.cpp
                        src/pvr/InterProcess_PVR_Trigger.cpp
                        ../shared-code/AddonLib_cpp.cpp
                        ../shared-code/addon/AddonLib_Addon_Codec_cpp.cpp
                        ../shared-code/addon/AddonLib_Addon_General_cpp.cpp
                        ../shared-code/addon/AddonLib_Addon_Network_cpp.cpp
                        ../shared-code/addon/AddonLib_Addon_SoundPlay_cpp.cpp
                        ../shared-code/addon/AddonLib_Addon_VFSUtils_cpp.cpp
                        ../shared-code/audioengine/AddonLib_AudioEngine_General_cpp.cpp
                        ../shared-code/audioengine/AddonLib_AudioEngine_Stream_cpp.cpp
                        ../shared-code/gui/AddonLib_GUI_ControlButton_cpp.cpp
                        ../shared-code/gui/AddonLib_GUI_ControlEdit_cpp.cpp
                        ../shared-code/gui/AddonLib_GUI_ControlFadeLabel_cpp.cpp
                        ../shared-code/gui/AddonLib_GUI_ControlImage_cpp.cpp
                        ../shared-code/gui/AddonLib_GUI_ControlLabel_cpp.cpp
                        ../shared-code/gui/AddonLib_GUI_ControlProgress_cpp.cpp
                        ../shared-code/gui/AddonLib_GUI_ControlRadioButton_cpp.cpp
                        ../shared-code/gui/AddonLib_GUI_ControlRendering_cpp.cpp
                        ../shared-code/gui/AddonLib_GUI_ControlSettingsSlider_cpp.cpp
                        ../shared-code/gui/AddonLib_GUI_ControlSlider_cpp.cpp
                        ../shared-code/gui/AddonLib_GUI_ControlSpin_cpp.cpp
                        ../shared-code/gui/AddonLib_GUI_ControlTextBox_cpp.cpp
                        ../shared-code/gui/AddonLib_GUI_DialogExtendedProgress_cpp.cpp
                        ../shared-code/gui/AddonLib_GUI_DialogFileBrowser_cpp.cpp
                        ../shared-code/gui/AddonLib_GUI_DialogKeyboard_cpp.cpp
                        ../shared-code/gui/AddonLib_GUI_DialogNumeric_cpp.cpp
                        ../shared-code/gui/AddonLib_GUI_DialogOK_cpp.cpp
                        ../shared-code/gui/AddonLib_GUI_DialogProgress_cpp.cpp
                        ../shared-code/gui/AddonLib_GUI_DialogSelect_cpp.cpp
                        ../shared-code/gui/AddonLib_GUI_DialogTextViewer_cpp.cpp
                        ../shared-code/gui/AddonLib_GUI_DialogYesNo_cpp.cpp
                        ../shared-code/gui/AddonLib_GUI_General_cpp.cpp
                        ../shared-code/gui/AddonLib_GUI_ListItem_cpp.cpp
                        ../shared-code/gui/AddonLib_GUI_Window_cpp.cpp
                        ../shared-code/player/AddonLib_Player_InfoTagMusic_cpp.cpp
                        ../shared-code/player/AddonLib_Player_InfoTagVideo_cpp.cpp
                        ../shared-code/player/AddonLib_Player_Player_cpp.cpp
                        ../shared-code/player/AddonLib_Player_Playlist_cpp.cpp
                        ../shared-code/pvr/AddonLib_PVR_General_cpp.cpp
                        ../shared-code/pvr/AddonLib_PVR_Stream_cpp.cpp
                        ../shared-code/pvr/AddonLib_PVR_Transfer_cpp.cpp
                        ../shared-code/pvr/AddonLib_PVR_Trigger_cpp.cpp
                        ../shared-code/ErrorCodeNames.cpp)

if(WIN32)
  list(APPEND DEPLIBS ${p8-platform_LIBRARIES} ws2_32)
else()
  list(APPEND DEPLIBS ${p8-platform_LIBRARIES})
endif()

include_directories(${INCLUDES}
                    ../include/
                    src/
                    ${kodiplatform_INCLUDE_DIRS}
                    ${p8-platform_INCLUDE_DIRS}
                    ${KODI_INCLUDE_DIR})

add_executable(kodi.addon.executable ${LIB_SOURCES})
target_link_libraries(kodi.addon.executable ${DEPLIBS})
set_target_properties(kodi.addon.executable PROPERTIES OUTPUT_NAME kodi.addon.executable.api${ADDON_API_LEVEL})
