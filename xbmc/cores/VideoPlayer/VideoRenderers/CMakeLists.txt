set(SOURCES BaseRenderer.cpp
            ColorManager.cpp
            OverlayRenderer.cpp
            OverlayRendererUtil.cpp
            RenderCapture.cpp
            RenderFactory.cpp
            RenderFlags.cpp
            RenderManager.cpp
            DebugRenderer.cpp)

set(HEADERS BaseRenderer.h
            ColorManager.h
            DebugInfo.h
            OverlayRenderer.h
            OverlayRendererUtil.h
            RenderCapture.h
            RenderFactory.h
            RenderFlags.h
            RenderInfo.h
            RenderManager.h
            DebugRenderer.h)

if(CORE_SYSTEM_NAME STREQUAL windows OR CORE_SYSTEM_NAME STREQUAL windowsstore)
  list(APPEND SOURCES WinRenderer.cpp
                      OverlayRendererDX.cpp
                      RenderCaptureDX.cpp)
  list(APPEND HEADERS WinRenderer.h
                      OverlayRendererDX.h
                      RenderCapture.h)
endif()

if (OPENGL_FOUND OR OPENGLES_FOUND)
  list(APPEND SOURCES OverlayRendererGL.cpp
                      FrameBufferObject.cpp)
  list(APPEND HEADERS OverlayRendererGL.h
                      FrameBufferObject.h)
endif()

if(OPENGL_FOUND)
  list(APPEND SOURCES LinuxRendererGL.cpp
                      RenderCaptureGL.cpp)
  list(APPEND HEADERS LinuxRendererGL.h
                      RenderCaptureGL.h)
endif()

if(OPENGLES_FOUND AND ("android" IN_LIST CORE_PLATFORM_NAME_LC OR
                       "ios" IN_LIST CORE_PLATFORM_NAME_LC OR
                       "tvos" IN_LIST CORE_PLATFORM_NAME_LC OR
                       "gbm" IN_LIST CORE_PLATFORM_NAME_LC OR
                       "x11" IN_LIST CORE_PLATFORM_NAME_LC OR
                       "wayland" IN_LIST CORE_PLATFORM_NAME_LC))
  list(APPEND SOURCES LinuxRendererGLES.cpp
                      RenderCaptureGLES.cpp)
  list(APPEND HEADERS LinuxRendererGLES.h
                      RenderCaptureGLES.h)
endif()

core_add_library(videorenderers)
