INCLUDES=-I. -I../../ -I../../../../ -I../../../../linux -I../../../../../guilib

SRCS=DVDOverlayCodecCC.cpp DVDOverlayCodecFFmpeg.cpp

LIB=dvdcodecs_overlay.a

dvdcodecs_overlay.a : libspucc/cc_decoder.o DVDOverlayCodecCC.o DVDOverlayCodecFFmpeg.o

include ../../../../../Makefile.include
-include Makefile.depend
