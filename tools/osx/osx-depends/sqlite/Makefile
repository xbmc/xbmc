#  A quick and dirty Makefile to download/build and install
#
#  Usage:
#    make
#    sudo make install

include ../config.mk

PKGFILE=$(PREFIX)/lib/pkgconfig/sqlite3.pc

all: $(PKGFILE)

$(PKGFILE):
	mkdir -p $(PREFIX)/lib/pkgconfig
	cp sqlite3.pc $(PREFIX)/lib/pkgconfig/sqlite3.pc
	touch $@

clean:
distclean::
	rm -f $(PKGFILE) .installed
