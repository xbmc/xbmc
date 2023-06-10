include ../../Makefile.include
DEPS =../../Makefile.include Makefile ../../download-files.include

LIBNAME=hwdata
VERSION=0.368
ARCHIVE=$(LIBNAME)-$(VERSION).tar.gz
SHA512=a38adffa503f6f37ddefbad9a0fe9694605ffe54781fc88dd91937a09a54a7de70e027138f34a64bc3a701ab91656c1ae2dc938ed7cb0f73652d34a2ae917690
include ../../download-files.include

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

.installed-$(PLATFORM): $(PLATFORM)
	cd $(PLATFORM); ./configure --prefix=$(PREFIX) --datarootdir=$(PREFIX)/share --disable-blacklist
	cd $(PLATFORM); make install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
