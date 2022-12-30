include ../../Makefile.include LIBGPG-ERROR-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile LIBGPG-ERROR-VERSION ../../download-files.include \
                              tvos_remove-fork.patch

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) \
                      --disable-doc \
                      --disable-tests \
                      --disable-languages \
                      --disable-nls \
                      --disable-shared

LIBDYLIB=$(PLATFORM)/src/.libs/$(LIBNAME).a

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
ifeq ($(TARGET_PLATFORM),appletvos)
	cd $(PLATFORM); patch -p1 -i ../tvos_remove-fork.patch
endif
	cd $(PLATFORM); $(AUTORECONF) -vif
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
ifeq ($(OS),android)
ifeq ($(CPU),arm64-v8a)
	cp lock-obj-pub.aarch64-unknown-linux-android.h $(PLATFORM)/src/syscfg/lock-obj-pub.linux-android.h
else
	cp $(PLATFORM)/src/syscfg/lock-obj-pub.arm-unknown-linux-androideabi.h $(PLATFORM)/src/syscfg/lock-obj-pub.linux-android.h
endif
endif
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
