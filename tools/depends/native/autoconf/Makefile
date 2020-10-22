ifneq ("$(wildcard ../../Makefile.include)","")

include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS= ../../Makefile.include Makefile endian-c++11-narrowing.patch

else

include ../../Makefile.include.in
PREFIX=$(CURDIR)/../../pre-build-deps
PLATFORM=native
TARBALLS_LOCATION_BOOTSTRAP ?= $(PREFIX)
TARBALLS_LOCATION=$(TARBALLS_LOCATION_BOOTSTRAP)
RETRIEVE_TOOL=curl
ARCHIVE_TOOL=tar

export PATH:=$(CURDIR)/../../pre-build-deps/bin:$(PATH)

endif

# lib name, version
LIBNAME=autoconf
VERSION=2.69
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX)

LIBDYLIB=$(PLATFORM)/bin/autoconf

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	mkdir -p $(TARBALLS_LOCATION)
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	-rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../endian-c++11-narrowing.patch

$(LIBDYLIB): $(PLATFORM)
	cd $(PLATFORM); $(CONFIGURE)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
