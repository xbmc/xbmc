include ../../Makefile.include
DEPS = ../../Makefile.include Makefile ../../download-files.include

# We use uuid from e2fsprogs for legacy. util-linux works fine on android now
# lib name, version
LIBNAME=libuuid
VERSION=1.46.5
SOURCE=e2fsprogs-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz
SHA512=53282e1c524f62a95012b1aceef296d494f5238c82c9b08b29fbe6a803dbf7ccfdcd9124eb2f11fe2ff9de26387c78751a92c3217ca414f6db6242098a72d3fa
include ../../download-files.include

LIBDYLIB=$(PLATFORM)/lib/$(LIBNAME).a

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	# e2fsprog libuuid often fails because it doesnt create include/uuid location before
	# installing. lets just create it and move on.
	mkdir -p $(PREFIX)/include/uuid
	cd $(PLATFORM); ./configure --prefix=$(PREFIX) --disable-fsck --enable-libuuid

$(LIBDYLIB): $(PLATFORM)
  # -j1 is required due to concurrency failures with dirpath.h generation
	cd $(PLATFORM)/lib/uuid ; $(MAKE) -j1

.installed-$(PLATFORM): $(LIBDYLIB)
  # -j1 is required due to concurrency failures with dirpath.h generation
	cd $(PLATFORM)/lib/uuid ; $(MAKE) -j1 install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
