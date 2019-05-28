include ../../Makefile.include

GEBIN=$(NATIVEPREFIX)/bin/gen_entitlements.py

all: $(GEBIN)

$(GEBIN):
	mkdir -p $(NATIVEPREFIX)/bin
	cp gen_entitlements.py $(GEBIN)
	chmod 755 $(GEBIN)

clean:
distclean::
	rm -f $(GEBIN)
