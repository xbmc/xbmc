include ../../Makefile.include
include ../depends.mk
DEPS= ../../Makefile.include ../depends.mk Makefile

# set to "yes" to enable patching
# we don't apply patches until we move to a vanilla ffmpeg tarball
APPLY_PATCHES=no

# lib name, version
LIBNAME=ffmpeg
# ffmpeg GIT
BASE_URL=https://github.com/FernetMenta/FFmpeg/archive
VERSION=11086c540305ffb208e6fc17d301db989d3c4151
ARCHIVE=$(LIBNAME)-$(VERSION).tar.gz


# configuration settings
ffmpg_config = --sysroot=$(SYSROOT) --prefix=$(PREFIX)
ffmpg_config += --enable-cross-compile --target-os=linux --cpu=arm1176jzf-s --arch=arm
ffmpg_config += --cc=$(CC) --cxx=$(CXX)
ffmpg_config += --disable-devices --disable-doc
ffmpg_config += --disable-ffplay --disable-ffmpeg
ffmpg_config += --disable-ffprobe --disable-ffserver
ffmpg_config += --enable-gpl --enable-runtime-cpudetect
ffmpg_config += --enable-postproc --enable-pthreads
ffmpg_config += --enable-muxer=spdif --enable-muxer=adts
ffmpg_config += --enable-muxer=asf --enable-muxer=ipod
ffmpg_config += --enable-encoder=ac3 --enable-encoder=aac
ffmpg_config += --enable-encoder=wmav2 --enable-protocol=http
ffmpg_config += --enable-libvorbis --enable-muxer=ogg --enable-encoder=libvorbis
ffmpg_config += --disable-vaapi --disable-vdpau --disable-crystalhd
#ffmpg_config += --enable-gnutls
ifeq ($(Configuration), Release)
  ffmpg_config += --disable-debug
endif


CLEAN_FILES=$(ARCHIVE) rpi

all: .installed-rpi

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) -Ls --create-dirs -f -o $(TARBALLS_LOCATION)/$(ARCHIVE) $(BASE_URL)/$(VERSION).tar.gz

rpi: $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf rpi; mkdir -p rpi
	cd rpi; $(ARCHIVE_TOOL) --strip-components=1 -xf $(TARBALLS_LOCATION)/$(ARCHIVE)
	if test "$(APPLY_PATCHES)" = "yes"; then \
	  cd rpi; \
	  for PATCH in ../patches/*.patch ; do \
	    patch -p1 < $${PATCH}; \
	  done; \
        fi
	cd rpi;\
	CFLAGS="$(CFLAGS)" CXXFLAGS="$(CXXFLAGS)" CPPFLAGS="$(CPPFLAGS)" LDFLAGS="$(LDFLAGS)" \
	./configure $(ffmpg_config)

build: rpi
	$(MAKE) -j $(JOBS) -C rpi

.installed-rpi: build
	$(MAKE) -C rpi install
	touch $@

clean:
	$(MAKE) -C rpi clean
	rm -f .installed-rpi

distclean::
	rm -rf rpi .installed-rpi

