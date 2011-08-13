include ../Makefile.include

GASBIN=$(TOOLCHAIN)/bin/gas-preprocessor.pl

all: $(GASBIN)

$(GASBIN):
	mkdir -p $(TOOLCHAIN)/bin
	cp gas-preprocessor.pl $(GASBIN)

clean:
distclean::
	rm -f $(GASBIN)
