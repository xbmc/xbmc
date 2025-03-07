include ../../Makefile.include PCRE2-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile PCRE2-VERSION ../../download-files.include \
                                001-all-enable_docs_pc.patch

# configuration settings
CMAKE_OPTIONS=-DBUILD_STATIC_LIBS=ON \
              -DPCRE2_BUILD_PCRE2_8=ON \
              -DPCRE2_BUILD_PCRE2_16=OFF \
              -DPCRE2_BUILD_PCRE2_32=OFF \
              -DPCRE2_SUPPORT_JIT=ON \
              -DPCRE2_SUPPORT_UNICODE=ON \
              -DPCRE2_BUILD_PCRE2GREP=OFF \
              -DPCRE2_BUILD_TESTS=OFF \
              -DENABLE_DOCS=OFF

ifeq ($(OS),darwin_embedded)
  # build default is off, look to see if we want to enable this for non darwin_embedded platforms
  CMAKE_OPTIONS+= -DPCRE2_SUPPORT_JIT=OFF
endif

LIBDYLIB=$(PLATFORM)/build/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); mkdir -p build
	cd $(PLATFORM); patch -p1 -i ../001-all-enable_docs_pc.patch
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
