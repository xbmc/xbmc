if(CORE_SYSTEM_NAME STREQUAL windows)
  list(APPEND SOURCES DXVAHD.cpp)
  list(APPEND HEADERS DXVAHD.h)
endif()

if(VAAPI_FOUND)
  list(APPEND SOURCES RendererVAAPI.cpp)
  list(APPEND HEADERS RendererVAAPI.h)
endif()

if(VDPAU_FOUND)
  list(APPEND SOURCES RendererVDPAU.cpp)
  list(APPEND HEADERS RendererVDPAU.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL darwin)
  list(APPEND SOURCES RendererVDA.cpp)
  list(APPEND HEADERS RendererVDA.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL ios)
  list(APPEND SOURCES RendererVTB.cpp)
  list(APPEND HEADERS RendererVTB.h)
endif()

if(MMAL_FOUND)
  list(APPEND SOURCES MMALRenderer.cpp)
  list(APPEND HEADERS MMALRenderer.h)
endif()

if(AML_FOUND)
  list(APPEND SOURCES RendererAML.cpp)
  list(APPEND HEADERS RendererAML.h)
endif()

if(OPENMAX_FOUND)
  list(APPEND SOURCES RendererOpenMax.cpp)
  list(APPEND HEADERS RendererOpenMax.h)
endif()

if(IMX_FOUND)
  list(APPEND SOURCES RendererIMX.cpp)
  list(APPEND HEADERS RendererIMX.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL android)
  list(APPEND SOURCES RendererMediaCodec.cpp
                      RendererMediaCodecSurface.cpp)
  list(APPEND HEADERS RendererMediaCodec.h
                      RendererMediaCodecSurface.h)
endif()

core_add_library(videorenderers_hwdec)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(videorenderers_hwdec ffmpeg)
endif()
