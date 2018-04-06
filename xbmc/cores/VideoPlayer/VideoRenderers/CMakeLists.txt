set(SOURCES BaseRenderer.cpp
            ColorManager.cpp
            OverlayRenderer.cpp
            OverlayRendererGUI.cpp
            OverlayRendererUtil.cpp
            RenderCapture.cpp
            RenderFactory.cpp
            RenderFlags.cpp
            RenderManager.cpp
            DebugRenderer.cpp)

set(HEADERS BaseRenderer.h
            ColorManager.h
            OverlayRenderer.h
            OverlayRendererGUI.h
            OverlayRendererUtil.h
            RenderCapture.h
            RenderFactory.h
            RenderFlags.h
            RenderInfo.h
            RenderManager.h
            DebugRenderer.h)

if(CORE_SYSTEM_NAME STREQUAL windows OR CORE_SYSTEM_NAME STREQUAL windowsstore)
  list(APPEND SOURCES WinRenderer.cpp
                      WinRenderBuffer.cpp
                      OverlayRendererDX.cpp)
  list(APPEND HEADERS WinRenderer.h
                      WinRenderBuffer.h
                      OverlayRendererDX.h)
endif()

if (OPENGL_FOUND OR OPENGLES_FOUND)
  list(APPEND SOURCES OverlayRendererGL.cpp)
  list(APPEND HEADERS OverlayRendererGL.h)
endif()

if(OPENGL_FOUND)
  list(APPEND SOURCES LinuxRendererGL.cpp
                      FrameBufferObject.cpp)
  list(APPEND HEADERS LinuxRendererGL.h
                      FrameBufferObject.h)
endif()

if(OPENGLES_FOUND AND (CORE_PLATFORM_NAME_LC STREQUAL android OR
                       CORE_PLATFORM_NAME_LC STREQUAL ios OR
                       CORE_PLATFORM_NAME_LC STREQUAL aml OR
                       CORE_PLATFORM_NAME_LC STREQUAL gbm OR
                       CORE_PLATFORM_NAME_LC STREQUAL wayland))
  list(APPEND SOURCES LinuxRendererGLESBase.cpp
                      LinuxRendererGLES2.cpp
                      FrameBufferObject.cpp)
  list(APPEND HEADERS LinuxRendererGLESBase.h
                      LinuxRendererGLES2.h
                      FrameBufferObject.cpp)

  if(OPENGLES3_FOUND)
    list(APPEND SOURCES LinuxRendererGLES3.cpp)
    list(APPEND HEADERS LinuxRendererGLES3.h)
  endif()
endif()

core_add_library(videorenderers)
