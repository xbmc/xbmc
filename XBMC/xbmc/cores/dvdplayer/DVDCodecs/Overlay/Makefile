INCLUDES=-I. -I../../ -I../../../../ -I../../../../../ -I../../../../linux -I../../../../../guilib
ifneq (1,1)
  INCLUDES+=-I../../../ffmpeg
endif

SRCS=	DVDOverlayCodecCC.cpp \
	DVDOverlayCodecFFmpeg.cpp \
	DVDOverlayCodecText.cpp \
	DVDOverlayCodecSSA.cpp \
	libspucc/cc_decoder.c \

LIB=	Overlay.a

include ../../Makefile.include

