include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS= ../../Makefile.include Makefile 01-disable-libedit.patch 02-autoconf-2.70-configure.patch

# lib name, version
LIBNAME=heimdal
VERSION=7.7.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

APP=$(PLATFORM)/lib/asn1/asn1_compile

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
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
