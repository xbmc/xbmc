include ../../Makefile.include
DEPS = ../../Makefile.include Makefile 01-SDL_SetWidthHeight.patch 02-OSX_interpretKeyEvents.patch 03-mavericks-compile.patch 04-fix_external_screen_crash.patch ../../download-files.include

# lib name, version
LIBNAME=SDL
VERSION=1.2.15
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=ac392d916e6953b0925a7cbb0f232affea33339ef69b47a0a7898492afb9784b93138986df53d6da6d3e2ad79af1e9482df565ecca30f89428be0ae6851b1adc
include ../../download-files.include

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --disable-video-directfb
ifneq ($(OS),linux)
CONFIGURE += --without-x --disable-video-x11
endif

LIBDYLIB=$(PLATFORM)/build/.libs/lib$(LIBNAME).a

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-SDL_SetWidthHeight.patch
	cd $(PLATFORM); patch -p1 -i ../02-OSX_interpretKeyEvents.patch
	cd $(PLATFORM); patch -p1 -i ../03-mavericks-compile.patch
	cd $(PLATFORM); patch -p1 -i ../04-fix_external_screen_crash.patch
	cd $(PLATFORM); ./autogen.sh
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -j 1 -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
