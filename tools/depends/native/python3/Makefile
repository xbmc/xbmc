include ../../Makefile.include PYTHON3-VERSION ../../download-files.include
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile PYTHON3-VERSION ../../download-files.include

CONFIGURE=./configure --prefix=$(NATIVEPREFIX) \
                      --disable-shared \
                      --disable-framework \
                      --without-pymalloc \
                      --with-system-ffi

ifeq ($(OS),linux)
  CONFIGURE += --with-system-expat
endif

LIBDYLIB=$(PLATFORM)/python
ifeq ($(NATIVE_OS), osx)
    # Case insensitive FS will add .exe to buildexe regardless of --with-suffix=no
	LIBDYLIB=$(PLATFORM)/python.exe
endif

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	cd $(PLATFORM); $(MAKE)

.installed-$(PLATFORM): $(LIBDYLIB)
# We specifically use -j1 as some threading issues can occur with install directory creation
	cd $(PLATFORM); $(MAKE) install -j1
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
