INCLUDES =-I../../xbmc/utils -I../../guilib -I../../xbmc/linux -I../../xbmc/lib/libsquish
DEFINES = 
LIBS = ../../xbmc/lib/libsquish/libsquish.a ../../guilib/XBTF.o -L/opt/local/lib -lSDL_image -lSDL
OBJS = \
	SDL_anigif.o \
	XBTFWriter.o \
	XBMCTex.o \

TARGET = XBMCTexXBT
CLEAN_FILES=$(TARGET)

all: $(TARGET)

include ../../Makefile.include

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) $(LIBS) -o $(TARGET)
