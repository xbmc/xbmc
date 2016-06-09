include ../Makefile.include

ifneq ($(shell test -f $(NATIVEPREFIX)/share/config.site && echo 1),1)
  $(error Error: $(NATIVEPREFIX)/share/config.site  is missing. Please reconfigure depends to generate it)
endif

NATIVE= m4-native gettext-native autoconf-native automake-native \
        libtool-native pkg-config-native yasm-native cmake-native \
        gas-preprocessor-native python27-native zlib-native \
        pcre-native swig-native rpl-native \
        libpng-native libjpeg-turbo-native liblzo2-native giflib-native \
        distribute-native distutilscross-native JsonSchemaBuilder TexturePacker \
        google-breakpad-native


ifeq ($(OS),ios)
  NATIVE += dpkg-native xz-native tar-native fakeroot-native gen_entitlements-native ldid-native
endif

ifeq ($(OS),osx)
  NATIVE += xz-native tar-native
endif

.PHONY: $(NATIVE) native

all: native
	@echo "Dependencies built successfully."

# Dependency layout for parallel builds
autoconf-native: m4-native
automake-native: autoconf-native
libtool-native: automake-native
libpng-native: zlib-native
swig-native: pcre-native
distribute-native: python27-native
distutilscross-native: python27-native distribute-native
tar-native: xz-native automake-native
python27-native: zlib-native

#liblzo2 has stale packaged automake files that cause borked host/build detection
liblzo2-native: automake-native
JsonSchemaBuilder: automake-native
TexturePacker: automake-native pkg-config-native libpng-native liblzo2-native giflib-native libjpeg-turbo-native

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

