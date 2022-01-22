include ../../Makefile.include
DEPS = ../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=libxml2
VERSION=2.9.12
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=df1c6486e80f0fcf3c506f3599bcfb94b620c00d0b5d26831bc983daa78d58ec58b5057b1ec7c1a26c694f40199c6234ee2a6dcabf65abfa10c447cb5705abbd
include ../../download-files.include

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --without-python --without-iconv --disable-shared

LIBDYLIB=$(PLATFORM)/.libs/$(LIBNAME).a

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); ./autogen.sh
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

