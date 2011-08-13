include ../Makefile.include

SOURCE=libcrystalhd

all: .installed

.installed:
	mkdir -p $(PREFIX)/include
	cp -rf $(SOURCE) $(PREFIX)/include/
	touch $@

clean:
distclean::
	rm -f .installed

