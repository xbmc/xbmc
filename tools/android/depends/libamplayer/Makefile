include ../Makefile.include
DEPS= ../Makefile.include Makefile

all: .installed-$(PLATFORM)

.installed-$(PLATFORM): $(DEPS)
	mkdir -p $(PREFIX)/include/amlplayer
	cp -rf libamplayer/include/* $(PREFIX)/include/amlplayer
	touch $@

clean:
	rm -f .installed-$(PLATFORM)

distclean:: clean
