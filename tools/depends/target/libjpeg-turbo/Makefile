include ../../Makefile.include LIBJPEG-TURBO-VERSION ../../download-files.include
DEPS = ../../Makefile.include LIBJPEG-TURBO-VERSION Makefile ../../download-files.include

LIBDYLIB=$(PLATFORM)/build/libjpeg.a

CMAKE_OPTIONS=-DENABLE_SHARED:BOOL=OFF -DWITH_JPEG8:BOOL=ON -DWITH_TURBOJPEG=OFF

ifeq ($(OS),wasm)
  CMAKE_OPTIONS+= -DWITH_SIMD=OFF
else
  CMAKE_OPTIONS+= -DCMAKE_ASM_NASM_COMPILER:FILEPATH=$(NATIVEPREFIX)/bin/nasm
endif

all: .installed-$(PLATFORM)

.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CMAKE) -B build $(CMAKE_OPTIONS)
	touch $@

$(LIBDYLIB): .configured-$(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)
