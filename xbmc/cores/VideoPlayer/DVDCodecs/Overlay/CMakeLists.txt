set(SOURCES DVDOverlayCodec.cpp
            DVDOverlayCodecFFmpeg.cpp
            DVDOverlayCodecSSA.cpp
            DVDOverlayCodecText.cpp
            DVDOverlayCodecTX3G.cpp
            contrib/cc_decoder.c
            contrib/cc_decoder708.cpp)

set(HEADERS DVDOverlay.h
            DVDOverlayCodec.h
            DVDOverlayCodecFFmpeg.h
            DVDOverlayCodecSSA.h
            DVDOverlayCodecTX3G.h
            DVDOverlayCodecText.h
            DVDOverlayImage.h
            DVDOverlaySSA.h
            DVDOverlaySpu.h
            DVDOverlayText.h
            contrib/cc_decoder.h
            contrib/cc_decoder708.h)

core_add_library(dvdoverlaycodecs)
