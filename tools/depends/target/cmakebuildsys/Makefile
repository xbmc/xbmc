-include ../../Makefile.include

VERSION.TXT := $(CMAKE_SOURCE_DIR)/version.txt
APP_NAME=$(shell awk '/APP_NAME/ {print tolower($$2)}' $(VERSION.TXT))

ifeq ($(Configuration),)
  Configuration = Release
  ifeq ($(DEBUG_BUILD),yes)
    Configuration = Debug
  endif
endif

ifeq ($(OS),darwin_embedded)
  # cmake toolset option buildsystem selects the legacy xcode build system
  # cmake 3.19+ with xcode 12 defaults to the new build system. Stick with the legacy
  # system for now for our building
  CMAKE_BUILD_ARGUMENTS = -G Xcode -T buildsystem=1
else ifeq ($(GEN),Xcode)
  CMAKE_BUILD_ARGUMENTS = -G Xcode -T buildsystem=1
else
  CMAKE_BUILD_ARGUMENTS = -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=$(Configuration)
endif

ifeq ($(BUILD_DIR),)
  BUILD_DIR=$(CMAKE_SOURCE_DIR)/build
endif

all:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR); $(CMAKE) $(CMAKE_BUILD_ARGUMENTS) -DENABLE_INTERNAL_CROSSGUID=ON -DENABLE_INTERNAL_FFMPEG=OFF $(CMAKE_EXTRA_ARGUMENTS) $(CMAKE_SOURCE_DIR)

kodi:
	$(MAKE) -C $(BUILD_DIR)

install:
	$(MAKE) -C $(BUILD_DIR) install

clean:
	$(MAKE) -C $(BUILD_DIR) clean

distclean:
	cd $(BUILD_DIR) && rm -rf *
