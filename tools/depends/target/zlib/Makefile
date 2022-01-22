include ../../Makefile.include
DEPS = ../../Makefile.include Makefile visibility.patch remove_fixe_ar_osx.patch ../../download-files.include

# lib name, version
LIBNAME=zlib
VERSION=1.2.11
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=73fd3fff4adeccd4894084c15ddac89890cd10ef105dd5e1835e1e9bbb6a49ff229713bd197d203edfa17c2727700fce65a2a235f07568212d820dca88b528ae
include ../../download-files.include

# configuration settings
CONFIGURE= RANLIB="$(RANLIB)" LD="$(LD)" AR="$(AR)" CC="$(CC)" CFLAGS="$(CFLAGS)" ./configure --prefix=$(PREFIX) --static

LIBDYLIB=$(PLATFORM)/$(LIBNAME).a

all: .installed-$(PLATFORM)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../visibility.patch
	cd $(PLATFORM); patch -p1 -i ../remove_fixe_ar_osx.patch
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $(LIBDYLIB)
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
