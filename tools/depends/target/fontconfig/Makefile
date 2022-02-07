include ../../Makefile.include
DEPS = ../../Makefile.include 01-disable-test.patch lconv.patch fix-aarch64_atomics.patch Makefile ../../download-files.include

# lib name, version
LIBNAME=fontconfig
VERSION=2.13.1
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2
SHA512=f97f2a9db294fd72d416a7d76dd7db5934ade2cf76903764b09e7decc33e0e2eed1a1d35c5f1c7fd9ea39e2c7653b9e65365f0c6205e047e95e38ba5000dd100
include ../../download-files.include

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


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
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
