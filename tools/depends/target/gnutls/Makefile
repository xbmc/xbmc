include ../../Makefile.include GNUTLS-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile ../../download-files.include \
                              0002-makefile-am-wasm-fix.patch \
                              0003-counted-by-clang-19.patch

# disable rpl_malloc and rpl_realloc symbols in glib
# causes linking errors in libmicrohttpd and ffmpeg with missing symbols
# Undefined symbols for architecture arm64:
#  "_rpl_realloc", referenced from:
#      _gnutls_realloc in libgnutls.a(mem.o)
#      _gnutls_realloc_zero in libgnutls.a(init.o)
#      __asn1_realloc in libgnutls.a(parser_aux.o)
#      __asn1_ordering_set_of in libgnutls.a(coding.o)
#      __asn1_realloc in libgnutls.a(decoding.o)
#      __gnutls_fread_file in libgnutls.a(read-file.o)
#      _rpl_getdelim in libgnutls.a(getdelim.o)
#      ...
CONFIGURE_OPTIONS+= ac_cv_func_malloc_0_nonnull=yes \
                    ac_cv_func_realloc_0_nonnull=yes

ifeq ($(findstring apple-darwin, $(HOST)), apple-darwin)
  # strchrnul is newer than the deployment targets (macOS 15.4, iOS/tvOS 18.4),
  # make gnulib use its replacement
  CONFIGURE_OPTIONS+= gl_cv_onwards_func_strchrnul=future
endif

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) \
                      --disable-shared \
                      --without-p11-kit \
                      --disable-nls \
                      --with-included-unistring \
                      --enable-local-libopts \
                      --disable-doc \
                      --disable-tests \
                      --disable-guile \
                      --disable-tools \
                      --without-idn \
                      --without-brotli \
                      --without-zstd \
                      $(CONFIGURE_OPTIONS)

# LLVM 15 has raised this to error by default. drop back to warning
CFLAGS+= -Wno-error=implicit-int -Wno-error=implicit-function-declaration -Wno-error=int-conversion 
export CFLAGS

LIBDYLIB=$(PLATFORM)/lib/.libs/lib$(LIBNAME).a

all: .installed-$(PLATFORM)

.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../0002-makefile-am-wasm-fix.patch
	cd $(PLATFORM); patch -p1 -i ../0003-counted-by-clang-19.patch
	cd $(PLATFORM); $(AUTORECONF) -vif
	cd $(PLATFORM); $(CONFIGURE)
	touch $@

$(LIBDYLIB): .configured-$(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)
