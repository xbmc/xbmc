include ../../Makefile.include
DEPS = ../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=libasound
VERSION=1.1.4.1
SOURCE=alsa-lib-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2
SHA512=7b548c4ee29c4a1230a0edcd5d19219831290f96a214180a6530628acc05278d1348376195287d188f4f44d6be1914391c63994f1b50985c3eee74352da26b0b
include ../../download-files.include

# configuration settings
CFLAGS += \
	-fPIC -DPIC \
	-finline-limit=300 -finline-functions -fno-inline-functions-called-once \

CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          $(AUTORECONF) -vif; \
          ./configure --prefix=$(PREFIX) \
          --with-alsa-devdir=/dev/snd/ --with-plugindir=/system/usr/lib/alsa-lib --with-configdir=/system/usr/share/alsa \
          --with-ctl-plugins=ext \
          --with-pcm-plugins="copy,linear,route,mulaw,alaw,adpcm,rate,plug,multi,file,null,empty,share,meter,hooks,lfloat,ladspa,asym,iec958,softvol,extplug,ioplug,mmap_emul" \
          --disable-resmgr --enable-aload --enable-mixer  --enable-pcm  --enable-rawmidi  --enable-hwdep  --disable-seq  --disable-alisp  --disable-old-symbols --disable-python \
          --with-softfloat=yes --with-libdl=yes --with-pthread=yes --with-librt=no --enable-shared \

LIBDYLIB=$(PLATFORM)/src/.libs/$(LIBNAME).so

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/src

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/src install-libLTLIBRARIES
	$(MAKE) -C $(PLATFORM)/include install
	$(MAKE) -C $(PLATFORM)/utils install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
