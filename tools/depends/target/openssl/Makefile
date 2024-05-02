include ../../Makefile.include OPENSSL-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile OPENSSL-VERSION ../../download-files.include \
                        001-android-getauxvalrevert.patch 16-kodi.conf

# configuration settings
ifeq ($(OS), linux)
  TARGETOPT=--with-zlib-include=$(PREFIX)/include --with-zlib-lib=$(PREFIX)/lib
  CONFIGURE=./Configure no-shared zlib no-tests no-asm no-module  $(TARGETOPT) --prefix=$(PREFIX) --libdir=lib
  ifeq ($(TARGET_PLATFORM),webos)
    CONFIGURE+= linux-armv4
  else
    ifeq ($(CPU),aarch64)
      CONFIGURE+= linux-aarch64
    else ifeq ($(CPU),arm)
      CONFIGURE+= linux-armv4
    else ifeq ($(CPU),x86_64)
      CONFIGURE+= linux-x86_64
    else
      # Fallback to base generic linux target
      CONFIGURE+= linux-generic32
    endif
  endif
else
  ifeq ($(OS), android)
    TARGETOPT=--with-zlib-include=$(PREFIX)/include --with-zlib-lib=$(PREFIX)/lib -U__ANDROID_API__ -D__ANDROID_API__=$(NDK_LEVEL)

    export ANDROID_NDK_ROOT=$(NDKROOT)
    export PATH:=$(TOOLCHAIN)/bin:$(PATH)
    ifeq ($(MESON_CPU), aarch64)
      OPENSSLPLATFORM=android-arm64
    else
      OPENSSLPLATFORM=android-$(MESON_CPU)
    endif
  endif
  ifeq ($(OS), darwin_embedded)
    # LLVM 15 has raised this to error by default. drop back to warning
    CFLAGS+= -Wno-error=implicit-int
    export SDKROOT CFLAGS
    OPENSSLPLATFORM=kodi-iphoneos-arm64

    ifeq ($(TARGET_PLATFORM),appletvos)
      OPENSSLPLATFORM=kodi-appletvos-arm64
      # Need to add "no-async" to avoid "'setcontext' is unavailable: not available on tvOS" error
      TARGETOPT+= no-async
    endif
  endif
  ifeq ($(OS), osx)
    OPENSSLPLATFORM=kodi-darwin64-$(CPU)
  endif

  # Openssl 3.2.0 allows no-apps no-docs as options. Use when we update further
  # We explicitly use no-asm as there is a symbol clash with gnutls
  CONFIGURE=./Configure no-shared zlib no-tests no-asm no-module  $(TARGETOPT) --prefix=$(PREFIX) $(OPENSSLPLATFORM)
endif

export CC CXX AR RANLIB
export CFLAGS CXXFLAGS CPPFLAGS LDFLAGS

LIBDYLIB=$(PLATFORM)/libssl.a

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
ifeq ($(OS),android)
	cd $(PLATFORM); patch -p1 -i ../001-android-getauxvalrevert.patch
endif
	cd $(PLATFORM); cp ../16-kodi.conf ./Configurations/
	cd $(PLATFORM); $(CONFIGURE)
	if test "$(OS)" = "darwin_embedded"; then \
		sed -E -ie "s|static volatile sig_atomic_t intr_signal;|static volatile intr_signal;|" "$(PLATFORM)/crypto/ui/ui_openssl.c"; \
	fi
	sed -ie "s|PROGRAMS=|PROGRAMS=#|" "$(PLATFORM)/Makefile";

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)
	touch $@

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install_sw
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

