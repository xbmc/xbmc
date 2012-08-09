include ../Makefile.include
DEPS= ../Makefile.include Makefile

# lib name, version
LIBNAME=libvorbis
VERSION=1.2.3
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

export LIBS=-lm
# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) --host=$(HOST) --disable-oggtest --disable-docs --disable-examples

LIBDYLIB=$(PLATFORM)/lib/.libs/$(LIBNAME).a

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	sed -ie "s|SUBDIRS = m4 include vq lib examples test doc|SUBDIRS = m4 include lib|" "$(PLATFORM)/Makefile.in"
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	rm -f $(PREFIX)/lib/libvorbis.la $(PREFIX)/lib/libvorbis.so $(PREFIX)/lib/libvorbis.so.0
	mv -f $(PREFIX)/lib/libvorbis.so.0.4.3 $(PREFIX)/lib/libxbvorbis.so
	ln -sf $(PREFIX)/lib/libxbvorbis.so $(PREFIX)/lib/libvorbis.so
	$(RPL) -e "libvorbis.so.0" "libxbvorbis.so" $(PREFIX)/lib/libxbvorbis.so
	-$(READELF) --dynamic $(PREFIX)/lib/libxbvorbis.so | grep ibrary
	#
	rm -f $(PREFIX)/lib/libvorbisfile.la $(PREFIX)/lib/libvorbisfile.so $(PREFIX)/lib/libvorbisfile.so.3
	mv -f $(PREFIX)/lib/libvorbisfile.so.3.3.2 $(PREFIX)/lib/libxbvorbisfile.so
	ln -sf $(PREFIX)/lib/libxbvorbisfile.so $(PREFIX)/lib/libvorbisfile.so
	$(RPL) -e "libvorbisfile.so.3" "libxbvorbisfile.so" $(PREFIX)/lib/libxbvorbisfile.so
	$(RPL) -e "libvorbis.so.0" "libxbvorbis.so" $(PREFIX)/lib/libxbvorbisfile.so
	-$(READELF) --dynamic $(PREFIX)/lib/libxbvorbisfile.so | grep ibrary
	#
	rm -f $(PREFIX)/lib/libvorbisenc.la $(PREFIX)/lib/libvorbisenc.so $(PREFIX)/lib/libvorbisenc.so.2
	mv -f $(PREFIX)/lib/libvorbisenc.so.2.0.6 $(PREFIX)/lib/libxbvorbisenc.so
	ln -sf $(PREFIX)/lib/libxbvorbisenc.so $(PREFIX)/lib/libvorbisenc.so
	$(RPL) -e "libvorbisenc.so.2" "libxbvorbisenc.so" $(PREFIX)/lib/libxbvorbisenc.so
	$(RPL) -e "libvorbis.so.0" "libxbvorbis.so" $(PREFIX)/lib/libxbvorbisenc.so
	-$(READELF) --dynamic $(PREFIX)/lib/libxbvorbisenc.so | grep ibrary
	#
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

