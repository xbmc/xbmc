-include ../../Makefile.include

VERSION.TXT := $(CORE_SOURCE_DIR)/version.txt
APP_NAME=$(shell awk '/APP_NAME/ {print tolower($$2)}' $(VERSION.TXT))

all:
	mkdir -p $(CORE_SOURCE_DIR)/build
	cd $(CORE_SOURCE_DIR)/build; $(CMAKE) -DCMAKE_BUILD_TYPE=$(Configuration) $(CORE_SOURCE_DIR)/project/cmake

clean:
	cd $(CORE_SOURCE_DIR)/build; $(MAKE) clean

distclean:
	cd $(CORE_SOURCE_DIR)/build; $(MAKE) clean
