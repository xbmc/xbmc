SRCS=BasicMemoryStream.cpp \
     DeltaPairMemoryStream.cpp \
     LinearMemoryStream.cpp \
     Savestate.cpp \
     SavestateDatabase.cpp \
     SavestateReader.cpp \
     SavestateTranslator.cpp \
     SavestateUtils.cpp \
     SavestateWriter.cpp \

LIB=gamesavestates.a

include ../../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
