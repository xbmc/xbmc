CXX ?= g++
CFLAGS = -Wall -pipe -fPIC -funroll-loops
OBJ_DIR = release-$(shell $(CC) -v 2>&1|grep ^Target:|cut -d' ' -f2)
OBJS = wiiuse_v0.12/src/$(OBJ_DIR)/libwiiuse.so
BIN = WiiUse_WiiRemote
VERSION = v0.12

all:
	@$(MAKE) -C wiiuse_$(VERSION)/src $@
	$(CXX) $(CFLAGS) -I./wiiuse_$(VERSION)/src WiiUse_WiiRemote.cpp $(OBJS) -o $(BIN)
wiiuse:
	@$(MAKE) -C wiiuse_$(VERSION)/src
clean:
	rm $(OBJS) $(BIN)
