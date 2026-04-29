set(SOURCES AudioSinkAE.cpp
            DVDClock.cpp
            DVDDemuxSPU.cpp
            DVDFileInfo.cpp
            DVDMessage.cpp
            DVDMessageQueue.cpp
            DVDOverlayContainer.cpp
            DVDStreamInfo.cpp
            Edl.cpp
            Edl/EdlParserFactory.cpp
            Edl/EdlParsers/BeyondTVParser.cpp
            Edl/EdlParsers/ComskipParser.cpp
            Edl/EdlParsers/EdlFileParser.cpp
            Edl/EdlParsers/PvrEdlParser.cpp
            Edl/EdlParsers/VideoReDoParser.cpp
            PTSTracker.cpp
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
            Edl/EdlParser.h
            Edl/EdlParserFactory.h
            Edl/EdlParsers/BeyondTVParser.h
            Edl/EdlParsers/ComskipParser.h
            Edl/EdlParsers/EdlFileParser.h
            Edl/EdlParsers/PvrEdlParser.h
            Edl/EdlParsers/VideoReDoParser.h
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

if("webos" IN_LIST CORE_PLATFORM_NAME_LC)
    list(APPEND SOURCES MediaPipelineWebOS.cpp
                        MediaPipelineWebOSLegacy.cpp
                        VideoPlayerAudioWebOS.cpp
                        VideoPlayerVideoWebOS.cpp
                        VideoPlayerWebOS.cpp)
    list(APPEND HEADERS MediaPipelineWebOS.h
                        VideoPlayerAudioWebOS.h
                        VideoPlayerVideoWebOS.h
                        VideoPlayerWebOS.h)
endif()

core_add_library(VideoPlayer)
