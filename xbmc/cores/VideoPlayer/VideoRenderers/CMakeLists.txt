set(SOURCES BaseRenderer.cpp
            ColorManager.cpp
            OverlayRenderer.cpp
            OverlayRendererUtil.cpp
            RenderFactory.cpp
            RenderFlags.cpp
            RenderManager.cpp
            DebugRenderer.cpp)

set(HEADERS BaseRenderer.h
            ColorManager.h
            DebugInfo.h
            OverlayRenderer.h
            OverlayRendererUtil.h
            RenderFactory.h
            RenderFlags.h
            RenderInfo.h
            RenderManager.h
            DebugRenderer.h)

if(CORE_SYSTEM_NAME STREQUAL windows OR CORE_SYSTEM_NAME STREQUAL windowsstore)
  list(APPEND SOURCES WinRenderer.cpp
                      OverlayRendererDX.cpp)
  list(APPEND HEADERS WinRenderer.h
                      OverlayRendererDX.h)
endif()

if(TARGET ${APP_NAME_LC}::OpenGl OR TARGET ${APP_NAME_LC}::OpenGLES)
  list(APPEND SOURCES FrameBufferObject.cpp)
  list(APPEND HEADERS FrameBufferObject.h)
endif()

if(TARGET ${APP_NAME_LC}::OpenGl)
  list(APPEND SOURCES LinuxRendererGL.cpp
                      OverlayRendererGL.cpp)
  list(APPEND HEADERS LinuxRendererGL.h
                      OverlayRendererGL.h)
endif()

if(TARGET ${APP_NAME_LC}::OpenGLES AND ("android" IN_LIST CORE_PLATFORM_NAME_LC OR
                            "ios" IN_LIST CORE_PLATFORM_NAME_LC OR
                            "tvos" IN_LIST CORE_PLATFORM_NAME_LC OR
                            "gbm" IN_LIST CORE_PLATFORM_NAME_LC OR
                            "x11" IN_LIST CORE_PLATFORM_NAME_LC OR
                            "wayland" IN_LIST CORE_PLATFORM_NAME_LC))
  list(APPEND SOURCES LinuxRendererGLES.cpp
                      OverlayRendererGLES.cpp)
  list(APPEND HEADERS LinuxRendererGLES.h
                      OverlayRendererGLES.cpp)
endif()

core_add_library(videorenderers)
