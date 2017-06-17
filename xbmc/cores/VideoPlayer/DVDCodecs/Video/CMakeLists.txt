set(SOURCES DVDVideoCodec.cpp
            DVDVideoCodecFFmpeg.cpp
            DVDVideoPPFFmpeg.cpp)

set(HEADERS DVDVideoCodec.h
            DVDVideoCodecFFmpeg.h
            DVDVideoPPFFmpeg.h)

if(AML_FOUND)
  list(APPEND SOURCES AMLCodec.cpp
                      DVDVideoCodecAmlogic.cpp)
  list(APPEND HEADERS AMLCodec.h
                      DVDVideoCodecAmlogic.h)
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
add_platform_override(${CORE_LIBRARY} HWAccel.h PLATFORMS osx X11 windows)
