include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

#hint for building a fat lib - "lipo -arch i386 libboblight-i386.dylib -arch x86_64 libboblight-x86_64.dylib -output libboblight-fat.dylib"

# lib name, version
LIBNAME=libboblight
VERSION=r478
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) \
  --without-opengl \
  --without-portaudio \
  --without-x11 \
  --without-libusb

LIBDYLIB=$(PLATFORM)/src/.libs/$(LIBNAME).a

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: $(LIBDYLIB) .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p0 < ../01-fix_fpermissive.patch
	cd $(PLATFORM); patch -p0 < ../02-fixandroid.patch
	cd $(PLATFORM); autoreconf -vif
	cd $(PLATFORM); $(CONFIGURE)
ifeq ($(CPU),arm64)
	cd $(PLATFORM); patch -p0 < ../03-fixtvos.patch
	cd $(PLATFORM); sed -ie "s|-bind_at_load||" ./libtool
	cd $(PLATFORM); sed -ie "s|-bind_at_load||" ./ltmain.sh
endif

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
ifeq ($(OS),android)
	rm -f $(PLATFORM)/src/.libs/libboblight.la $(PLATFORM)/src/.libs/libboblight.so $(PLATFORM)/src/.libs/libboblight.so.0
	mv -f $(PLATFORM)/src/.libs/libboblight.so.0.0.0 $(PLATFORM)/src/.libs/libboblight.so
	$(RPL) -e "libboblight.so.0" "libboblight.so\x00\x00" $(PLATFORM)/src/.libs/libboblight.so
	-$(READELF) --dynamic $(PLATFORM)/src/.libs/libboblight.so | grep ibrary
endif
ifeq ($(OS),ios)
ifeq ($(TARGET_PLATFORM),appletvos)
	#deploy into source tree for tvos - we distribute libboblight with the bundle...
	cp $(PLATFORM)/src/.libs/libboblight.0.dylib $(CORE_SOURCE_DIR)/system/libboblight-tvos.0.dylib
endif
else
	echo "libboblight isn't a dependency of XBMC and won't be installed"
endif
	touch $@
clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -r .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
