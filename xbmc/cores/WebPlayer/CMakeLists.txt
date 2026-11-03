if(TARGET ${APP_NAME_LC}::WaylandServer)
  set(SOURCES GUIDialogWebKeyboard.cpp
              GUIDialogWebOSD.cpp
              GUIWindowFullScreenWeb.cpp
              WebAudioServer.cpp
              WebClientManager.cpp
              WebPlayer.cpp
              WebVideoBuffer.cpp
              WebVideoServer.cpp)

  set(HEADERS GUIDialogWebKeyboard.h
              GUIDialogWebOSD.h
              GUIWindowFullScreenWeb.h
              WebApp.h
              WebAudioServer.h
              WebClientManager.h
              WebPlayer.h
              WebPlayerDefs.h
              WebVideoBuffer.h
              WebVideoServer.h
              kodi-web-audio.h
              kodi-web-video.h)

  if("gbm" IN_LIST CORE_PLATFORM_NAME_LC AND TARGET ${APP_NAME_LC}::OpenGLES)
    list(APPEND SOURCES WebLayerBridge.cpp)
    list(APPEND HEADERS WebLayerBridge.h)
  endif()

  include_directories(${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/platform/linux/waylandhost)

  core_add_library(webplayer)
endif()
