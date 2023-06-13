include ../../Makefile.include FLATBUFFERS-VERSION ../../download-files.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS =../../Makefile.include Makefile FLATBUFFERS-VERSION ../../download-files.include

APP=$(PLATFORM)/build-cmake/flatc

# BUILD Notes
# When we go c++17, flatbuffers has FLATBUFFERS_BUILD_CPP17 to enable
# If we bump cmake min req to 3.16, enable FLATBUFFERS_ENABLE_PCH

CMAKE_OPTIONS := -DCMAKE_BUILD_TYPE=Release \
                 -DFLATBUFFERS_CODE_COVERAGE=OFF \
                 -DFLATBUFFERS_BUILD_TESTS=OFF \
                 -DFLATBUFFERS_INSTALL=ON \
                 -DFLATBUFFERS_BUILD_FLATLIB=OFF \
                 -DFLATBUFFERS_BUILD_FLATC=ON \
                 -DFLATBUFFERS_BUILD_FLATHASH=OFF \
                 -DFLATBUFFERS_BUILD_GRPCTEST=OFF \
                 -DFLATBUFFERS_BUILD_SHAREDLIB=OFF \
                 $(CMAKE_OPTIONS)
BUILDDIR = $(PLATFORM)/build-cmake # 'build' conflicts with file BUILD on case-insensitive FS

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR); $(CMAKE_FOR_BUILD) $(CMAKE_OPTIONS) ..

$(APP): $(PLATFORM)
	$(MAKE) -C $(BUILDDIR)

.installed-$(PLATFORM): $(APP)
	$(MAKE) -C $(BUILDDIR) install
	touch $@

clean:
	$(MAKE) -C $(BUILDDIR) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
