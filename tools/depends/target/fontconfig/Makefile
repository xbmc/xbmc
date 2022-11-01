include ../../Makefile.include FONTCONFIG-VERSION ../../download-files.include
DEPS = ../../Makefile.include FONTCONFIG-VERSION Makefile ../../download-files.include

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) \
  --disable-libxml2 --disable-docs --with-arch=$(PLATFORM) --disable-shared

ifeq ($(OS),android)
  # Freetype with Harfbuzz support requires stdc++ lib linking
  # Fontconfig uses clang and not clang++, therefore we need the explicit stdc++ link
  export LDFLAGS+= -lstdc++
endif

LIBDYLIB=$(PLATFORM)/src/.libs/lib$(LIBNAME).a

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
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
