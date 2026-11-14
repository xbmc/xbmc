include ../../Makefile.include LIBUUID-VERSION ../../download-files.include
DEPS = ../../Makefile.include LIBUUID-VERSION Makefile ../../download-files.include

# We use uuid from e2fsprogs for legacy. util-linux works fine on android now

LIBDYLIB=$(PLATFORM)/lib/$(LIBNAME).a

all: .installed-$(PLATFORM)

.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	# e2fsprog libuuid often fails because it doesnt create include/uuid location before
	# installing. lets just create it and move on.
	mkdir -p $(PREFIX)/include/uuid
	cd $(PLATFORM); ./configure --prefix=$(PREFIX) --disable-fsck --enable-libuuid
	touch $@

$(LIBDYLIB): .configured-$(PLATFORM)
  # -j1 is required due to concurrency failures with dirpath.h generation
	cd $(PLATFORM)/lib/uuid ; $(MAKE) -j1

.installed-$(PLATFORM): $(LIBDYLIB)
  # -j1 is required due to concurrency failures with dirpath.h generation
	cd $(PLATFORM)/lib/uuid ; $(MAKE) -j1 install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)
