include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS= ../../Makefile.include Makefile uconfig_defines.patch

# lib name, version
LIBNAME=icu
VERSION=54_1
SOURCE=$(LIBNAME)4c-$(VERSION)-src
ARCHIVE=$(SOURCE).tgz
ICU_PLATFORM=

ifeq ($(findstring linux,$(NATIVEPLATFORM)),linux)
	ICU_PLATFORM=Linux/GCC
else ifeq ($(findstring darwin,$(NATIVEPLATFORM)),darwin)
	ICU_PLATFORM=MacOSX
endif
# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
        ./runConfigureICU $(ICU_PLATFORM) --prefix=$(PREFIX) --disable-tests --disable-samples

LIBDYLIB=$(PLATFORM)/source/lib/lib$(LIBNAME)uc.so

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM)/source; patch -p0 < ../../uconfig_defines.patch
	cd $(PLATFORM)/source; $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/source

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/source install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/source clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
