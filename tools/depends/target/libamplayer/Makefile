include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

all: .installed-$(PLATFORM)

.patched-$(PLATFORM): $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cp -rf libamplayer $(PLATFORM)/
	cd $(PLATFORM); patch -p1 < ../android-4.0-to-4.1.patch
	cd $(PLATFORM); patch -p1 < ../android-4.1-to-4.2.patch
	cd $(PLATFORM); patch -p1 < ../android-4.2-to-4.4.patch
	touch $@

.installed-$(PLATFORM): .patched-$(PLATFORM)
	mkdir -p $(PREFIX)/include/amcodec
	cp -rf $(PLATFORM)/libamplayer/include/amcodec/* $(PREFIX)/include/amcodec/
	mkdir -p $(PREFIX)/include/amplayer
	cp -rf $(PLATFORM)/libamplayer/include/amplayer/* $(PREFIX)/include/amplayer/
	touch $@

clean:
	rm -f .installed-$(PLATFORM)
	rm -f .patched-$(PLATFORM)

distclean:: clean
	rm -rf $(PLATFORM)
