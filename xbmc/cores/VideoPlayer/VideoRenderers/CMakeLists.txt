enable_language(CXX ASM)
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp" )

set(SOURCES BaseRenderer.cpp
            ColorManager.cpp
            OverlayRenderer.cpp
            OverlayRendererGUI.cpp
            OverlayRendererUtil.cpp
            RenderCapture.cpp
            RenderFlags.cpp
            RenderManager.cpp
            DebugRenderer.cpp)

set(HEADERS BaseRenderer.h
            ColorManager.h
            OverlayRenderer.h
            OverlayRendererGUI.h
            OverlayRendererUtil.h
            RenderCapture.h
            RenderFlags.h
            RenderFormats.h
            RenderManager.h
            DebugRenderer.h)

if(CORE_SYSTEM_NAME STREQUAL windows)
  list(APPEND SOURCES WinRenderer.cpp
                      OverlayRendererDX.cpp)
  list(APPEND HEADERS WinRenderer.h
                      OverlayRendererDX.h)
endif()

if(OPENGL_FOUND)
  list(APPEND SOURCES OverlayRendererGL.cpp
                      LinuxRendererGL.cpp
                      FrameBufferObject.cpp)
  list(APPEND HEADERS OverlayRendererGL.h
                      LinuxRendererGL.h
                      FrameBufferObject.h)
endif()

if(OPENGLES_FOUND)
  list(APPEND SOURCES LinuxRendererGLES.cpp
                      OverlayRendererGL.cpp
                      FrameBufferObject.cpp)
  list(APPEND HEADERS LinuxRendererGLES.h
                      OverlayRendererGL.h
                      FrameBufferObject.cpp)
endif()

if(ARCH MATCHES arm)
  list(APPEND SOURCES yuv2rgb.neon.S)
  list(APPEND HEADERS yuv2rgb.neon.h)
endif()

core_add_library(videorenderers)
add_dependencies(videorenderers libcpluff)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(videorenderers ffmpeg)
endif()

if(CORE_SYSTEM_NAME STREQUAL windows)
  add_dependencies(videorenderers d3dx11effects)
endif()
