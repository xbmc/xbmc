
DEFINES = -DUSE_SDL -D_LINUX
INCLUDES = -I. -I../../guilib -I../../xbmc -I../../xbmc/linux/
LIBS = -lSDL -lSDL_image -llzo
LDFLAGS =
OBJS = \
	AnimatedGif.o \
	Bundler.o \
	SurfaceSDL.o \
	XBMCTex.o \
	xbox.o \
	xwinapi.o \
	XFileUtils.o \
	XHandle.o \
	XTimeUtils.o \
	sha.o

TARGET = XBMCTex

all: $(TARGET)
	
$(TARGET): $(OBJS)
	g++ -O3 $(OBJS) $(LDFLAGS) $(LIBS) -o $(TARGET)

.cpp.o:
	g++ -O3 $(DEFINES) $(INCLUDES) -c -o $@ $<

.c.o:
	gcc -O3 $(DEFINES) $(INCLUDES) -c -o $@ $<

clean:
	rm -f $(TARGET) *.o
