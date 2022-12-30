include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile 01-disable-libedit.patch 02-autoconf-2.70-configure.patch ../../download-files.include

# lib name, version
LIBNAME=heimdal
VERSION=7.7.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=6660939b5a36ce36310721a08a089fb671d1e3d2e8ac74ea4775bfa5f8f772d32de805551456200fe96cc486c092c44beb84f5dd877008bc305490ee971bbf99
include ../../download-files.include

APP=$(PLATFORM)/lib/asn1/asn1_compile

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-disable-libedit.patch
	cd $(PLATFORM); patch -p1 -i ../02-autoconf-2.70-configure.patch
	cd $(PLATFORM); $(AUTORECONF) -vif
	cd $(PLATFORM); ./configure --prefix=$(PREFIX) --disable-shared

$(APP): $(PLATFORM)
	make -C $(PLATFORM)/include
	make -C $(PLATFORM)/lib/roken
	make -C $(PLATFORM)/lib/vers
	make -C $(PLATFORM)/lib/com_err
	make -C $(PLATFORM)/lib/asn1

.installed-$(PLATFORM): $(APP)
	install $(PLATFORM)/lib/asn1/asn1_compile $(PLATFORM)/lib/com_err/compile_et $(PREFIX)/bin
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
