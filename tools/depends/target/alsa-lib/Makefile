include ../../Makefile.include
DEPS = ../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=libasound
VERSION=1.2.8
SOURCE=alsa-lib-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2
SHA512=865ff05a8f589996f8d63d43a91c961f1b64144f3e1d17c7074b7ac16f25b3fd1c371d46ed63a8cc20fa01e63c76b75f1a9802b56889ae1073854dd050d27688
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


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
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
