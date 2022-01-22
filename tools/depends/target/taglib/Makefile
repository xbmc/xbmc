include ../../Makefile.include
DEPS = ../../Makefile.include Makefile ../../download-files.include

LIBNAME=taglib
VERSION=1.12
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(LIBNAME)-$(VERSION).tar.gz
SHA512=7e369faa5e3c6c6401052b7a19e35b0cf8c1e5ed9597053ac731a7718791d5d4803d1b18a93e903ec8c3fc6cb92e34d9616daa2ae4d326965d4c4d5624dcdaba
include ../../download-files.include

LIBDYLIB=$(PLATFORM)/build/$(LIBNAME)/libtag.a
all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); rm -rf build; mkdir -p build
	cd $(PLATFORM)/build; $(CMAKE) -DCMAKE_LIBRARY_PATH=$(TOOLCHAIN)/lib/$(HOST) -DCMAKE_CXX_FLAGS="$(CXXFLAGS) -std=c++11" -DBUILD_SHARED_LIBS=OFF ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build
	touch $@
.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	rm -f $(PREFIX)/lib/libtag_c.*
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
