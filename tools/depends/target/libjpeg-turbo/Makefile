include ../../Makefile.include
DEPS= ../../Makefile.include Makefile 01-disable-executables.patch 02-fix-gas-preprocessor-with-ccache.patch

# lib name, version
LIBNAME=libjpeg-turbo
VERSION=2.0.4
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(LIBNAME)-$(VERSION).tar.gz

LIBDYLIB=$(PLATFORM)/build/libjpeg.a

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-disable-executables.patch
	cd $(PLATFORM); patch -p1 -i ../02-fix-gas-preprocessor-with-ccache.patch
	cd $(PLATFORM); $(CMAKE) -B build -DCMAKE_ASM_NASM_COMPILER:FILEPATH=$(NATIVEPREFIX)/bin/yasm -DENABLE_SHARED:BOOL=OFF -DWITH_JPEG8:BOOL=ON

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
