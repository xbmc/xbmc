include ../../Makefile.include LIBNFS-VERSION ../../download-files.include
DEPS= ../../Makefile.include Makefile LIBNFS-VERSION ../../download-files.include \
                  01-MSUWP-compat.patch \
                  04-all-sync-fail-pending-requests.patch \
                  05-all-udp-sendto-address-length.patch \
                  07-all-nfs4-readlink-length.patch

ifeq ($(OS),wasm)
DEPS+= 02-wasm-zdr-callback-signature.patch \
       03-wasm-monotonic-clock.patch
endif

# configuration settings
CMAKE_OPTIONS=-DBUILD_SHARED_LIBS=OFF \
              -DENABLE_TESTS=OFF \
              -DENABLE_DOCUMENTATION=OFF \
              -DENABLE_UTILS=OFF \
              -DENABLE_EXAMPLES=OFF

LIBDYLIB=$(PLATFORM)/build/lib/$(BYPRODUCT)

all: .installed-$(PLATFORM)

.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	-rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); mkdir -p build
	cd $(PLATFORM); patch -p1 -i ../01-MSUWP-compat.patch
ifeq ($(OS),wasm)
	cd $(PLATFORM); patch -p1 -i ../02-wasm-zdr-callback-signature.patch
	cd $(PLATFORM); patch -p1 -i ../03-wasm-monotonic-clock.patch
endif
	cd $(PLATFORM); patch -p1 -i ../04-all-sync-fail-pending-requests.patch
	cd $(PLATFORM); patch -p1 -i ../05-all-udp-sendto-address-length.patch
	cd $(PLATFORM); patch -p1 -i ../07-all-nfs4-readlink-length.patch
	cd $(PLATFORM)/build; $(CMAKE) $(CMAKE_OPTIONS) ..
	touch $@

$(LIBDYLIB): .configured-$(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean
	rm -f .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)
