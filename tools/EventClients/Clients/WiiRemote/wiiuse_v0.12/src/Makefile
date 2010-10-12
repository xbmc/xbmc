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
LDFLAGS = -shared -lm -lbluetooth

#
# Target binaries (always created as BIN)
#
BIN = ./$(OBJ_DIR)/libwiiuse.so

#
# Inclusion paths.
#
INCLUDES = -I.

#
# Generate a list of object files
#
OBJS = \
	$(OBJ_DIR)/classic.o \
	$(OBJ_DIR)/dynamics.o \
	$(OBJ_DIR)/events.o \
	$(OBJ_DIR)/io.o \
	$(OBJ_DIR)/io_nix.o \
	$(OBJ_DIR)/ir.o \
	$(OBJ_DIR)/nunchuk.o \
	$(OBJ_DIR)/guitar_hero_3.o \
	$(OBJ_DIR)/wiiuse.o

###############################
#
# Build targets.
#
###############################

all: $(BIN)

clean:
	@-rm $(OBJS) 2> /dev/null

distclean:
	@-rm -r debug-* release-* 2> /dev/null

install:
	@if [ -e $(BIN) ]; then \
		cp -v $(BIN) /usr/lib ; \
	fi
	@cp -v wiiuse.h /usr/include

$(BIN): mkdir $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $(BIN)

$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

mkdir:
	@if [ ! -d $(OBJ_DIR) ]; then \
		mkdir $(OBJ_DIR); \
	fi

