include ../Makefile.include

ifneq ($(shell test -f $(NATIVEPREFIX)/share/config.site && echo 1),1)
  $(error Error: $(NATIVEPREFIX)/share/config.site  is missing. Please reconfigure depends to generate it)
endif

# Keep in alphabetical order
NATIVE= \
  autoconf \
  autoconf-archive \
  automake \
  cmake \
  flatbuffers \
  gas-preprocessor \
  gettext \
  giflib \
  heimdal \
  JsonSchemaBuilder \
  libjpeg-turbo \
  liblzo2 \
  libpng \
  libtool \
  m4 \
  meson \
  nasm \
  ninja \
  openssl \
  pcre \
  pkg-config \
  python3 \
  setuptools \
  swig \
  TexturePacker \
  zlib

ifneq ($(NATIVE_OS),osx)
  NATIVE += libffi
  LIBFFI = libffi
endif

ifeq ($(OS),darwin_embedded)
  NATIVE += dpkg xz tar ldid
endif

ifeq ($(OS),linux)
  NATIVE += expat wayland-scanner pugixml waylandpp-scanner
  EXPAT = expat

  ifeq ($(RENDER_SYSTEM),gles)
    NATIVE += MarkupSafe Mako
  endif
endif

.PHONY: $(NATIVE) native

all: native
	@echo "Dependencies built successfully."

# Dependency layout for parallel builds
autoconf-archive: autoconf
autoconf: m4
automake: autoconf
dpkg: automake gettext libtool pkg-config tar
flatbuffers: cmake
heimdal: libtool
JsonSchemaBuilder: automake
libjpeg-turbo: cmake nasm
libpng: zlib automake
libtool: automake
Mako: MarkupSafe
meson: python3 setuptools
ninja: meson
openssl: zlib
pugixml: cmake
python3: $(EXPAT) $(LIBFFI) pkg-config zlib openssl autoconf-archive
setuptools: python3
swig: pcre
tar: xz automake
TexturePacker: automake pkg-config libpng liblzo2 giflib libjpeg-turbo
wayland-scanner: expat pkg-config
waylandpp-scanner: cmake pugixml

# python installs are not thread safe when using easy_install method.
# MarkupSafe doesn't really depend on ninja but we need to make the
# build sequential
MarkupSafe: ninja

#liblzo2 has stale packaged automake files that cause borked host/build detection
liblzo2: automake

native: $(NATIVE)
$(NATIVE):
	$(MAKE) -C $@
clean:
	for d in $(NATIVE); do $(MAKE) -C $$d clean; done

# Debug target, this will DELETE all data in staging!
test-dependencies:
	( for d in $(NATIVE); do \
	rm -rf $(NATIVEPREFIX); \
        mkdir -p $(NATIVEPREFIX)/include $(NATIVEPREFIX)/share $(NATIVEPREFIX)/bin; \
	cp -f config.site $(NATIVEPREFIX)/share/; \
	$(MAKE) distclean; \
	$(MAKE) $$d; done ) && echo "$@ built successfully"

distclean::
	for d in $(NATIVE); do $(MAKE) -C $$d distclean; done

