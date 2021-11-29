include ../../Makefile.include
include FFMPEG-VERSION
DEPS= ../../Makefile.include FFMPEG-VERSION Makefile

export PKG_CONFIG_LIBDIR=$(PREFIX)/lib/pkgconfig

# configuration settings
ffmpg_config = --prefix=$(PREFIX) --extra-version="kodi-$(VERSION)"
ffmpg_config += --cc="$(CC)" --cxx="$(CXX)" --ar=$(AR) --ranlib=$(RANLIB) --strip=$(STRIP)
ffmpg_config += --extra-cflags="$(CFLAGS)"
ffmpg_config += --extra-cxxflags="$(CXXFLAGS)"
ffmpg_config += --extra-ldflags="$(LDFLAGS)"
ffmpg_config += --pkg-config=$(NATIVEPREFIX)/bin/pkg-config

ffmpg_config += --disable-doc
ffmpg_config += --disable-devices
ffmpg_config += --disable-programs
ffmpg_config += --disable-sdl2
ffmpg_config += --enable-gpl
ffmpg_config += --enable-postproc
ffmpg_config += --enable-runtime-cpudetect
ffmpg_config += --enable-pthreads
ffmpg_config += --enable-gnutls
ffmpg_config += --enable-libdav1d
ffmpg_config += $(FFMPEG_CONFIGURE_OPTIONS)

CONFIGARCH= --arch=$(CPU)

ifeq ($(CROSS_COMPILING), yes)
  ffmpg_config += --enable-cross-compile
  ffmpg_config += --pkg-config-flags=--static
  ffmpg_config += --enable-pic
endif
ifeq ($(OS), linux)
  ffmpg_config += --target-os=$(OS)
endif
ifeq ($(OS), android)
  ifeq ($(findstring arm64, $(CPU)), arm64)
    ffmpg_config += --cpu=cortex-a53
    CONFIGARCH= --arch=aarch64
  else
    ifeq ($(findstring arm, $(CPU)), arm)
      ffmpg_config += --cpu=cortex-a9
    else
      ifeq ($(CPU), x86_64)
        ffmpg_config += --cpu=$(CPU)
      else
        ffmpg_config += --cpu=i686 --disable-mmx
      endif
      ffmpg_config += --extra-cflags='-mno-stackrealign'
    endif
  endif
  ffmpg_config += --target-os=$(OS) --extra-libs=-liconv
endif
ifeq ($(OS), darwin_embedded)
  ffmpg_config += --disable-crystalhd --enable-videotoolbox
  ffmpg_config += --target-os=darwin
endif
ifeq ($(OS), osx)
  ffmpg_config += --disable-crystalhd --enable-videotoolbox
  ffmpg_config += --target-os=darwin
  ffmpg_config += --disable-securetransport
endif
ifeq ($(findstring arm, $(CPU)), arm)
  ffmpg_config += --disable-armv5te --disable-armv6t2
endif
ifeq ($(findstring mips, $(CPU)), mips)
  ffmpg_config += --disable-mips32r2 --disable-mipsdsp --disable-mipsdspr2
endif
ifeq ($(CPU),x86)
  ffmpg_config += --x86asmexe=$(NATIVEPREFIX)/bin/nasm
else ifeq ($(CPU),x86_64)
  ffmpg_config += --x86asmexe=$(NATIVEPREFIX)/bin/nasm
endif
ifeq ($(Configuration), Release)
  ffmpg_config += --disable-debug
endif

ffmpg_config +=$(CONFIGARCH)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) -Ls --create-dirs -f -o $(TARBALLS_LOCATION)/$(ARCHIVE) $(BASE_URL)/archive/$(VERSION).tar.gz

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM);	./configure $(ffmpg_config)

.build-$(PLATFORM): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)
	touch $@

.installed-$(PLATFORM): .build-$(PLATFORM)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
