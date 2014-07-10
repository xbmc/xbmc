include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

# lib name, version
LIBNAME=libvorbis
VERSION=1.3.2
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

export LIBS=-lm
# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) --disable-oggtest --disable-docs --disable-examples

LIBDYLIB=$(PLATFORM)/lib/.libs/$(LIBNAME).a

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	sed -ie "s|SUBDIRS = m4 include vq lib examples test doc|SUBDIRS = m4 include lib|" "$(PLATFORM)/Makefile.in"
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
ifeq ($(OS),android)
	rm -f $(PREFIX)/lib/libvorbis.la $(PREFIX)/lib/libvorbis.so $(PREFIX)/lib/libvorbis.so.0
	mv -f $(PREFIX)/lib/libvorbis.so.0.4.5 $(PREFIX)/lib/libxbvorbis.so
	ln -sf $(PREFIX)/lib/libxbvorbis.so $(PREFIX)/lib/libvorbis.so
	$(RPL) -e "libvorbis.so.0" "libxbvorbis.so" $(PREFIX)/lib/libxbvorbis.so
	-$(READELF) --dynamic $(PREFIX)/lib/libxbvorbis.so | grep ibrary
	#
	rm -f $(PREFIX)/lib/libvorbisfile.la $(PREFIX)/lib/libvorbisfile.so $(PREFIX)/lib/libvorbisfile.so.3
	mv -f $(PREFIX)/lib/libvorbisfile.so.3.3.4 $(PREFIX)/lib/libxbvorbisfile.so
	ln -sf $(PREFIX)/lib/libxbvorbisfile.so $(PREFIX)/lib/libvorbisfile.so
	$(RPL) -e "libvorbisfile.so.3" "libxbvorbisfile.so" $(PREFIX)/lib/libxbvorbisfile.so
	$(RPL) -e "libvorbis.so.0" "libxbvorbis.so" $(PREFIX)/lib/libxbvorbisfile.so
	-$(READELF) --dynamic $(PREFIX)/lib/libxbvorbisfile.so | grep ibrary
	#
endif
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

