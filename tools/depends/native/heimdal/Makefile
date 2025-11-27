include ../../Makefile.include HEIMDAL-VERSION ../../download-files.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile HEIMDAL-VERSION ../../download-files.include \
			01-disable-libedit.patch \
			02-fix-build-with-autoconf-2.72.patch

APP=$(PLATFORM)/lib/asn1/asn1_compile

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-disable-libedit.patch
	cd $(PLATFORM); patch -p1 -i ../02-fix-build-with-autoconf-2.72.patch
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
