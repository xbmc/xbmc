include ../../Makefile.include
include FFMPEG-VERSION
DEPS= ../../Makefile.include FFMPEG-VERSION Makefile

# set to "yes" to enable patching
# we don't apply patches until we move to a vanilla ffmpeg tarball
APPLY_PATCHES=no

# configuration settings
ffmpg_config = --prefix=$(PREFIX) --extra-version="kodi-$(VERSION)"
ffmpg_config += --cc=$(CC) --cxx=$(CXX) --ar=$(AR) --ranlib=$(RANLIB)
ffmpg_config += --disable-devices --disable-doc
ffmpg_config += --disable-ffplay --disable-ffmpeg --disable-sdl
ffmpg_config += --disable-ffprobe --disable-ffserver
ffmpg_config += --enable-gpl --enable-runtime-cpudetect
ffmpg_config += --enable-postproc --enable-pthreads
ffmpg_config += --enable-muxer=spdif --enable-muxer=adts
ffmpg_config += --enable-muxer=asf --enable-muxer=ipod
ffmpg_config += --enable-encoder=ac3 --enable-encoder=aac
ffmpg_config += --enable-encoder=wmav2 --enable-protocol=http
ffmpg_config += --enable-gnutls
ffmpg_config += --enable-encoder=png --enable-encoder=mjpeg

ifeq ($(CROSS_COMPILING), yes)
  ffmpg_config += --arch=$(CPU) --enable-cross-compile
endif
ifeq ($(OS), linux)
  ffmpg_config += --target-os=$(OS) --cpu=$(CPU)
  ffmpg_config += --enable-vdpau --enable-vaapi --enable-pic
endif
ifeq ($(OS), android)
  ifeq ($(findstring arm, $(CPU)), arm)
    ffmpg_config += --cpu=cortex-a9
  else
    ffmpg_config += --cpu=i686 --disable-mmx
  endif
  ffmpg_config += --target-os=linux
endif
ifeq ($(OS), ios)
  ifneq ($(CPU), arm64)
    ffmpg_config += --cpu=cortex-a8
  endif
  ffmpg_config += --yasmexe=$(NATIVEPREFIX)/bin/yasm
  ffmpg_config += --disable-decoder=mpeg_xvmc --disable-vda --disable-crystalhd --enable-videotoolbox
  ffmpg_config += --target-os=darwin
endif
ifeq ($(OS), osx)
  ffmpg_config += --disable-outdev=sdl
  ffmpg_config += --disable-decoder=mpeg_xvmc --disable-vda --disable-crystalhd --enable-videotoolbox
  ffmpg_config += --target-os=darwin
  ffmpg_config += --disable-securetransport
endif
ifeq ($(findstring arm, $(CPU)), arm)
  ffmpg_config += --enable-pic --disable-armv5te --disable-armv6t2
endif
ifeq ($(findstring mips, $(CPU)), mips)
  ffmpg_config += --disable-mips32r2 --disable-mipsdspr1 --disable-mipsdspr2
endif
ifeq ($(Configuration), Release)
  ffmpg_config += --disable-debug
endif


CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) -Ls --create-dirs -f -o $(TARBALLS_LOCATION)/$(ARCHIVE) $(BASE_URL)/$(VERSION).tar.gz

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); sed -i".bak" -e "s%pkg_config_default=pkg-config%export PKG_CONFIG_LIBDIR=$(PREFIX)/lib/pkgconfig \&\& pkg_config_default=$(NATIVEPREFIX)/bin/pkg-config%" configure
	cd $(PLATFORM);\
	CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" CPPFLAGS="$(CPPFLAGS)" LDFLAGS="$(LDFLAGS)" \
	./configure $(ffmpg_config)

build: $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): build
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
