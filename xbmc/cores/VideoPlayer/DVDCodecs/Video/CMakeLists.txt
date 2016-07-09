set(SOURCES DVDVideoCodec.cpp
            DVDVideoCodecFFmpeg.cpp)

set(HEADERS DVDVideoCodec.h
            DVDVideoCodecFFmpeg.h)

if(NOT ENABLE_EXTERNAL_LIBAV)
  list(APPEND SOURCES DVDVideoPPFFmpeg.cpp)
  list(APPEND HEADERS DVDVideoPPFFmpeg.h)
endif()

if(AML_FOUND)
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

if(APPLE)
  list(APPEND SOURCES VTB.cpp)
  list(APPEND HEADERS DllVideoToolBox.h
                      VTB.h)
endif()

if(MMAL_FOUND)
  list(APPEND SOURCES MMALCodec.cpp)
  list(APPEND HEADERS MMALCodec.h)
  list(APPEND SOURCES MMALFFmpeg.cpp)
  list(APPEND HEADERS MMALFFmpeg.h)
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
