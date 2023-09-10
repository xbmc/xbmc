if(CORE_SYSTEM_NAME STREQUAL windows OR CORE_SYSTEM_NAME STREQUAL windowsstore)
  list(APPEND SOURCES DXVAHD.cpp)
  list(APPEND SOURCES DXVAEnumeratorHD.cpp)
  list(APPEND HEADERS DXVAHD.h)
  list(APPEND HEADERS DXVAEnumeratorHD.h)
endif()

if(VAAPI_FOUND)
  if(TARGET OpenGL::GL)
    list(APPEND SOURCES RendererVAAPIGL.cpp)
    list(APPEND HEADERS RendererVAAPIGL.h)
  endif()
  if(TARGET OpenGL::GLES)
    list(APPEND SOURCES RendererVAAPIGLES.cpp)
    list(APPEND HEADERS RendererVAAPIGLES.h)
  endif()
  if(TARGET EGL::EGL)
    list(APPEND SOURCES VaapiEGL.cpp)
    list(APPEND HEADERS VaapiEGL.h)
  endif()
endif()

if(TARGET VDPAU::VDPAU)
  list(APPEND SOURCES RendererVDPAU.cpp
                      VdpauGL.cpp)
  list(APPEND HEADERS RendererVDPAU.h
                      VdpauGL.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL osx)
  if(TARGET OpenGL::GL)
    list(APPEND SOURCES RendererVTBGL.cpp)
    list(APPEND HEADERS RendererVTBGL.h)
  endif()
endif()

if(CORE_SYSTEM_NAME STREQUAL darwin_embedded)
  if(TARGET OpenGL::GLES)
    list(APPEND SOURCES RendererVTBGLES.cpp)
    list(APPEND HEADERS RendererVTBGLES.h)
  endif()
endif()

if(CORE_SYSTEM_NAME STREQUAL android)
  list(APPEND SOURCES RendererMediaCodec.cpp
                      RendererMediaCodecSurface.cpp)
  list(APPEND HEADERS RendererMediaCodec.h
                      RendererMediaCodecSurface.h)
endif()

if("gbm" IN_LIST CORE_PLATFORM_NAME_LC OR "wayland" IN_LIST CORE_PLATFORM_NAME_LC)
  if("gbm" IN_LIST CORE_PLATFORM_NAME_LC)
    list(APPEND SOURCES RendererDRMPRIME.cpp
                        VideoLayerBridgeDRMPRIME.cpp)
    list(APPEND HEADERS RendererDRMPRIME.h
                        VideoLayerBridgeDRMPRIME.h)
  endif()

  if(TARGET OpenGL::GLES)
    list(APPEND SOURCES RendererDRMPRIMEGLES.cpp
                        DRMPRIMEEGL.cpp)
    list(APPEND HEADERS RendererDRMPRIMEGLES.h
                        DRMPRIMEEGL.h)
  endif()
endif()

if("webos" IN_LIST CORE_PLATFORM_NAME_LC)
  list(APPEND SOURCES RendererStarfish.cpp)
  list(APPEND HEADERS RendererStarfish.h)
endif()

# we might want to build on linux systems
# with ENABLE_VDPAU=OFF and ENABLE_VAAPI=OFF
if(SOURCES)
  core_add_library(videorenderers_hwdec)
endif()
