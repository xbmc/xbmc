include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

all: .installed-$(PLATFORM)

.installed-$(PLATFORM): $(DEPS)
	mkdir -p $(PREFIX)/include/amcodec
	cp -rf libamplayer/include/amcodec/* $(PREFIX)/include/amcodec/
	mkdir -p $(PREFIX)/include/amplayer
	cp -rf libamplayer/include/amplayer/* $(PREFIX)/include/amplayer/
	touch $@

clean:
	rm -f .installed-$(PLATFORM)

distclean:: clean
