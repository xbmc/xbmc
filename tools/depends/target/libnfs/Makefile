include ../../Makefile.include
DEPS = ../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=libnfs
VERSION=17f882fbdbe7f739d1285f173781c1525a29a7fa
# Github commit date of version hash
COMMITDATE=2020-06-09
ARCHIVE=$(LIBNAME)-$(VERSION).tar.gz
SHA512=0a367863ed47cc041007480473da4e936ef81978689d35cd048f5f72cbbf300b6e6982559303c67c2b6efd6c3b90ceea57930440f849f3921a8b2c40395e03f1
include ../../download-files.include

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --disable-shared --disable-utils --disable-examples --disable-werror

LIBDYLIB=$(PLATFORM)/lib/.libs/$(LIBNAME).a

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	-rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); ./bootstrap
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -r .installed-$(PLATFORM)
	rm -rf $(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
