include ../../Makefile.include PYTHON3-VERSION
PLATFORM=$(NATIVEPLATFORM)
DEPS= ../../Makefile.include Makefile PYTHON3-VERSION 01-distutil-flags.patch

CWD=$(shell pwd)
HOSTPYTHONDIR=$(CWD)/$(PLATFORM)/hostpython

CONFIGURE=./configure --prefix=$(NATIVEPREFIX) --disable-shared --disable-framework --without-pymalloc --with-system-ffi
ifeq ($(OS),linux)
  CONFIGURE += --with-system-expat
endif

LIBDYLIB=$(PLATFORM)/python
ifeq ($(NATIVE_OS), osx)
    # Case insensitive FS will add .exe to buildexe regardless of --with-suffix=no
	LIBDYLIB=$(PLATFORM)/python.exe
endif

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	cd $(PLATFORM); $(MAKE)

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM); patch -p1 -i ../01-distutil-flags.patch
	cd $(PLATFORM); $(MAKE) install
	touch $(LIBDYLIB)
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
