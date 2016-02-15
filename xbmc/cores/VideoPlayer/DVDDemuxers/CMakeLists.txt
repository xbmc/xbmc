set(SOURCES DVDDemux.cpp
            DVDDemuxBXA.cpp
            DVDDemuxCC.cpp
            DVDDemuxCDDA.cpp
            DVDDemuxClient.cpp
            DVDDemuxFFmpeg.cpp
            DVDDemuxUtils.cpp
            DVDDemuxVobsub.cpp
            DVDFactoryDemuxer.cpp)

set(HEADERS DVDDemux.h
            DVDDemuxBXA.h
            DVDDemuxCC.h
            DVDDemuxCDDA.h
            DVDDemuxClient.h
            DVDDemuxFFmpeg.h
            DVDDemuxPacket.h
            DVDDemuxUtils.h
            DVDDemuxVobsub.h
            DVDFactoryDemuxer.h)

core_add_library(dvddemuxers)
add_dependencies(dvddemuxers libcpluff)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(dvddemuxers ffmpeg)
endif()

if(NOT CORE_SYSTEM_NAME STREQUAL windows)
  add_dependencies(dvddemuxers dvdnav)
endif()
