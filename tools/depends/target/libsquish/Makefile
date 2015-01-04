include ../../Makefile.include
DEPS = ../../Makefile.include.in Makefile

# reuse native src
SOURCE=$(abs_top_srcdir)/native/libsquish-native/src

DYLIB=$(PLATFORM)/libsquish.a

all: .installed-$(PLATFORM)

$(PLATFORM):
	-rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); cp -a $(SOURCE)/* .
	cd $(PLATFORM); mv Makefile.target Makefile
	cd $(PLATFORM); mkdir -p $(PREFIX)/include; mkdir -p $(PREFIX)/lib/pkgconfig

$(DYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(DYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
