include ../Makefile.include
DEPS= ../Makefile.include Makefile prefix.patch

# lib name, version
LIBNAME=rtmpdump
VERSION=2.3
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tgz

# configuration settings

LIBDYLIB=$(PLATFORM)/librtmp/librtmp.a

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p0 < ../prefix.patch

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/librtmp CROSS_COMPILE=$(CROSSTOOLS) PREFIX=$(PREFIX)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/librtmp install PREFIX=$(PREFIX)
	rm -f $(PREFIX)/lib/librtmp.la $(PREFIX)/lib/librtmp.so
	mv -f $(PREFIX)/lib/librtmp.so.0 $(PREFIX)/lib/librtmp.so
	$(RPL) -e "librtmp.so.0" "librtmp.so\x00\x00" $(PREFIX)/lib/librtmp.so
	-$(READELF) --dynamic $(PREFIX)/lib/librtmp.so | grep ibrary
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

