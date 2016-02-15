set(SOURCES DVDAudio.cpp
            DVDClock.cpp
            DVDDemuxSPU.cpp
            DVDFileInfo.cpp
            DVDMessage.cpp
            DVDMessageQueue.cpp
            DVDOverlayContainer.cpp
            DVDStreamInfo.cpp
            DVDTSCorrection.cpp
            Edl.cpp
            VideoPlayerAudio.cpp
            VideoPlayer.cpp
            VideoPlayerRadioRDS.cpp
            VideoPlayerSubtitle.cpp
            VideoPlayerTeletext.cpp
            VideoPlayerVideo.cpp)

set(HEADERS DVDAudio.h
            DVDClock.h
            DVDDemuxSPU.h
            DVDFileInfo.h
            DVDMessage.h
            DVDMessageQueue.h
            DVDOverlayContainer.h
            DVDResource.h
            DVDStreamInfo.h
            DVDTSCorrection.h
            Edl.h
            IVideoPlayer.h
            VideoPlayer.h
            VideoPlayerAudio.h
            VideoPlayerRadioRDS.h
            VideoPlayerSubtitle.h
            VideoPlayerTeletext.h
            VideoPlayerVideo.h)

core_add_library(VideoPlayer)
add_dependencies(VideoPlayer libcpluff)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(VideoPlayer ffmpeg)
endif()

if(NOT CORE_SYSTEM_NAME STREQUAL windows)
  add_dependencies(VideoPlayer dvdnav)
endif()
