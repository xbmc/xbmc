set(SOURCES DemuxMultiSource.cpp
            DemuxTimeline.cpp
            DVDDemux.cpp
            DVDDemuxBXA.cpp
            DVDDemuxCC.cpp
            DVDDemuxCDDA.cpp
            DVDDemuxClient.cpp
            DVDDemuxFFmpeg.cpp
            DVDDemuxUtils.cpp
            DVDDemuxVobsub.cpp
            DVDFactoryDemuxer.cpp
            EbmlParser.cpp
            MatroskaParser.cpp)

set(HEADERS DemuxMultiSource.h
            DemuxTimeline.h
            DVDDemux.h
            DVDDemuxBXA.h
            DVDDemuxCC.h
            DVDDemuxCDDA.h
            DVDDemuxClient.h
            DVDDemuxFFmpeg.h
            DVDDemuxUtils.h
            DVDDemuxVobsub.h
            DVDFactoryDemuxer.h
            EbmlParser.h
            MatroskaParser.h)

core_add_library(dvddemuxers)
