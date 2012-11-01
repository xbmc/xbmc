NATIVE_BUILD=1
include ../Makefile.include
DEPS= ../Makefile.include Makefile rpl

all: .installed-$(PLATFORM)

.installed-$(PLATFORM): $(DEPS)
	mkdir -p $(PREFIX)/bin
	cp rpl $(PREFIX)/bin
	touch $@

clean:
	rm -f .installed-$(PLATFORM)

distclean::
	rm -f .installed-$(PLATFORM)
