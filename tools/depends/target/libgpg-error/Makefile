include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

# lib name, version
LIBNAME=libgpg-error
VERSION=1.20
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) \
  --disable-languages \
  --disable-nls --disable-shared

LIBDYLIB=$(PLATFORM)/src/.libs/$(LIBNAME).a

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
ifeq ($(OS),osx)
	$(MAKE) -C $(PLATFORM)/src gen-posix-lock-obj
	$(PLATFORM)/src/gen-posix-lock-obj > $(PLATFORM)/src/syscfg/tmp.h
	mv $(PLATFORM)/src/syscfg/tmp.h $(PLATFORM)/src/syscfg/$$(awk 'NR==1 {print $$2}' $(PLATFORM)/src/syscfg/tmp.h)
endif
ifeq ($(OS),android)
	cp $(PLATFORM)/src/syscfg/lock-obj-pub.arm-unknown-linux-androideabi.h $(PLATFORM)/src/syscfg/lock-obj-pub.linux-android.h
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
