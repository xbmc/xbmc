INCLUDES=-I. -I../ -I../../../ -I../../../../ -I../../../linux -I../../../../guilib -I../../../FileSystem/ -I../DVDCodecs/Overlay/
ifneq (1,1)
  INCLUDES+=-I../../ffmpeg
endif

SRCS=	DVDFactorySubtitle.cpp \
	DVDSubtitleLineCollection.cpp \
	DVDSubtitleParserSubrip.cpp \
	DVDSubtitleStream.cpp \
	DVDSubtitleParserMicroDVD.cpp \
	DVDSubtitleParserSami.cpp \
	DVDSubtitlesLibass.cpp \
	DVDSubtitleParserSSA.cpp \
					
LIB=	DVDSubtitles.a

include ../Makefile.include

