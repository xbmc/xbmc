include ../Makefile.include
DEPS= ../Makefile.include Makefile

# lib name, version
LIBNAME=libid3tag
VERSION=0.15.1b
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings

CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) --build=$(BUILD) --host=$(HOST) \

LIBDYLIB=$(PLATFORM)/.libs/$(LIBNAME).a

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -j 1 -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	rm -f $(PREFIX)/lib/libid3tag.la $(PREFIX)/lib/libid3tag.so $(PREFIX)/lib/libid3tag.so.0
	mv -f $(PREFIX)/lib/libid3tag.so.0.3.0 $(PREFIX)/lib/libid3tag.so
	$(RPL) -e "libid3tag.so.0" "libid3tag.so\x00\x00" $(PREFIX)/lib/libid3tag.so
	-$(READELF) --dynamic $(PREFIX)/lib/libid3tag.so | grep ibrary
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
