include ../../Makefile.include OPENSSL-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile OPENSSL-VERSION ../../download-files.include \
                        001-android-getauxvalrevert.patch 16-kodi.conf

# configuration settings
ifeq ($(OS), linux)
  # Need to export our vars to override "default" conf data of openssl
  TARGETOPT=--with-zlib-include=$(PREFIX)/include --with-zlib-lib=$(PREFIX)/lib
  CONFIGURE=MACHINE=$(PLATFORM) ./config no-shared zlib no-tests no-asm no-module $(TARGETOPT) --prefix=$(PREFIX) --libdir=lib
else
  ifeq ($(OS), android)
    TARGETOPT=--with-zlib-include=$(PREFIX)/include --with-zlib-lib=$(PREFIX)/lib

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
    OPENSSLPLATFORM=ios64-cross

    ifeq ($(TARGET_PLATFORM),appletvos)
      # Apple tv is the only platform we use a custom conf target
      OPENSSLPLATFORM=kodi-appletvos-arm64
      # Need to add "no-async" to avoid "'setcontext' is unavailable: not available on tvOS" error
      TARGETOPT=--config=$(PWD)/16-kodi.conf no-async
    endif
  endif
  ifeq ($(OS), osx)
    OPENSSLPLATFORM=darwin64-$(CPU)
  endif

  # Openssl 3.2.0 allows no-apps no-docs as options. Use when we update further
  CONFIGURE=./Configure no-shared zlib $(TARGETOPT) no-tests no-module --prefix=$(PREFIX) $(OPENSSLPLATFORM)
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

