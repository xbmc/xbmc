include ../../Makefile.include OPENSSL-VERSION ../../download-files.include
DEPS = ../../Makefile.include OPENSSL-VERSION Makefile 16-kodi.conf ../../download-files.include

# configuration settings
ifeq ($(OS), linux)
  # Need to export our vars to override "default" conf data of openssl
  TARGETOPT=--with-zlib-include=$(PREFIX)/include --with-zlib-lib=$(PREFIX)/lib
  CONFIGURE=MACHINE=$(PLATFORM) ./config no-shared zlib no-tests no-asm $(TARGETOPT) --prefix=$(PREFIX) --libdir=lib
else
  ifeq ($(OS), android)
    TARGETOPT=--with-zlib-include=$(PREFIX)/include --with-zlib-lib=$(PREFIX)/lib -D__ANDROID_API__=$(NDK_LEVEL)
    export ANDROID_NDK_ROOT=$(NDKROOT)
    export PATH:=$(TOOLCHAIN)/bin:$(PATH)
    ifeq ($(MESON_CPU), aarch64)
      OPENSSLPLATFORM=android-arm64
    else
      OPENSSLPLATFORM=android-$(MESON_CPU)
    endif
  endif
  ifeq ($(OS), darwin_embedded)
    export SDKROOT CFLAGS
    OPENSSLPLATFORM=kodi-$(TARGET_PLATFORM)-$(CPU)
    ifeq ($(TARGET_PLATFORM),appletvos)
      # Need to add "no-async" to avoid "'setcontext' is unavailable: not available on tvOS" error
      TARGETOPT=no-async
    endif
  endif
  ifeq ($(OS), osx)
    OPENSSLPLATFORM=kodi-darwin64-$(CPU)
    ASMFLAG=no-asm
  endif

  CONFIGURE=./Configure $(OPENSSLPLATFORM) no-shared $(ASMFLAG) zlib no-tests $(TARGETOPT) --prefix=$(PREFIX)
endif

export CC CXX AR RANLIB
export CFLAGS CXXFLAGS CPPFLAGS LDFLAGS

LIBDYLIB=$(PLATFORM)/$(BYPRODUCT)

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
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

