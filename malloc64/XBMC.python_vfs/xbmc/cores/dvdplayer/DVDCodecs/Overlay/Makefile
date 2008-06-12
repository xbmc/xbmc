INCLUDES=-I. -I../../ -I../../../../ -I../../../../linux -I../../../../../guilib

SRCS=DVDOverlayCodecCC.cpp DVDOverlayCodecFFmpeg.cpp DVDOverlayCodecText.cpp DVDOverlayCodecSSA.cpp

LIB=dvdcodecs_overlay.a

dvdcodecs_overlay.a : libspucc/cc_decoder.o DVDOverlayCodecCC.o DVDOverlayCodecFFmpeg.o DVDOverlayCodecText.o DVDOverlayCodecSSA.o

include ../../../../../Makefile.include
-include Makefile.depend
