INCLUDES=-I. -I../../ -I../../../../ -I../../../ffmpeg -I../../../../linux -I../../../../../guilib

SRCS=	DVDOverlayCodecCC.cpp \
	DVDOverlayCodecFFmpeg.cpp \
	DVDOverlayCodecText.cpp \
	DVDOverlayCodecSSA.cpp \
	libspucc/cc_decoder.c \

LIB=	Overlay.a

include ../../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

