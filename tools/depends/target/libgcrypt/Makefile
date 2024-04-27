include ../../Makefile.include LIBGCRYPT-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile LIBGCRYPT-VERSION ../../download-files.include

ifeq ($(findstring apple-darwin, $(HOST)), apple-darwin)
  # required to fix asm link failure due C_SYMBOL_NAME macro not prepending _ to exported symbol
  # exported symbols only have s single _ instead of double __
  #   Undefined symbols for architecture x86_64:
  #   "__gcry_mpih_add_n", referenced from:
  CONFIGURE_FLAGS+= ac_cv_sys_symbol_underscore=yes
  ifeq ($(OS), darwin_embedded)
    # explicitly disable getentropy for ios/tvos as AC_CHECK_FUNCS(getentropy) incorrectly
    # succeeds as of libgcrypt 1.9.4
    CONFIGURE_FLAGS+= ac_cv_func_getentropy=no
  endif
endif

export GPG_ERROR_CONFIG=$(PREFIX)/bin/gpg-error-config

# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) --disable-doc --disable-shared $(CONFIGURE_FLAGS)

LIBDYLIB=$(PLATFORM)/src/.libs/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	# do not build the tests or docs
	sed -ie "s|\$$(doc) tests||" "$(PLATFORM)/Makefile.am"
	cd $(PLATFORM); $(AUTORECONF) -vif
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
