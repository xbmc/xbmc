include ../../Makefile.include
DEPS = ../../Makefile.include Makefile 0001-fix-c++17-support.patch ../../download-files.include

# lib name, version
LIBNAME=p8-platform
VERSION=2.1.0.1
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=10f8e3ca8ea7a48923a4cc57d47015b56b4bdbf78997ba77abd9fc2f929198fda16dfb869ba69eec393ab4b635be916c3ed9a07d6989bf8265aa055794e84bf7
include ../../download-files.include

LIBDYLIB=$(PLATFORM)/build/lib$(LIBNAME).a

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../0001-fix-c++17-support.patch
	cd $(PLATFORM)/build; $(CMAKE) -DBUILD_SHARED_LIBS=0 -DCMAKE_INSTALL_LIBDIR=$(PREFIX)/lib ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
