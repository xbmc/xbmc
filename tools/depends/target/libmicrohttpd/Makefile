include ../../Makefile.include LIBMICROHTTPD-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile LIBMICROHTTPD-VERSION ../../download-files.include

# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) \
                      --disable-shared \
                      --disable-doc \
                      --disable-examples \
                      --disable-curl \
                      --enable-https

ifeq ($(DEBUG_BUILD), yes)
  CONFIGURE+= --enable-asserts
endif

ifeq ($(findstring apple-darwin, $(HOST)), apple-darwin)
  # blanket disable timespec_get use for apple platforms. timespec_get was introduced in
  # __API_AVAILABLE(macosx(10.15), ios(13.0), tvos(13.0), watchos(6.0)) but older platforms
  # are failing to run.
  CONFIGURE+= mhd_cv_func_timespec_get=no
endif

LIBDYLIB=$(PLATFORM)/src/microhttpd/.libs/$(LIBNAME).a

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
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
