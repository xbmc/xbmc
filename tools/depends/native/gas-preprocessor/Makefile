include ../../Makefile.include

GASBIN=$(NATIVEPREFIX)/bin/gas-preprocessor.pl

all: $(GASBIN)

download:

$(GASBIN):
	mkdir -p $(NATIVEPREFIX)/bin
	cp gas-preprocessor.pl $(GASBIN)

clean:
distclean::
	rm -f $(GASBIN)
