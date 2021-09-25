include ../../Makefile.include
DEPS= ../../Makefile.include 01-disable-test.patch lconv.patch fix-aarch64_atomics.patch Makefile

# lib name, version
LIBNAME=fontconfig
VERSION=2.13.1
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) \
  --disable-libxml2 --disable-docs --with-arch=$(PLATFORM) --disable-shared

ifeq ($(OS),android)
  # Freetype with Harfbuzz support requires stdc++ lib linking
  # Fontconfig uses clang and not clang++, therefore we need the explicit stdc++ link
  export LDFLAGS+= -lstdc++
endif

LIBDYLIB=$(PLATFORM)/src/.libs/lib$(LIBNAME).a

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-disable-test.patch
	cd $(PLATFORM); patch -p1 -i ../lconv.patch
	cd $(PLATFORM); patch -p1 -i ../fix-aarch64_atomics.patch
	cd $(PLATFORM); $(AUTORECONF) -vif
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
