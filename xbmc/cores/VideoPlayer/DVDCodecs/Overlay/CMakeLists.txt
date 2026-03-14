set(SOURCES DVDOverlayCodec.cpp
            DVDOverlayCodecFFmpeg.cpp
            DVDOverlayCodecSSA.cpp
            DVDOverlayCodecText.cpp
            DVDOverlayCodecCCText.cpp
            DVDOverlayCodecTX3G.cpp
            OverlayCodecWebVTT.cpp)

set(HEADERS DVDOverlay.h
            DVDOverlayCodec.h
            DVDOverlayCodecFFmpeg.h
            DVDOverlayCodecSSA.h
            DVDOverlayCodecTX3G.h
            DVDOverlayCodecText.h
            DVDOverlayCodecCCText.h
            DVDOverlayImage.h
            DVDOverlayLibass.h
            DVDOverlaySSA.h
            DVDOverlaySpu.h
            DVDOverlayText.h
            OverlayCodecWebVTT.h)

core_add_library(dvdoverlaycodecs)
