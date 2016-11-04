-include ../../Makefile.include

VERSION.TXT := $(CMAKE_SOURCE_DIR)/version.txt
APP_NAME=$(shell awk '/APP_NAME/ {print tolower($$2)}' $(VERSION.TXT))

ifeq ($(Configuration),)
  Configuration = Release
  ifeq ($(DEBUG_BUILD),yes)
    Configuration = Debug
  endif
endif

all:
	mkdir -p $(CMAKE_SOURCE_DIR)/build
	cd $(CMAKE_SOURCE_DIR)/build; $(CMAKE) -DCMAKE_BUILD_TYPE=$(Configuration) -DENABLE_INTERNAL_CROSSGUID=OFF -DENABLE_INTERNAL_FFMPEG=OFF $(CMAKE_SOURCE_DIR)

clean:
	cd $(CMAKE_SOURCE_DIR)/build; $(MAKE) clean

distclean:
	cd $(CMAKE_SOURCE_DIR)/build; $(MAKE) clean
