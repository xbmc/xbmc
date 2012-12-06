include ../Makefile.include
DEPS= ../Makefile.include Makefile removeshm-2.patch removeshm-3.patch removeshm-upstream.patch timeval.patch

# lib name, version
LIBNAME=libasound
VERSION=1.0.23
SOURCE=alsa-lib-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2

# configuration settings
CFLAGS += \
	-fPIC -DPIC \
	-finline-limit=300 -finline-functions -fno-inline-functions-called-once \

CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          $(AUTORECONF) -vif; \
          ./configure --prefix=$(PREFIX) --host=$(HOST) \
          --with-alsa-devdir=/dev/snd/ --with-plugindir=/system/usr/lib/alsa-lib --with-configdir=/system/usr/share/alsa \
          --with-ctl-plugins=ext \
          --with-pcm-plugins="copy,linear,route,mulaw,alaw,adpcm,rate,plug,multi,file,null,empty,share,meter,hooks,lfloat,ladspa,asym,iec958,softvol,extplug,ioplug,mmap_emul" \
          --disable-resmgr --enable-aload --enable-mixer  --enable-pcm  --disable-rawmidi  --enable-hwdep  --disable-seq  --disable-alisp  --disable-old-symbols --disable-python \
          --with-softfloat=yes --with-libdl=yes --with-pthread=yes --with-librt=no \

LIBDYLIB=$(PLATFORM)/src/.libs/$(LIBNAME).so

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 < ../removeshm-upstream.patch
	cd $(PLATFORM); patch -p0 < ../removeshm-2.patch
	cd $(PLATFORM); patch -p0 < ../removeshm-3.patch
	cd $(PLATFORM); patch -p0 < ../timeval.patch
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/src

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/src install-libLTLIBRARIES
	$(MAKE) -C $(PLATFORM)/include install
	$(MAKE) -C $(PLATFORM)/utils install
	rm -f $(PREFIX)/lib/libasound.la $(PREFIX)/lib/libasound.so $(PREFIX)/lib/libxbasound.so $(PREFIX)/lib/libasound.so.2
	mv -f $(PREFIX)/lib/libasound.so.2.0.0 $(PREFIX)/lib/libasound.so
	$(RPL) -e "libasound.so.2" "libasound.so\x00\x00" $(PREFIX)/lib/libasound.so
	-$(READELF) --dynamic $(PREFIX)/lib/libasound.so | grep ibrary
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
