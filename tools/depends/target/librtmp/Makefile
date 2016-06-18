include ../../Makefile.include
DEPS= ../../Makefile.include Makefile prefix.patch

# lib name, version
LIBNAME=rtmpdump
VERSION=fa8646daeb19dfd12c181f7d19de708d623704c0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
# configuration settings
LIBDYLIB=$(PLATFORM)/librtmp/librtmp.a
CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

SYS=posix
ifeq ($(OS),ios)
  SYS=darwin
endif
ifeq ($(OS),osx)
  SYS=darwin
endif

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p0 < ../prefix.patch
	cd $(PLATFORM)/librtmp; patch -p0 < ../../libm.patch
	sed -i -e 's|CC=|#CC=|' $(PLATFORM)/librtmp/Makefile
	sed -i -e 's|LD=|#LD=|' $(PLATFORM)/librtmp/Makefile
	sed -i -e 's|AR=|#AR=|' $(PLATFORM)/librtmp/Makefile

$(LIBDYLIB): $(PLATFORM)
	$(MAKE)  PREFIX=$(PREFIX) SYS=$(SYS) CC="$(CC)" LD="$(LD)" AR="$(AR)" XCFLAGS="$(CFLAGS)" XLDFLAGS="$(LDFLAGS)" -C $(PLATFORM)/librtmp

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) SYS=$(SYS) prefix=$(PREFIX) -C $(PLATFORM)/librtmp install
ifeq ($(OS),android)
	rm -f $(PREFIX)/lib/librtmp.la $(PREFIX)/lib/librtmp.so
	mv -f $(PREFIX)/lib/librtmp.so.1 $(PREFIX)/lib/librtmp.so
	$(RPL) -e "librtmp.so.1" "librtmp.so\x00\x00" $(PREFIX)/lib/librtmp.so
	-$(READELF) --dynamic $(PREFIX)/lib/librtmp.so | grep ibrary
endif
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

