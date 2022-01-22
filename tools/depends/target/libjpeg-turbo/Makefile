include ../../Makefile.include
DEPS = ../../Makefile.include Makefile 01-disable-executables.patch ../../download-files.include

# lib name, version
LIBNAME=libjpeg-turbo
VERSION=2.1.1
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(LIBNAME)-$(VERSION).tar.gz
SHA512=9cf61b4677b38e254063248a3fdbe138634d23a35ae8f2ee2661bdb3248b1d0500bd43201b8d9fe66c779d578074fd9ed19bc097ba22e10143530c2a19738673
include ../../download-files.include

LIBDYLIB=$(PLATFORM)/build/libjpeg.a

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-disable-executables.patch
	cd $(PLATFORM); $(CMAKE) -B build -DCMAKE_ASM_NASM_COMPILER:FILEPATH=$(NATIVEPREFIX)/bin/nasm -DENABLE_SHARED:BOOL=OFF -DWITH_JPEG8:BOOL=ON

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
