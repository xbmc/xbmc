include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

# lib name, version
LIBNAME=openssl
VERSION=1.0.2h
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=MACHINE=$(PLATFORM) ./config shared zlib --openssldir=$(PREFIX) --with-zlib-include=$(PREFIX)/include --with-zlib-lib=$(PREFIX)/lib
ifeq ($(OS), android)
  CONFIGURE=./Configure shared zlib --openssldir=$(PREFIX) --with-zlib-include=$(PREFIX)/include --with-zlib-lib=$(PREFIX)/lib linux-generic32
endif
ifeq ($(OS), ios)
  CONFIGURE=./Configure iphoneos-cross zlib no-asm no-krb5 --openssldir=$(PREFIX)
endif
ifeq ($(OS), osx)
  ifeq ($(CPU),x86_64)
    CONFIGURE=./Configure darwin64-$(CPU)-cc zlib no-asm no-krb5 shared --openssldir=$(PREFIX)
  else
    CONFIGURE=./Configure darwin-$(CPU)-cc zlib no-asm no-krb5 shared --openssldir=$(PREFIX)
  endif
endif
LIBDYLIB=$(PLATFORM)/libssl.a
CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); AR="$(AR)" CFLAGS="$(CFLAGS)" CC=$(CC) RANLIB=$(RANLIB) $(CONFIGURE)
	if test "$(OS)" = "osx"; then \
		sed -ie "s|CC= /usr/bin/gcc-4.2|CC= $(CC)|" "$(PLATFORM)/Makefile"; \
		sed -ie "s|CFLAG= |CFLAG=$(CFLAGS) |" "$(PLATFORM)/Makefile"; \
	fi
	# for iphoneos-cross config a sysroot argument is added
	# however sysroot already set in Makefile.include, so remove this 	
	if test "$(OS)" = "ios"; then \
		sed -ie "s|CFLAG= |CFLAG=$(CFLAGS) |" "$(PLATFORM)/Makefile"; \
		sed -ie "s|-isysroot \$$(CROSS_TOP)/SDKs/\$$(CROSS_SDK) ||" "$(PLATFORM)/Makefile"; \
		sed -ie "s|static volatile sig_atomic_t intr_signal;|static volatile intr_signal;|" "$(PLATFORM)/crypto/ui/ui_openssl.c"; \
	fi
	sed -ie "s|apps test||" "$(PLATFORM)/Makefile"; \

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)
	touch $@

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install_sw
	rm -f $(PREFIX)/lib/libcrypto.so*
	rm -f $(PREFIX)/lib/libssl.so*
	rm -rf $(PREFIX)/lib/libcrypto.*dylib*
	rm -rf $(PREFIX)/lib/libssl.*dylib*
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

