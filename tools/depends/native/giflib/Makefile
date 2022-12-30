include ../../Makefile.include
DEPS = ../../Makefile.include Makefile 01-static-lib.patch ../../download-files.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)

# lib name, version
LIBNAME=giflib
VERSION=5.2.1
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=4550e53c21cb1191a4581e363fc9d0610da53f7898ca8320f0d3ef6711e76bdda2609c2df15dc94c45e28bff8de441f1227ec2da7ea827cb3c0405af4faa4736
include ../../download-files.include

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX)

LIBDYLIB=$(PLATFORM)/libgif.a

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	-rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-static-lib.patch

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM) libgif.a CFLAGS="$(NATIVE_CFLAGS)" LDFLAGS="$(NATIVE_LDFLAGS)"

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install-include PREFIX="$(PREFIX)"
	$(MAKE) -C $(PLATFORM) install-lib PREFIX="$(PREFIX)"
	touch $@
clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -r .installed-$(PLATFORM)
	rm -rf $(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
