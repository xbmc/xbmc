include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=libtool
VERSION=2.4.6
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=3233d81cb2739a54b840a0a82064eebbfaa4fb442fb993a35d6bd41d8395c51f038c90ae048b9252f172d0a5bbfb4b36e2b13d4477001f9ff7d4124237819a18
include ../../download-files.include

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) \
  --disable-shared --disable-ltdl-install

LIBDYLIB=$(PLATFORM)/libtool

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	-rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
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
