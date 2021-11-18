include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS= ../../Makefile.include Makefile 01-disable-executables.patch

# lib name, version
LIBNAME=libjpeg-turbo
VERSION=2.1.1
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(LIBNAME)-$(VERSION).tar.gz

CMAKE_OPTIONS := -DCMAKE_TOOLCHAIN_FILE= \
                 -DCMAKE_ASM_NASM_COMPILER:FILEPATH=$(NATIVEPREFIX)/bin/nasm \
                 -DENABLE_SHARED:BOOL=OFF \
                 -DWITH_JPEG8:BOOL=ON \
                 -DCMAKE_C_COMPILER="$(CC_BINARY_FOR_BUILD)" \
                 -DCMAKE_C_FLAGS="$(NATIVE_CFLAGS)" \
                 -DCMAKE_EXE_LINKER_FLAGS="$(NATIVE_LDFLAGS)" \
                 -DCMAKE_INSTALL_LIBDIR:STRING=lib

LIBDYLIB=$(PLATFORM)/build/libjpeg.a

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-disable-executables.patch
	cd $(PLATFORM); $(CMAKE) -B build $(CMAKE_OPTIONS)

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
