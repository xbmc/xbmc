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

if(TARGET OpenGL::GL OR TARGET OpenGL::GLES)
  list(APPEND SOURCES FrameBufferObject.cpp)
  list(APPEND HEADERS FrameBufferObject.h)
endif()

if(TARGET OpenGL::GL)
  list(APPEND SOURCES LinuxRendererGL.cpp
                      OverlayRendererGL.cpp
                      RenderCaptureGL.cpp)
  list(APPEND HEADERS LinuxRendererGL.h
                      OverlayRendererGL.h
                      RenderCaptureGL.h)
endif()

if(TARGET OpenGL::GLES AND ("android" IN_LIST CORE_PLATFORM_NAME_LC OR
                            "ios" IN_LIST CORE_PLATFORM_NAME_LC OR
                            "tvos" IN_LIST CORE_PLATFORM_NAME_LC OR
                            "gbm" IN_LIST CORE_PLATFORM_NAME_LC OR
                            "x11" IN_LIST CORE_PLATFORM_NAME_LC OR
                            "wayland" IN_LIST CORE_PLATFORM_NAME_LC))
  list(APPEND SOURCES LinuxRendererGLES.cpp
                      OverlayRendererGLES.cpp
                      RenderCaptureGLES.cpp)
  list(APPEND HEADERS LinuxRendererGLES.h
                      OverlayRendererGLES.cpp
                      RenderCaptureGLES.h)
endif()

core_add_library(videorenderers)
