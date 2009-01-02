INCLUDES=-I. -I.. -I../linux -I../../guilib

SRCS=karaokelyricscdg.cpp \
	karaokelyrics.cpp	\
	karaokelyricsfactory.cpp \
	karaokelyricsmanager.cpp \
	karaokelyricstext.cpp \
	karaokelyricstextkar.cpp \
	karaokelyricstextlrc.cpp \
	karaokesongselector.cpp \
	Cdg.cpp \

LIB=karaoke.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
