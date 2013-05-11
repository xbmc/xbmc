include ../../Makefile.include
DEPS= ../../Makefile.include

SOURCE=libcrystalhd

all: .installed-$(PLATFORM)

.installed-$(PLATFORM): $(SOURCE)
	mkdir -p $(PREFIX)/include
	cp -rf $(SOURCE) $(PREFIX)/include/
	touch $@

clean:
distclean::
	rm -f .installed-$(PLATFORM)
