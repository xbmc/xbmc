set(SOURCES DVDAudio.cpp
            DVDClock.cpp
            DVDDemuxSPU.cpp
            DVDFileInfo.cpp
            DVDMessage.cpp
            DVDMessageQueue.cpp
            DVDOverlayContainer.cpp
            DVDStreamInfo.cpp
            PTSTracker.cpp
            Edl.cpp
            VideoPlayerAudio.cpp
            VideoPlayer.cpp
            VideoPlayerRadioRDS.cpp
            VideoPlayerSubtitle.cpp
            VideoPlayerTeletext.cpp
            VideoPlayerVideo.cpp
            VideoReferenceClock.cpp)

set(HEADERS DVDAudio.h
            DVDClock.h
            DVDDemuxSPU.h
            DVDFileInfo.h
            DVDMessage.h
            DVDMessageQueue.h
            DVDOverlayContainer.h
            DVDResource.h
            DVDStreamInfo.h
            Edl.h
            IVideoPlayer.h
            PTSTracker.h
            TimingConstants.h
            VideoPlayer.h
            VideoPlayerAudio.h
            VideoPlayerRadioRDS.h
            VideoPlayerSubtitle.h
            VideoPlayerTeletext.h
            VideoPlayerVideo.h
            VideoReferenceClock.h)

core_add_library(VideoPlayer)
