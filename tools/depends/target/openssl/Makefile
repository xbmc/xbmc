include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

# lib name, version
LIBNAME=openssl
VERSION=1.1.1j
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

BASE_URL=https://www.openssl.org/source

# configuration settings
ifeq ($(OS), linux)
  TARGETOPT=--with-zlib-include=$(PREFIX)/include --with-zlib-lib=$(PREFIX)/lib
endif
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
  OPENSSLPLATFORM=darwin64-$(CPU)-cc
  ifeq ($(TARGET_PLATFORM),appletvos)
    # Need to add "no-async" to avoid "'setcontext' is unavailable: not available on tvOS" error
    TARGETOPT=no-async
  endif
endif
ifeq ($(OS), osx)
  OPENSSLPLATFORM=darwin64-$(CPU)-cc
endif

ifeq ($(OS), linux)
  # config auto detects well on linux. Do we need MACHINE ??
  CONFIGURE=MACHINE=$(PLATFORM) ./config no-shared zlib no-tests no-asm $(TARGETOPT) --prefix=$(PREFIX)
else
  CONFIGURE=./Configure $(OPENSSLPLATFORM) no-shared zlib no-tests $(TARGETOPT) --prefix=$(PREFIX)
endif

LIBDYLIB=$(PLATFORM)/libssl.a

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); AR="$(AR)" CFLAGS="$(CFLAGS)" CC="$(CC)" RANLIB=$(RANLIB) $(CONFIGURE)
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

