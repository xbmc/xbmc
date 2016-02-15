set(SOURCES DVDVideoCodec.cpp
            DVDVideoCodecFFmpeg.cpp)

set(HEADERS DVDVideoCodec.h
            DVDVideoCodecFFmpeg.h)

if(NOT ENABLE_EXTERNAL_LIBAV)
  list(APPEND SOURCES DVDVideoPPFFmpeg.cpp)
  list(APPEND HEADERS DVDVideoPPFFmpeg.h)
endif()

if(AMLCODEC_FOUND)
  list(APPEND SOURCES AMLCodec.cpp
                      DVDVideoCodecAmlogic.cpp)
  list(APPEND HEADERS AMLCodec.h
                      DVDVideoCodecAmlogic.h)
endif()

if(OPENMAX_FOUND)
  list(APPEND SOURCES DVDVideoCodecOpenMax.cpp
                      OpenMax.cpp
                      OpenMaxVideo.cpp)
  list(APPEND HEADERS DVDVideoCodecOpenMax.h
                      OpenMax.h
                      OpenMaxVideo.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL windows)
  list(APPEND SOURCES DXVA.cpp)
  list(APPEND HEADERS DXVA.h)
endif()

if(VDPAU_FOUND)
  list(APPEND SOURCES VDPAU.cpp)
  list(APPEND HEADERS VDPAU.h)
endif()

if(VAAPI_FOUND)
  list(APPEND SOURCES VAAPI.cpp)
  list(APPEND HEADERS VAAPI.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL darwin)
  list(APPEND SOURCES DVDVideoCodecVDA.cpp
                      VDA.cpp)
  list(APPEND HEADERS DVDVideoCodecVDA.h
                      VDA.h)
endif()

if(VTB_FOUND)
  list(APPEND SOURCES DVDVideoCodecVideoToolBox.cpp)
  list(APPEND HEADERS DllVideoToolBox.h
                      DVDVideoCodecVideoToolBox.h)
endif()

if(MMAL_FOUND)
  list(APPEND SOURCES MMALCodec.cpp)
  list(APPEND HEADERS MMALCodec.h)
endif()

if(IMX_FOUND)
  list(APPEND SOURCES DVDVideoCodecIMX.cpp)
  list(APPEND HEADERS DVDVideoCodecIMX.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL android)
  list(APPEND SOURCES DVDVideoCodecAndroidMediaCodec.cpp)
  list(APPEND HEADERS DVDVideoCodecAndroidMediaCodec.h)
endif()

core_add_library(dvdvideocodecs)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(dvdvideocodecs ffmpeg)
endif()
