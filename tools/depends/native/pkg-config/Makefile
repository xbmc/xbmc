include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile ../../download-files.include

# lib name, version
# lib name, version
LIBNAME=pkg-config
VERSION=0.29.2
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=4861ec6428fead416f5cbbbb0bbad10b9152967e481d4b0ff2eb396a9f297f552984c9bb72f6864a37dcd8fca1d9ccceda3ef18d8f121938dbe4fdf2b870fe75
include ../../download-files.include

PC_PATH=$(PREFIX)/lib/pkgconfig:$(PREFIX)/share/pkgconfig
ifeq ($(CROSS_COMPILING),no)
PC_PATH:=$(PC_PATH):/usr/lib/pkgconfig:/usr/lib/$(HOST)/pkgconfig:/usr/share/pkgconfig
endif

# LLVM 15 has raised this to error by default. drop back to warning
CFLAGS=$(NATIVE_CFLAGS) -Wno-error=int-conversion
export CFLAGS

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --enable-indirect-deps --with-pc-path=$(PC_PATH) --with-internal-glib
LIBDYLIB=$(PLATFORM)/pkg-config

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	-rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)
	cd $(PLATFORM); sed -ie "s|LN = ln|LN = ln -f|" Makefile

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
