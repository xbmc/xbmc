include ../../Makefile.include
DEPS= ../../Makefile.include Makefile tvos-bitcode-fix.patch jit_aarch64.patch ios-clear_cache.patch

# lib name, version
LIBNAME=pcre
VERSION=8.45
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2

# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; ./configure --prefix=$(PREFIX) \
	--disable-shared --disable-stack-for-recursion \
	--enable-pcre8 --disable-pcre16 --disable-pcre32 \
	--enable-jit --enable-utf --enable-unicode-properties \
	--enable-newline-is-anycrlf


LIBDYLIB=$(PLATFORM)/.libs/lib$(LIBNAME).a

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../tvos-bitcode-fix.patch
	cd $(PLATFORM); patch -p1 -i ../jit_aarch64.patch
	cd $(PLATFORM); patch -p1 -i ../ios-clear_cache.patch
	cd $(PLATFORM); $(CONFIGURE)

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
