include ../../Makefile.include
DEPS = ../../Makefile.include Makefile 01-fix_fpermissive.patch 02-fixandroid.patch 03-fixtvos.patch ../../download-files.include

#hint for building a fat lib - "lipo -arch i386 libboblight-i386.dylib -arch x86_64 libboblight-x86_64.dylib -output libboblight-fat.dylib"

# lib name, version
LIBNAME=libboblight
VERSION=r478
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=382e0b0f1ef2fca676cd64ec4190d3cfb791fed0f9477af8436e461cebfbc268058abc1fbba97a0337d3152a9b292580160b42157b4076d59b3847071deb1881
include ../../download-files.include

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) \
  --without-opengl \
  --without-portaudio \
  --without-x11 \
  --without-libusb

LIBDYLIB=$(PLATFORM)/src/.libs/$(LIBNAME).a

all: $(LIBDYLIB) .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-fix_fpermissive.patch
	cd $(PLATFORM); patch -p1 -i ../02-fixandroid.patch
	cd $(PLATFORM); autoreconf -vif
	cd $(PLATFORM); $(CONFIGURE)
ifeq ($(CPU),arm64)
	cd $(PLATFORM); patch -p1 -i ../03-fixtvos.patch
	cd $(PLATFORM); sed -ie "s|-bind_at_load||" ./libtool
	cd $(PLATFORM); sed -ie "s|-bind_at_load||" ./ltmain.sh
endif

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
ifeq ($(OS),darwin_embedded)
ifeq ($(TARGET_PLATFORM),appletvos)
	#deploy into source tree for tvos - we distribute libboblight with the bundle...
	cp $(PLATFORM)/src/.libs/libboblight.0.dylib $(CMAKE_SOURCE_DIR)/system/libboblight-tvos.0.dylib
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
