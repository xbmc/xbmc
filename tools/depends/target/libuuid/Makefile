include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

# We use uuid from e2fsprogs for legacy. util-linux works fine on android now
# lib name, version
LIBNAME=libuuid
VERSION=1.46.5
SOURCE=e2fsprogs-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz

LIBDYLIB=$(PLATFORM)/lib/$(LIBNAME).a

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
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
