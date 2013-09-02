include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

# lib name, version
LIBNAME=gmp
VERSION=5.1.2
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2

# ABI selection
ifeq (,$(findstring x86_64,$(HOST)))
  ABI=32
else
  ABI=64
endif

# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) --disable-shared ABI=$(ABI)

LIBDYLIB=$(PLATFORM)/src/.libs/$(LIBNAME).a

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
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
