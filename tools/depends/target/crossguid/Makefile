-include ../../Makefile.include
DEPS = Makefile

# lib name, version
LIBNAME=crossguid
VERSION=8f399e8bd4
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz


ifeq ($(CROSS_COMPILING), yes)
  DEPS += ../../Makefile.include
  CXXFLAGS += -std=c++11
else
  CXXFLAGS += -std=c++11
  ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

  ifeq ($(PLATFORM),)
    PLATFORM = native
    TARBALLS_LOCATION = $(ROOT_DIR)
    BASE_URL := http://mirrors.kodi.tv/build-deps/sources
    RETRIEVE_TOOL := curl -Ls --create-dirs -f -O
    ARCHIVE_TOOL := tar --strip-components=1 -xf
  endif
endif


# define specifying the native GUID implementation to use
GUID_PLATFORM_DEFINE=GUID_LIBUUID
ifeq ($(OS),osx)
	GUID_PLATFORM_DEFINE=GUID_CFUUID
endif
ifeq ($(OS),darwin_embedded)
	GUID_PLATFORM_DEFINE=GUID_CFUUID
endif

LIBDYLIB=$(PLATFORM)/lib$(LIBNAME).a

.PHONY: .installed-native

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
ifeq ($(PREFIX),)
	@echo
	@echo "ERROR: please set PREFIX to the kodi install path e.g. make PREFIX=/usr/local"
	@exit 1
endif
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

$(LIBDYLIB): $(PLATFORM)
	cd $(PLATFORM); $(CXX) -c guid.cpp -o guid.o $(CXXFLAGS) -D$(GUID_PLATFORM_DEFINE)
	cd $(PLATFORM); $(AR) rvs lib$(LIBNAME).a guid.o

.installed-$(PLATFORM): $(LIBDYLIB)
	@[ -d $(PREFIX)/include ] || mkdir -p $(PREFIX)/include
	@[ -d $(PREFIX)/lib ] || mkdir $(PREFIX)/lib
	cp $(PLATFORM)/guid.h $(PREFIX)/include/
	cp $(LIBDYLIB) $(PREFIX)/lib/
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

