include ../../Makefile.include
DEPS= ../../Makefile.include Makefile darwin_embedded_entitlements.xml

DARWIN_EMBEDDED_ENT=$(PREFIX)/share/darwin_embedded_entitlements.xml

all: $(DARWIN_EMBEDDED_ENT)

$(DARWIN_EMBEDDED_ENT):
	mkdir -p $(PREFIX)/share
	cp darwin_embedded_entitlements.xml $(DARWIN_EMBEDDED_ENT)


clean:
distclean::
	rm -f $(DARWIN_EMBEDDED_ENT)
