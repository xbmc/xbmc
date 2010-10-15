#
#	wiiuse Makefile
#

#
# Change this to your GCC version.
#
CC ?= gcc

####################################################
#
# You should not need to edit below this line.
#
####################################################

#
# Universal cflags
#
CFLAGS = -Wall -pipe -fPIC -funroll-loops

ifeq ($(debug),1)
        OBJ_PREFIX = debug
        CFLAGS += -g -pg -DWITH_WIIUSE_DEBUG
else
        OBJ_PREFIX = release
        CFLAGS += -O2
endif

OBJ_DIR = $(OBJ_PREFIX)-$(shell $(CC) -v 2>&1|grep ^Target:|cut -d' ' -f2)

#
# Linking flags
#
LDFLAGS = -L../src/$(OBJ_DIR) -lm -lGL -lGLU -lglut -lSDL -lbluetooth -lwiiuse

#
# Target binaries (always created as BIN)
#
BIN = ./$(OBJ_DIR)/wiiuse-sdl

#
# Inclusion paths.
#
INCLUDES = -I../src/ -I/usr/include/SDL

#
# Generate a list of object files
#
OBJS = $(OBJ_DIR)/sdl.o

###############################
#
# Build targets.
#
###############################

all: $(BIN)

clean:
	@-rm $(OBJS) 2> /dev/null

distclean: clean
	@-rm -r debug-* release-* 2> /dev/null

install:
	@if [ -e $(BIN) ]; then \
		cp -v $(BIN) /usr/bin ;\
	fi

$(BIN): mkdir $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $(BIN)

$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

mkdir:
	@if [ ! -d $(OBJ_DIR) ]; then \
		mkdir $(OBJ_DIR); \
	fi

run: all
	LD_LIBRARY_PATH=`pwd`/../src/$(OBJ_DIR):$(LD_LIBRARY_PATH) $(BIN)

