include ../../Makefile.include
DEPS = ../../Makefile.include Makefile 01-all-disable-tests-docs.patch ../../download-files.include

# lib name, version
LIBNAME=libxslt
VERSION=1.1.34
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=1516a11ad608b04740674060d2c5d733b88889de5e413b9a4e8bf8d1a90d712149df6d2b1345b615f529d7c7d3fa6dae12e544da828b39c7d415e54c0ee0776b
include ../../download-files.include

# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) --without-python --disable-shared \
                      --with-debug --with-debugger --with-mem-debug \
                      --with-libxml-prefix=$(PREFIX) \
                      --with-libxml-libs-prefix=$(PREFIX)/lib \
                      --with-libxml-include-prefix=$(PREFIX)/include/libxml2


LIBDYLIB=$(PLATFORM)/$(LIBNAME)/.libs/$(LIBNAME).a

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-all-disable-tests-docs.patch
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)
	touch $@

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

