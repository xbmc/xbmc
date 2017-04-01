include ../../Makefile.include
PLATFORM=$(NATIVEPLATFORM)
DEPS= ../../Makefile.include Makefile

APPNAME=cmake
VERSION=3.6.2
SOURCE=$(APPNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings

SETENV=CC="$(CC_FOR_BUILD)" CXX="$(CXX_FOR_BUILD)" LD=$(LD_FOR_BUILD) CFLAGS=$(NATIVE_CFLAGS) \
	CXXFLAGS=$(NATIVE_CXXFLAGS) LDFLAGS=$(NATIVE_LDFLAGS)

ifeq ($(NATIVE_OS), osx)
  SETENV+=SDKROOT=$(shell xcrun --show-sdk-path)
endif

CONFIGURE=./bootstrap --prefix=$(NATIVEPREFIX) --system-curl
ifeq ($(USE_CCACHE), yes)
	CONFIGURE+=--enable-ccache
endif

ifeq ($(OS),osx)
  SETENV += SDKROOT=$(SDKROOT)
endif

APP=$(PLATFORM)/bin/$(APPNAME)

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	-rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(SETENV) $(CONFIGURE)

$(APP): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)
	touch $@

.installed-$(PLATFORM): $(APP)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

