include ../Makefile.include

ifneq ($(shell test -f $(NATIVEPREFIX)/share/config.site && echo 1),1)
  $(error Error: $(NATIVEPREFIX)/share/config.site  is missing. Please reconfigure depends to generate it)
endif

NATIVE= m4 gettext heimdal autoconf automake \
        libtool pkg-config yasm nasm cmake \
        gas-preprocessor libffi python3 zlib \
        pcre swig \
        libpng libjpeg-turbo liblzo2 giflib \
        setuptools distutilscross JsonSchemaBuilder TexturePacker \
        flatbuffers \
        meson ninja


ifeq ($(OS),darwin_embedded)
  NATIVE += dpkg xz tar gen_entitlements ldid
endif

ifeq ($(TARGET_PLATFORM),wayland)
  NATIVE += expat wayland-scanner waylandpp-scanner
  EXPAT = expat
endif

ifeq ($(TARGET_PLATFORM),gbm)
  NATIVE += MarkupSafe Mako
endif

.PHONY: $(NATIVE) native

all: native
	@echo "Dependencies built successfully."

# Dependency layout for parallel builds
autoconf: m4
automake: autoconf
dpkg: automake gettext libtool pkg-config tar
flatbuffers: cmake
heimdal: libtool
libtool: automake
libjpeg-turbo: cmake yasm
libpng: zlib
meson: python3 setuptools
ninja: meson
swig: pcre
distutilscross: python3
tar: xz automake
python3: $(EXPAT) libffi pkg-config zlib
setuptools: python3
wayland-scanner: expat
waylandpp-scanner: cmake

# python installs are not thread safe when using easy_install method.
# MarkupSafe doesn't really depend on ninja but we need to make the
# build sequential
MarkupSafe: ninja
Mako: MarkupSafe

#liblzo2 has stale packaged automake files that cause borked host/build detection
liblzo2: automake
JsonSchemaBuilder: automake
TexturePacker: automake pkg-config libpng liblzo2 giflib libjpeg-turbo

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

