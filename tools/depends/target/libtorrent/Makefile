include ../../Makefile.include
include LIBTORRENT-VERSION
include ../../download-files.include

DEPS := \
  ../../download-files.include \
  ../../Makefile.include \
  0001-Use-system-libdatachannel-and-try_signal.patch \
  0002-Disable-asserts.patch \
  LIBTORRENT-VERSION \
  Makefile \

CMAKE_OPTIONS := \
  -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_CXX_STANDARD=17 \
  -Ddeprecated-functions=OFF \
  -Dwebtorrent=ON \
  $(CMAKE_OPTIONS)

# Build directory
BUILDDIR := $(PLATFORM)/build

# Generated library
LIBDYLIB := $(BUILDDIR)/$(BYPRODUCT)

.PHONY: .installed-native

all: .installed-$(PLATFORM)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
ifeq ($(PREFIX),)
	@echo
	@echo "ERROR: please set PREFIX to the kodi install path e.g. make PREFIX=/usr/local"
	@exit 1
endif
	rm -rf $(PLATFORM); mkdir -p $(BUILDDIR)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../0001-Use-system-libdatachannel-and-try_signal.patch
	cd $(PLATFORM); patch -p1 -i ../0002-Disable-asserts.patch
	cd $(BUILDDIR); $(CMAKE) $(CMAKE_OPTIONS) ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(BUILDDIR)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(BUILDDIR) install
	touch $@

clean:
	$(MAKE) -C $(BUILDDIR) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
