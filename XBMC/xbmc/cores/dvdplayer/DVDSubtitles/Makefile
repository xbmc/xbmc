INCLUDES=-I. -I../ -I../../../ -I../../ffmpeg -I../../../linux -I../../../../guilib -I../../../FileSystem/ -I../DVDCodecs/Overlay/

SRCS=	DVDFactorySubtitle.cpp \
	DVDSubtitleLineCollection.cpp \
	DVDSubtitleParserSubrip.cpp \
	DVDSubtitleStream.cpp \
	DVDSubtitleParserMicroDVD.cpp \
	DVDSubtitleParserSami.cpp \
	DVDSubtitlesLibass.cpp \
	DVDSubtitleParserSSA.cpp \

LIB=	DVDSubtitles.a

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

