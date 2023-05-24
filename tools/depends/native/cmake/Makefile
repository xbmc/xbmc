include ../../Makefile.include
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile ../../download-files.include

APPNAME=cmake
VERSION=3.26.4
SOURCE=$(APPNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=fe817c8d5e247db3f0a9a58ee37c466a47220100d9e90711cd5d06c223cef87e41d1a756e75d1537e5f8cd010dcb8971cbeab4684b1ac12bcecf84bf7b720167
include ../../download-files.include

# configuration settings

SETENV=CC="$(CC_FOR_BUILD)" CXX="$(CXX_FOR_BUILD)" LD=$(LD_FOR_BUILD) CFLAGS="$(NATIVE_CFLAGS)" \
	CXXFLAGS="$(NATIVE_CXXFLAGS)" LDFLAGS="$(NATIVE_LDFLAGS)"

ifeq ($(NATIVE_OS), osx)
  SETENV+=SDKROOT=$(shell xcrun --show-sdk-path)
endif

CONFIGURE=./bootstrap --prefix=$(NATIVEPREFIX) --system-curl
ifeq ($(USE_CCACHE), yes)
	CONFIGURE+=--enable-ccache
endif

APP=$(PLATFORM)/bin/$(APPNAME)

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
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

