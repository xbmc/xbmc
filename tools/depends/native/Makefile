include ../Makefile.include

ifneq ($(shell test -f $(NATIVEPREFIX)/share/config.site && echo 1),1)
  $(error Error: $(NATIVEPREFIX)/share/config.site  is missing. Please reconfigure depends to generate it)
endif

NATIVE= m4 gettext autoconf automake \
        libtool pkg-config yasm cmake \
        gas-preprocessor libffi python3 zlib \
        pcre swig \
        libpng libjpeg-turbo liblzo2 giflib \
        distribute distutilscross JsonSchemaBuilder TexturePacker \
        flatbuffers


ifeq ($(OS),darwin_embedded)
  NATIVE += dpkg xz tar gen_entitlements ldid
endif

ifeq ($(OS),osx)
  NATIVE += xz tar
endif

ifeq ($(OS),linux)
  ifeq ($(TARGET_PLATFORM),$(filter $(TARGET_PLATFORM),raspberry-pi gbm))
    NATIVE += meson ninja setuptools
  endif
endif

ifeq ($(TARGET_PLATFORM),wayland)
  NATIVE += expat wayland-scanner waylandpp-scanner
  EXPAT = expat
endif

.PHONY: $(NATIVE) native

all: native
	@echo "Dependencies built successfully."

# Dependency layout for parallel builds
autoconf: m4
automake: autoconf
dpkg: automake gettext libtool pkg-config tar
flatbuffers: cmake
libtool: automake
libpng: zlib
meson: python3 setuptools
ninja: python3
swig: pcre
distribute: python3
distutilscross: python3 distribute
tar: xz automake
python3: $(EXPAT) libffi pkg-config zlib
setuptools: python3
wayland-scanner: expat
waylandpp-scanner: cmake

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

