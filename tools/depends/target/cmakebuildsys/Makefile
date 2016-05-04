-include ../../Makefile.include

VERSION.TXT := $(XBMCROOT)/version.txt
APP_NAME=$(shell awk '/APP_NAME/ {print tolower($$2)}' $(VERSION.TXT))

all:
	mkdir -p $(XBMCROOT)/build
	cd $(XBMCROOT)/build; $(CMAKE) $(XBMCROOT)/project/cmake

clean:
	cd $(XBMCROOT)/build; $(MAKE) clean

distclean:
	cd $(XBMCROOT)/build; $(MAKE) clean
