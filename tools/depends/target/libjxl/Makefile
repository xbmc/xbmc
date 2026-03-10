include ../../Makefile.include LIBJXL-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile LIBJXL-VERSION ../../download-files.include


CMAKE_OPTIONS= -DBUILD_TESTING=OFF \
               -DBUILD_SHARED_LIBS=OFF \
               -DJPEGXL_ENABLE_TOOLS=OFF \
               -DJPEGXL_ENABLE_FUZZERS=OFF \
               -DJPEGXL_ENABLE_JPEGLI=OFF \
               -DJPEGXL_ENABLE_JPEGLI_LIBJPEG=OFF \
               -DJPEGXL_ENABLE_DOXYGEN=OFF \
               -DJPEGXL_ENABLE_MANPAGES=OFF \
               -DJPEGXL_ENABLE_BENCHMARK=OFF \
               -DJPEGXL_ENABLE_EXAMPLES=OFF \
               -DJPEGXL_BUNDLE_LIBPNG=OFF \
               -DJPEGXL_ENABLE_JNI=OFF \
               -DJPEGXL_ENABLE_SJPEG=OFF \
               -DJPEGXL_ENABLE_OPENEXR=OFF \
               -DJPEGXL_ENABLE_SKCMS=OFF \
               -DJPEGXL_ENABLE_TCMALLOC=OFF \
               -DJPEGXL_FORCE_SYSTEM_BROTLI=ON \
               -DJPEGXL_FORCE_SYSTEM_LCMS2=ON \
               -DJPEGXL_FORCE_SYSTEM_HWY=ON

LIBDYLIB=$(PLATFORM)/build/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM)/build; $(CMAKE) $(CMAKE_OPTIONS) ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
