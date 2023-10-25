include ../../Makefile.include OPENSSL-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile OPENSSL-VERSION ../../download-files.include \
                        001-android-getauxvalrevert.patch 16-kodi.conf

# configuration settings
ifeq ($(OS), linux)
  # Need to export our vars to override "default" conf data of openssl
  TARGETOPT=--with-zlib-include=$(PREFIX)/include --with-zlib-lib=$(PREFIX)/lib
  CONFIGURE=MACHINE=$(PLATFORM) ./config no-shared zlib no-tests no-asm $(TARGETOPT) --prefix=$(PREFIX) --libdir=lib
else
  ifeq ($(OS), android)
    TARGETOPT=--with-zlib-include=$(PREFIX)/include --with-zlib-lib=$(PREFIX)/lib -D__ANDROID_API__=$(NDK_LEVEL)
    # openssl 1.x incorrectly uses ANDROID_NDK_HOME, when openssl 3.x.x comes in use ANDROID_NDK_ROOT
    # see https://github.com/openssl/openssl/pull/11206
    export ANDROID_NDK_HOME=$(NDKROOT)
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

