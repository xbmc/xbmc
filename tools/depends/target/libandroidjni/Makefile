include ../../Makefile.include
DEPS = ../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=libandroidjni
VERSION=7b3e6a3be0e4f3c704016c44eeb37b5f026d6b9a
SOURCE=archive
ARCHIVE=$(VERSION).tar.gz
SHA512=6dfe592e13a7b1ce68b349201a4f0051b365471a545fa90e4a77e0a8f506710d6bfa8e8d0df1b972c345d41e490229d9e7fc2acd49be0f4a49176ea056ad9a3e
include ../../download-files.include
GIT_BASE_URL=https://github.com/xbmc

LIBDYLIB=$(PLATFORM)/$(LIBNAME)/.libs/$(LIBNAME).a

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(GIT_BASE_URL)/$(LIBNAME)/$(SOURCE)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); rm -rf build; mkdir -p build
	cd $(PLATFORM)/build; $(CMAKE) -DCMAKE_LIBRARY_PATH=$(TOOLCHAIN)/lib/$(HOST) ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

