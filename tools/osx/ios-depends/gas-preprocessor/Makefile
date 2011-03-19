#  A quick and dirty Makefile to download/build and install
#
#  Usage:
#    make
#    sudo make install

include ../config.mk

GASBIN=$(PREFIX)/bin/gas-preprocessor.pl

all: .installed

.installed:
	mkdir -p $(PREFIX)/bin
	cp gas-preprocessor.pl $(GASBIN)
	touch $@

clean:
distclean::
	rm $(GASBIN) .installed
