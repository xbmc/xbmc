include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

# lib name, version
LIBNAME=libmicrohttpd
VERSION=svn-35533
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) --disable-shared \
            --disable-doc --disable-examples --disable-curl \
            --disable-spdy

# internal memmem sym collides with internal gnutils memmem
# when built on 10.6.8, strip it.
ifeq ($(OS), osx)
STRIP_MEMMEM=-$(AR) d $(PLATFORM)/src/daemon/.libs/libmicrohttpd.a memmem.o
endif


LIBDYLIB=$(PLATFORM)/src/daemon/.libs/$(LIBNAME).a

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/src/microhttpd
	$(MAKE) -C $(PLATFORM)/src/include install
	$(STRIP_MEMMEM)

.installed-$(PLATFORM): $(LIBDYLIB)
	rm -f $(PREFIX)/lib/libmicrohttpd.so $(PREFIX)/lib/libmicrohttpd.so.5 $(PREFIX)/lib/libmicrohttpd.so.5.2.1
	$(MAKE) -C $(PLATFORM)/src/microhttpd install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

