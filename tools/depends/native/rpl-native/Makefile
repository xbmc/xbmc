include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS= ../../Makefile.include.in Makefile rpl

all: .installed-$(PLATFORM)

.installed-$(PLATFORM): $(DEPS)
	mkdir -p $(PREFIX)/bin
	cp rpl $(PREFIX)/bin
	touch $@

clean:
	rm -f .installed-$(PLATFORM)

distclean::
	rm -f .installed-$(PLATFORM)
