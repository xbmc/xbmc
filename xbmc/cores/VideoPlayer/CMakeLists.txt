set(SOURCES AudioSinkAE.cpp
            DVDClock.cpp
            DVDDemuxSPU.cpp
            DVDFileInfo.cpp
            DVDMessage.cpp
            DVDMessageQueue.cpp
            DVDOverlayContainer.cpp
            DVDStreamInfo.cpp
            PTSTracker.cpp
            Edl.cpp
            VideoPlayer.cpp
            VideoPlayerAudio.cpp
            VideoPlayerAudioID3.cpp
            VideoPlayerRadioRDS.cpp
            VideoPlayerSubtitle.cpp
            VideoPlayerTeletext.cpp
            VideoPlayerVideo.cpp
            VideoReferenceClock.cpp)

set(HEADERS AudioSinkAE.h
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
            VideoPlayer.h
            VideoPlayerAudio.h
            VideoPlayerAudioID3.h
            VideoPlayerRadioRDS.h
            VideoPlayerSubtitle.h
            VideoPlayerTeletext.h
            VideoPlayerVideo.h
            VideoReferenceClock.h
            Interface/StreamInfo.h
            Interface/DemuxPacket.h
            Interface/DemuxCrypto.h
            Interface/InputStreamConstants.h
            Interface/TimingConstants.h)

core_add_library(VideoPlayer)
